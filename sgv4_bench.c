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
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <sys/time.h>
#include <time.h>
#include <byteswap.h>

#include "libbsg.h"

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

void loop(int bsg_fd, int total, int max_outstanding, int bs, int rw)
{
	int i, ret, done, outstanding;
	char *buf;
	unsigned char scb[10], sense[32];
	struct pollfd pfd[1];
	struct sg_io_v4 *hdrs, hdr;
	struct timeval a, b;
	unsigned long long aa, bb;
	long double elasped_sec;
	unsigned long long sent_bytes;

	ret = fcntl(bsg_fd, F_GETFL);
	if (ret < 0) {
		fprintf(stderr, "can't set non-blocking %m\n");
		exit(1);
	}

	ret = fcntl(bsg_fd, F_SETFL, ret | O_NONBLOCK);
	if (ret == -1) {
		fprintf(stderr, "can't set non-blocking %m\n");
		exit(1);
	}

	buf = malloc(bs);
	if (!buf) {
		fprintf(stderr, "oom %m\n");
		exit(1);
	}

	hdrs = malloc(sizeof(hdr) * max_outstanding);
	if (!hdrs) {
		fprintf(stderr, "oom %m\n");
		exit(1);
	}

	memset(pfd, 0, sizeof(pfd));
	pfd[0].fd = bsg_fd;
	pfd[0].events = POLLIN;

	done = outstanding = 0;

	gettimeofday(&a, NULL);
	while (total > done) {
	again:
		setup_rw_scb(scb, sizeof(scb), rw, bs, bs * done);

		if (rw == READ_10)
			setup_sgv4_hdr(&hdr, scb, sizeof(scb), sense,
				       sizeof(sense), buf, bs, NULL, 0);
		else
			setup_sgv4_hdr(&hdr, scb, sizeof(scb), sense,
				       sizeof(sense), NULL, 0, buf, bs);

		ret = write(bsg_fd, &hdr, sizeof(hdr));
		if (ret < 0) {
			fprintf(stderr, "fail to write bsg dev, %m\n");
			break;
		}

		done++;
		outstanding++;

		if ((max_outstanding > outstanding) && (total > done))
			goto again;

		ret = poll(pfd, 1, -1);
		ret = read(bsg_fd, hdrs, sizeof(hdr) * max_outstanding);

		outstanding -= ret / sizeof(hdr);

		for (i = 0; i < ret / sizeof(hdr); i++) {
			if (sgv4_rsp_check(&hdr))
				fprintf(stderr, "error %u %u %u\n",
					hdrs[i].driver_status,
					hdrs[i].transport_status, hdrs[i].device_status);
		}
	}
	gettimeofday(&b, NULL);

	aa = a.tv_sec * 1000 * 1000 + a.tv_usec;
	bb = b.tv_sec * 1000 * 1000 + b.tv_usec;

	elasped_sec = (bb - aa) / (1000 * 1000.0);
	sent_bytes = done * bs;

	printf("block size : %u\n", bs);
	printf("outstanding : %u\n", max_outstanding);
	printf("done : %u\n", done);
	printf("elapsed time : %Lf[s]\n", elasped_sec);
	printf("totalbyte : %llu [bytes]\n", sent_bytes);
	printf("bandwidht : %Lf [KB/s], %Lf [MB/s]\n",
	       sent_bytes / elasped_sec / 1024.0,
	       sent_bytes / elasped_sec / 1024.0 / 1024.0);
}

int main(int argc, char **argv)
{
	int longindex, ch;
	int bs = SECTOR_SIZE, count = 1, bsg_fd;
	int rw = READ_10, max_outstanding = 1;

	while ((ch = getopt_long(argc, argv, "b:c:wo:h", long_options,
				 &longindex)) >= 0) {
		switch (ch) {
		case 'b':
			bs = atoi(optarg);
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

	if (optind == argc) {
		fprintf(stderr, "specify a bsg device\n");
		usage(1);
	}

	bsg_fd = open_bsg_dev(argv[optind]);
	if (bsg_fd < 0)
		exit(1);

	loop(bsg_fd, count, max_outstanding, bs, rw);

	return 0;
}
