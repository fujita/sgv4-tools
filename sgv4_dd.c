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
#include <linux/bsg.h>

#include "libbsg.h"

#define SECTOR_SIZE 512

static char pname[] = "sgv4_dd";

static struct option const long_options[] =
{
	{"count", required_argument, 0, 'c'},
	{"infile", required_argument, 0, 'i'},
	{"outfile", required_argument, 0, 'o'},
	{"sgio", no_argument, 0, 's'},
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
  -c, --count             number of BLOCKS. Default is 1\n\
  -i, --infile            read from FILE instead of stdin.\n\
  -o, --outfile           write to FILE instead of stdout.\n\
  -s, --sgio              Use SG_IO (ioctl) instead of read/write interface\n\
  -h, --help              display this help and exit\n\
");
		printf("\n\
Examples:\n\
  $ sgv4_dd -i /dev/sdb -o /dev/null\n\
  $ sgv4_dd -c 16 -s -i /dev/sdb -o /dev/null\n\
");
	}
	exit(status);
}

int main(int argc, char **argv)
{
	int err, longindex, ch;
	int in_fd, out_fd, bs, count, bsg_fd = -1, sgio = 0, rw = -1;
	struct sg_io_v4 hdr;
	char scb[10], sense[32], *buf, *in, *out;

	bs = SECTOR_SIZE;
	count = 1;
	in = out = NULL;

	while ((ch = getopt_long(argc, argv, "c:i:o:sh", long_options,
				 &longindex)) >= 0) {
		switch (ch) {
		case 'c':
			count = atoi(optarg);
			break;
		case 'i':
			in = optarg;
			break;
		case 'o':
			out = optarg;
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

	if (in) {
		in_fd = open_bsg_dev(in);
		if (in_fd > 0) {
			rw = READ_10;
			bsg_fd = in_fd;
		} else {
			in_fd = open(in, O_RDONLY);
			if (in_fd < 0) {
				fprintf(stderr, "%m %s\n", in);
				exit(1);
			}
		}
	} else
		in_fd = STDIN_FILENO;

	if (out) {
		out_fd = open_bsg_dev(out);
		if (out_fd > 0) {
			rw = WRITE_10;
			bsg_fd = out_fd;
		} else {
			out_fd = open(out, O_RDWR|O_CREAT);
			if (out_fd < 0) {
				fprintf(stderr, "%m %s\n", out);
				exit(1);
			}
		}
	} else
		out_fd = STDOUT_FILENO;

	buf = valloc(bs * count);
	if (!buf) {
		close(in_fd);
		return -ENOMEM;
	}

	memset(&hdr, 0, sizeof(hdr));
	memset(&scb, 0, sizeof(scb));

	scb[0] = rw;
	scb[7] = (unsigned char)(((bs * count / SECTOR_SIZE) >> 8) & 0xff);
	scb[8] = (unsigned char)((bs * count / SECTOR_SIZE) & 0xff);

	hdr.guard = 'Q';
	hdr.request_len = sizeof(scb);
	hdr.request = (unsigned long) scb;
	hdr.max_response_len = sizeof(sense);
	hdr.response = (unsigned long) sense;

	if (rw == READ_10) {
		hdr.din_xfer_len = bs * count;
		hdr.din_xferp = (unsigned long) buf;
	} else if (rw == WRITE_10) {
		hdr.dout_xfer_len = bs * count;
		hdr.dout_xferp = (unsigned long) buf;
	} else {
		fprintf(stderr, "infile or outfile must be a bsd device\n");
		return -EINVAL;
	}

	if (rw == WRITE_10) {
		err = read(in_fd, buf, bs * count);
		if (err < 0) {
			fprintf(stderr, "fail to read from the in file, %m\n");
			goto out;
		}
	}

	if (sgio) {
		err = ioctl(bsg_fd, SG_IO, &hdr);
		if (err) {
			fprintf(stderr, "fail to sgio, %m\n");
			goto out;
		}
	} else {
		err = write(bsg_fd, &hdr, sizeof(hdr));
		if (err < 0) {
			fprintf(stderr, "fail to write bsg dev, %m\n");
			goto out;
		}

		err = read(bsg_fd, &hdr, sizeof(hdr));
		if (err < 0) {
			fprintf(stderr, "fail to write bsg dev, %m\n");
			goto out;
		}
	}

	if (hdr.driver_status || hdr.transport_status || hdr.device_status
	    || hdr.din_resid) {
		fprintf(stderr, "error %u %u %u\n", hdr.driver_status,
			hdr.transport_status, hdr.device_status);
		err = -EINVAL;
		goto out;
	}

	if (rw == READ_10) {
		err = write(out_fd, buf, bs * count);
		if (err < 0)
			fprintf(stderr, "fail to write the out file, %m\n");
	}

	err = 0;
	printf("succeeded (%s)\n", sgio ? "SG_IO" : "read/write interface");
out:
	return err;
}
