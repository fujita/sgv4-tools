/*
 * This is a Serial Attached SCSI (SAS) management protocol (SMP)
 * utility program via bsg.
 *
 * This utility issues a REPORT MANUFACTURER INFORMATION function and
 * outputs its response.
 *
 */

/*
 * Copyright (c) 2006 Douglas Gilbert.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
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
#include <linux/bsg.h>
#include <sys/time.h>
#include <time.h>
#include <byteswap.h>

#include "libbsg.h"

static char pname[] = "smp_rep_manufacturer";

static struct option const long_options[] =
{
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
  -h, --help              display this help and exit\n\
");
	}
	exit(status);
}

#define SMP_FRAME_TYPE_REQ 0x40
#define SMP_FN_REPORT_MANUFACTURER 0x1

int main(int argc, char **argv)
{
	int longindex, ch;
	int bsg_fd, req_len, rsp_len, res;
	struct sg_io_v4 hdr;
	char cmd[16];
	char *req, *smp_resp;
	unsigned char smp_req[] = {SMP_FRAME_TYPE_REQ,
				   SMP_FN_REPORT_MANUFACTURER, 0, 0, 0, 0, 0, 0};

	while ((ch = getopt_long(argc, argv, "b:c:wo:h", long_options,
				 &longindex)) >= 0) {
		switch (ch) {
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
			break;
		}
	}

	if (optind == argc) {
		fprintf(stderr, "specify a bsg device\n");
		usage(1);
	}

	bsg_fd = open_bsg_dev(argv[optind]);
	if (bsg_fd < 0) {
		printf("can't open %s\n", argv[optind]);
		exit(1);
	}

	req_len = sizeof(smp_req);
	rsp_len = 64;

	req = valloc(req_len);
	smp_resp = valloc(rsp_len);

	memset(&hdr, 0, sizeof(hdr));
	memcpy(req, smp_req, req_len);
	memset(smp_resp, 0, sizeof(smp_resp));

	hdr.guard = 'Q';
	hdr.subprotocol = BSG_SUB_PROTOCOL_SCSI_TRANSPORT;
	/* dummy */
	hdr.request_len = sizeof(cmd);
	hdr.request = (unsigned long) cmd;

	hdr.din_xfer_len = rsp_len;
	hdr.din_xferp = (unsigned long) smp_resp;

	hdr.dout_xfer_len = req_len;
	hdr.dout_xferp = (unsigned long) req;

	res = ioctl(bsg_fd, SG_IO, &hdr);
	if (res) {
		printf("%d, %d\n", res, errno);
		exit(1);
	}

	printf("  SAS-1.1 format: %d\n", smp_resp[8] & 1);
	printf("  vendor identification: %.8s\n", smp_resp + 12);
	printf("  product identification: %.16s\n", smp_resp + 20);
	printf("  product revision level: %.4s\n", smp_resp + 36);
	if (smp_resp[40])
		printf("  component vendor identification: %.8s\n", smp_resp + 40);
	res = (smp_resp[48] << 8) + smp_resp[49];
	if (res)
		printf("  component id: %d\n", res);
	if (smp_resp[50])
		printf("  component revision level: %d\n", smp_resp[50]);

	return 0;
}
