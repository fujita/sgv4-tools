/*
 * sgv4 sequential I/O benchmark
 *
 * Copyright (C) 2007 FUJITA Tomonori <tomof@acm.org>
 *
 * Released under the terms of the GNU GPL v2.0.
 */

#include <inttypes.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mount.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <sys/time.h>
#include <time.h>
#include <byteswap.h>

#include "libbsg.h"

#define MAX_DEVICE_NR 8

static char pname[] = "sgv4_bench";

static struct option const long_options[] =
{
	{"blksize", required_argument, 0, 'b'},
	{"count", required_argument, 0, 'c'},
	{"write", no_argument, 0, 'w'},
	{"outstanding", required_argument, 0, 'o'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0},
};

static void usage(int status)
{
	if (status)
		fprintf(stderr, "Try `%s --help' for more information.\n", pname);
	else {
		printf("Usage: %s [DEVICE]\n", pname);
		printf("\
  -b, --blksize           I/O size.\n\
  -c, --count             number of I/O requests. Default is 1\n\
  -w, --write             Do write I/Os.\n\
  -o, --outstanding       number of outstanding I/O requests. Default is 1\n\
  -h, --help              display this help and exit\n\
");
	}
	exit(status);
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define __be32_to_cpu(x) bswap_32(x)
#else
#define __be32_to_cpu(x) (x)
#endif

static int get_capacity(int fd, uint64_t *size)
{
	unsigned char scb[10], sense[32];
	unsigned int buf[2];
	int ret;
	struct sg_io_v4 hdr;

	memset(scb, 0, sizeof(scb));
	memset(buf, 0, sizeof(buf));
	memset(&hdr, 0, sizeof(hdr));

	scb[0] = READ_CAPACITY;

	setup_sgv4_hdr(&hdr, scb, sizeof(scb), sense, sizeof(sense),
		       (char *)buf, sizeof(buf), NULL, 0);

	ret = ioctl(fd, SG_IO, &hdr);
	if (ret) {
		fprintf(stderr, "%m\n");
		return ret;
	}

	*size = (uint64_t)__be32_to_cpu(buf[0]) * (uint64_t)__be32_to_cpu(buf[1]);

	return 0;
}

struct bsg_dev_info {
	int fd;
	uint64_t size;

	int done;
	int outstanding;
};

static struct bsg_dev_info bi[MAX_DEVICE_NR];

static void loop(int nr, int total, int max_outstanding, int bs, int rw)
{
	int i, ret, no_more_submit = 0;
	char *buf;
	unsigned char scb[10], sense[32];
	struct sg_io_v4 *hdrs, hdr;
	struct timeval a, b;
	unsigned long long aa, bb;
	long double elasped_sec;
	unsigned long long sent_bytes;
	unsigned long long total_sent_bytes;
	struct pollfd pfd[MAX_DEVICE_NR];

	for (i = 0; i < nr; i++) {
		ret = fcntl(bi[i].fd, F_GETFL);
		if (ret < 0) {
			fprintf(stderr, "can't set non-blocking %m\n");
			exit(1);
		}

		ret = fcntl(bi[i].fd, F_SETFL, ret | O_NONBLOCK);
		if (ret == -1) {
			fprintf(stderr, "can't set non-blocking %m\n");
			exit(1);
		}

		pfd[i].fd = bi[i].fd;
		pfd[i].events = POLLIN;
		pfd[i].revents = 0;
	}

	buf = valloc(bs);
	if (!buf) {
		fprintf(stderr, "oom %m\n");
		exit(1);
	}

	hdrs = malloc(sizeof(hdr) * max_outstanding * nr);
	if (!hdrs) {
		fprintf(stderr, "oom %m\n");
		exit(1);
	}

	gettimeofday(&a, NULL);
	while (1) {
		for (i = 0; i < nr && !no_more_submit; i++) {

			if (max_outstanding == bi[i].outstanding ||
			    total == bi[i].outstanding + bi[i].done)
				continue;
		again:
			setup_rw_scb(scb, sizeof(scb), rw, bs,
				     ((bs * (bi[i].done + bi[i].outstanding)) % bi[i].size));

			if (rw == READ_10)
				setup_sgv4_hdr(&hdr, scb, sizeof(scb), sense,
					       sizeof(sense), buf, bs, NULL, 0);
			else
				setup_sgv4_hdr(&hdr, scb, sizeof(scb), sense,
					       sizeof(sense), NULL, 0, buf, bs);

			hdr.flags |= BSG_FLAG_Q_AT_TAIL;

			ret = write(bi[i].fd, &hdr, sizeof(hdr));
			if (ret < 0) {
				fprintf(stderr, "fail to write bsg dev, %m\n");
				exit(1);
			}

			bi[i].outstanding++;

			if (max_outstanding > bi[i].outstanding &&
			    total > bi[i].outstanding + bi[i].done)
				goto again;
		}

		ret = poll(pfd, nr, -1);
		if (ret < 0) {
			fprintf(stderr, "failed to poll from bsg dev, %m\n");
			exit(1);
		}

		for (i = 0; i < nr; i++) {
			int j, done;

			if (!(pfd[i].revents & POLLIN))
				continue;

			pfd[i].revents = 0;

			done = read(bi[i].fd, hdrs,
				    sizeof(hdr) * max_outstanding * nr);
			if (done < 0) {
				fprintf(stderr, "fail to read from bsg dev, %m\n");
				exit(1);
			}

			done /= sizeof(hdr);

			bi[i].outstanding -= done;
			bi[i].done += done;

			if (bi[i].done == total) {
				pfd[i].events = 0;
				no_more_submit++;
			}

			for (j = 0; j < done; j++) {
				if (sgv4_rsp_check(&hdr))
					fprintf(stderr, "error %u %u %u\n",
						hdrs[j].driver_status,
						hdrs[j].transport_status,
						hdrs[j].device_status);
			}
		}

		if (no_more_submit) {
			int outstanding = 0;
			for (i = 0; i < nr; i++)
				outstanding += bi[i].outstanding;

			if (!outstanding)
				break;
		}
	}

	gettimeofday(&b, NULL);

	aa = a.tv_sec * 1000 * 1000 + a.tv_usec;
	bb = b.tv_sec * 1000 * 1000 + b.tv_usec;

	elasped_sec = (bb - aa) / (1000 * 1000.0);

	printf("block size : %u\n", bs);
	printf("outstanding : %u\n", max_outstanding);
	printf("elapsed time : %Lf[s]\n", elasped_sec);

	total_sent_bytes = 0;

	for (i = 0; i < nr; i++) {
		sent_bytes = (unsigned long long)bi[i].done * (unsigned long long)bs;
		total_sent_bytes += sent_bytes;

		printf("\n%dth device\n", i);
		printf("done : %u\n", bi[i].done);
		printf("totalbyte : %llu [bytes]\n", sent_bytes);
		printf("bandwidht : %Lf [KB/s], %Lf [MB/s]\n",
		       sent_bytes / elasped_sec / 1024.0,
		       sent_bytes / elasped_sec / 1024.0 / 1024.0);
	}

	if (nr)
		printf("\ntotal bandwidht : %Lf [KB/s], %Lf [MB/s]\n",
		       total_sent_bytes / elasped_sec / 1024.0,
		       total_sent_bytes / elasped_sec / 1024.0 / 1024.0);
}

static int parse_blocksize(char *str)
{
	int v;

	v = atoi(str);
	switch (str[strlen(str) - 1]) {
	case 'k':
		v *= 1024;
		break;
	case 'm':
		v *= (1024 * 1024);
		break;
	}

	return v;
}

int main(int argc, char **argv)
{
	int longindex, ch;
	int bs = SECTOR_SIZE, count = 1, i;
	int rw = READ_10, max_outstanding = 32, ret;

	while ((ch = getopt_long(argc, argv, "b:c:wo:h", long_options,
				 &longindex)) >= 0) {
		switch (ch) {
		case 'b':
			bs = parse_blocksize(optarg);
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case 'w':
			rw = WRITE_10;
			break;
		case 'o':
			max_outstanding = atoi(optarg);
			break;
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
			break;
		}
	}

	if (bs % SECTOR_SIZE) {
		fprintf(stderr, "The I/O size should be a multiple of %d\n",
			SECTOR_SIZE);
		exit(1);
	}

	if (!max_outstanding) {
		fprintf(stderr, "The number outstanding shouldn't be zero\n");
		exit(1);
	}

	if (!count) {
		fprintf(stderr, "The number requests shouldn't be zero\n");
		exit(1);
	}

	if (max_outstanding > count)
		max_outstanding = count;

	if (argc == optind) {
		fprintf(stderr, "specify a bsg device\n");
		usage(1);
	}

	if (argc - optind > MAX_DEVICE_NR) {
		fprintf(stderr, "too many devices, the max is %d\n",
			MAX_DEVICE_NR);
		exit(1);
	}

	for (i = 0; i < argc - optind; i++) {
		bi[i].fd = open_bsg_dev(argv[optind + i]);
		if (bi[i].fd < 0)
			exit(1);

		ret = get_capacity(bi[i].fd, &(bi[i].size));
		if (ret) {
			fprintf(stderr, "can't get the capacity\n");
			exit(1);
		}
	}

	loop(argc - optind, count, max_outstanding, bs, rw);

	return 0;
}
