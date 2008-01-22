/*
 * makeshift tool to send XDWRITEREAD_10 requests via bsg
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

#ifndef XDWRITEREAD_10
#define XDWRITEREAD_10 0x53
#endif

static char pname[] = "sgv4_xdwriteread";

static struct option const long_options[] =
{
	{"length", required_argument, 0, 'l'},
	{"outfile", required_argument, 0, 'o'},
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
  -l, --length            number of bytes. Default is 8192.\n\
  -o, --outfile           write to FILE.\n\
  -h, --help              display this help and exit\n\
");
	}
	exit(status);
}

static int xdwriteread(int bsg_fd, int bufsize, char *outfile)
{
	struct sg_io_v4 hdr;
	unsigned char scb[10], sense[32];
	char *in, *out;
	int ret;

	bufsize &= ~(1024 - 1);

	in = valloc(bufsize);
	out = valloc(bufsize + 1024);

	if (!in || !out) {
		printf("out of memory\n");
		exit(1);
	}

	memset(in, 0, bufsize);
	memset(out, 0, bufsize + 1024);

	setup_sgv4_hdr(&hdr, scb, sizeof(scb), sense, sizeof(sense),
		       in, bufsize, out + 1024, bufsize);

	setup_rw_scb(scb, sizeof(scb), XDWRITEREAD_10, bufsize, 0);

	ret = write(bsg_fd, &hdr, sizeof(hdr));
	if (ret != sizeof(hdr)) {
		fprintf(stderr, "Can't send a bsg request, %m\n");
		goto out;
	}

	ret = read(bsg_fd, &hdr, sizeof(hdr));
	if (ret != sizeof(hdr))
		fprintf(stderr, "Can't get a bsg response, %m\n");

	printf("driver:%u, transport:%u, device:%u, din_resid: %d, dout_resid: %d\n",
	       hdr.driver_status, hdr.transport_status, hdr.device_status,
	       hdr.din_resid, hdr.dout_resid);

	if (outfile) {
		int fd = open(outfile, O_RDWR|O_CREAT);

		ret = write(fd, in, bufsize);
		if (ret == bufsize)
			printf("writes %d bytes to %s\n", bufsize, outfile);
		else
			printf("failed to write %d bytes to %s\n", bufsize,
			       outfile);
	}

out:
	free(in);
	free(out);
	return 0;
}

int main(int argc, char **argv)
{
	int longindex, ch;
	int bsg_fd;
	int length = 8192;
	char *out = NULL;

	while ((ch = getopt_long(argc, argv, "l:h", long_options,
				 &longindex)) >= 0) {
		switch (ch) {
		case 'l':
			length = atoi(optarg);
			break;
		case 'o':
			out = optarg;
			break;
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
			break;
		}
	}

	bsg_fd = open_bsg_dev(argv[optind]);
	if (bsg_fd < 0) {
		fprintf(stderr, "Can't open %s's bsg device.\n", argv[optind]);
	}

	xdwriteread(bsg_fd, length, out);

	close(bsg_fd);

	return 0;
}
