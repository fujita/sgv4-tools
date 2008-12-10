/*
 * makeshift sgv4 dd tool
 *
 * Copyright (C) 2007 FUJITA Tomonori <tomof@acm.org>
 *
 * Released under the terms of the GNU GPL v2.0.
 */

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
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include "libbsg.h"

static char pname[] = "sgv4_dd";
static int sgio;
static int align;

static struct option const long_options[] =
{
	{"sgio", no_argument, 0, 's'},
	{"align", required_argument, 0, 'a'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0},
};

static void usage(int status)
{
	if (status)
		fprintf(stderr, "Try `%s --help' for more information.\n", pname);
	else {
		printf("Usage: %s [OPTIONS]...\n", pname);
		printf("\
  -s, --sgio              Use SG_IO (ioctl) instead of read/write interface\n\
  -h, --help              display this help and exit\n\
");
		printf("\n\
Examples:\n\
  $ sgv4_dd if=/sys/class/bsg/0\:0\:0\:0 of=/dev/null count=1 bs=4k\n\
");
	}
	exit(status);
}

static int sgv4_read(int fd, char *p, int len, unsigned offset)
{
	struct sg_io_v4 hdr;
	unsigned char scb[10];
	unsigned char sense[32];
	int ret;

	setup_rw_scb(scb, sizeof(scb), READ_10, len, offset);

	setup_sgv4_hdr(&hdr, scb, sizeof(scb), sense,
		       sizeof(sense), p, len, NULL, 0);

	if (sgio) {
		ret = ioctl(fd, SG_IO, &hdr);
		if (ret) {
			fprintf(stderr, "fail to sgio, %m\n");
			goto out;
		}
	} else {
		ret = write(fd, &hdr, sizeof(hdr));
		if (ret != sizeof(hdr)) {
			fprintf(stderr, "fail to write bsg dev, %m\n");
			goto out;
		}

		memset(&hdr, 0, sizeof(hdr));
		ret = read(fd, &hdr, sizeof(hdr));
		if (ret != sizeof(hdr)) {
			fprintf(stderr, "fail to wait for the response, %m\n");
			goto out;
		}
	}

	if (sgv4_rsp_check(&hdr)) {
		fprintf(stderr, "error %x %x %x %u\n",
			hdr.driver_status, hdr.transport_status,
			hdr.device_status, hdr.din_resid);
		ret = -EIO;
		goto out;
	}

	ret = 0;
out:
	return ret;
}

static int sgv4_write(int fd, char *p, int len, unsigned offset)
{
	struct sg_io_v4 hdr;
	unsigned char scb[10];
	unsigned char sense[32];
	int ret;

	setup_rw_scb(scb, sizeof(scb), WRITE_10, len, offset);

	setup_sgv4_hdr(&hdr, scb, sizeof(scb), sense,
		       sizeof(sense), NULL, 0, p, len);

	printf("%s %d len %d off %u\n", __func__, __LINE__, len, offset);

	if (sgio) {
		ret = ioctl(fd, SG_IO, &hdr);
		if (ret) {
			fprintf(stderr, "fail to sgio, %m\n");
			goto out;
		}
	} else {
		ret = write(fd, &hdr, sizeof(hdr));
		if (ret != sizeof(hdr)) {
			fprintf(stderr, "fail to write bsg dev, %m\n");
			goto out;
		}

		memset(&hdr, 0, sizeof(hdr));
		ret = read(fd, &hdr, sizeof(hdr));
		if (ret != sizeof(hdr)) {
			fprintf(stderr, "fail to wait for the response, %m\n");
			goto out;
		}
	}

	if (sgv4_rsp_check(&hdr)) {
		fprintf(stderr, "error %x %x %x %u\n",
			hdr.driver_status, hdr.transport_status,
			hdr.device_status, hdr.din_resid);
		ret = -EIO;
		goto out;
	}
	ret = 0;
out:
	return ret;
}

