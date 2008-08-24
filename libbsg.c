/*
 * bsg lib functions
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
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include "libbsg.h"

int open_bsg_dev(char *in)
{
	int maj, min, err, fd = -1;
	struct timeval t;
	FILE *fp;
	char buf[1024], *dev;

	if (in[strlen(in) - 1] == '/')
		in[strlen(in) - 1] = 0;

	dev = strrchr(in, '/');
	if (!dev)
		return -ENOENT;

	snprintf(buf, sizeof(buf), "/sys/class/bsg/%s/dev", dev);

	fp = fopen(buf, "r");
	if (!fp)
		return -errno;

	if (!fgets(buf, sizeof(buf), fp))
		goto close_sysfs;

	if (sscanf(buf, "%d:%d", &maj, &min) != 2)
		goto close_sysfs;

	err = gettimeofday(&t, NULL);
	if (err)
		goto close_sysfs;

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "/tmp/%lx%lx", t.tv_sec, t.tv_usec);
	err = mknod(buf, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
		    makedev(maj, min));
	if (err)
		goto close_sysfs;

	fd = open(buf, O_RDWR);
	if (fd < 0)
		fprintf(stderr, "can't open %s, %m\n", buf);

	unlink(buf);

close_sysfs:
	fclose(fp);

	return fd >=0 ? fd : -errno;
}

void setup_sgv4_hdr(struct sg_io_v4 *hdr, unsigned char *scb, int scb_len,
		    unsigned char *sense, int sense_len,
		    char *rbuf, int rlen, char *wbuf, int wlen)
{
	memset(hdr, 0, sizeof(*hdr));

	hdr->guard = 'Q';
	hdr->request_len = scb_len;
	hdr->request = (unsigned long) scb;
	hdr->max_response_len = sense_len;
	hdr->response = (unsigned long) sense;

	hdr->din_xfer_len = rlen;
	hdr->din_xferp = (unsigned long) rbuf;

	hdr->dout_xfer_len = wlen;
	hdr->dout_xferp = (unsigned long) wbuf;
}

void setup_rw_scb(unsigned char *scb, int scb_len, unsigned char cmd,
		  unsigned long len, unsigned long offset)
{
	memset(scb, 0, scb_len);

	scb[0] = cmd;
	scb[7] = (unsigned char)(((len / SECTOR_SIZE) >> 8) & 0xff);
	scb[8] = (unsigned char)((len / SECTOR_SIZE) & 0xff);

	*((uint32_t *) &scb[2]) = htonl(offset / SECTOR_SIZE);
}