int main(int argc, char **argv)
{
	int longindex, ch;
	int if_fd, of_fd, i;
	char *p;
	char *if_file, *of_file;
	int count, bs;
	int ret = -EINVAL;
	void *buf;
	unsigned if_offset, of_offset;
	int if_sg, of_sg;

	while ((ch = getopt_long(argc, argv, "a:sh", long_options,
				 &longindex)) >= 0) {
		switch (ch) {
		case 'a':
			align = strtod(optarg, NULL);
			break;
		case 's':
			sgio = 1;
			break;
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
			break;
		}
	}

	if_file = of_file = NULL;
	count = 1;
	bs = 0;
	if_offset = of_offset = 0;
	if_sg = of_sg = 0;

	for (i = optind; i < argc; i++) {
		char *q = NULL;

		p = strchr(argv[i], '=');
		if (!p)
			continue;

		*p++ = '\0';

		if (!strcmp(argv[i], "if"))
			if_file = p;
		else if (!strcmp(argv[i], "of"))
			of_file = p;
		else if (!strcmp(argv[i], "count"))
			count = strtod(p, NULL);
		else if (!strcmp(argv[i], "bs")) {
			bs = strtod(p, &q);
			if (!*q)
				;
			else if (*q == 'k')
				bs *= 1024;
			else if (*q == 'm')
				bs *= (1024 * 1024);
			else {
				printf("unknown size, %s\n", p);
				goto out;
			}

		} else {
			printf("unknown option, %s\n", argv[i]);
			goto out;
		}
	}

	if (!if_file) {
		printf("if is not specified\n");
		goto out;
	}

	if (!of_file) {
		printf("of is not specified\n");
		goto out;
	}

	if_sg = !!strstr(if_file, "/sys/class/bsg/");
	of_sg = !!strstr(of_file, "/sys/class/bsg/");

	if (!if_sg && !of_sg) {
		printf("if (%s) or of (%s) must not a sg device\n",
		       if_file, of_file);
		goto out;
	}

	if (!bs || bs % SECTOR_SIZE) {
		printf("bs must be a multiple of 512, %d\n", bs);
		goto out;
	}

	if (!count) {
		printf("count must not be zero\n");
		goto out;
	}

	ret = 0;

	buf = malloc(bs + align);
	if (!buf) {
		ret = -ENOMEM;
		goto out;
	}

	buf += align;

	if (if_sg)
		if_fd = open_bsg_dev(if_file);
	else
		if_fd = open(if_file, O_RDWR);
	if (if_fd < 0) {
		printf("can't open if, %s %s\n", strerror(errno), if_file);
		goto out;
	}

	if (of_sg)
		of_fd = open_bsg_dev(of_file);
	else
		of_fd = open(of_file, O_RDWR |O_CREAT);
	if (of_fd < 0) {
		printf("can't open of, %s %s\n", strerror(errno), of_file);
		goto out;
	}

	printf("%s %d: %d %d align %d\n", __func__, __LINE__, if_sg, of_sg, align);

	for (i = 0; i < count; i++) {
		if (if_sg) {
			ret = sgv4_read(if_fd, buf, bs, if_offset);
			if (ret)
				break;
		} else {
			ret = pread(if_fd, buf, bs, if_offset);
			if (ret != bs) {
				printf("%s %d: %d\n", __func__, __LINE__, ret);
				goto out;
			}
		}

		if (of_sg) {
			ret = sgv4_write(of_fd, buf, bs, of_offset);
			if (ret)
				break;
		} else {
			ret = pwrite(of_fd, buf, bs, of_offset);
			if (ret != bs) {
				printf("%s %d: %d\n", __func__, __LINE__, ret);
				goto out;
			}
		}

		if_offset += bs;
		of_offset += bs;
	}

	free(buf - align);

	printf("succeeded (%s)\n", sgio ? "SG_IO" : "read/write interface");
out:
	return ret;
}
