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
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <linux/bsg.h>

#include "libbsg.h"

int open_bsg_dev(char *in)
{
	int major, minor, err, fd = -1;
	FILE *fp;
	char buf[1024], *path, *dev;

	dev = strrchr(in, '/');
	if (!dev)
		return -ENOENT;

	snprintf(buf, sizeof(buf), "/sys/class/bsg/%s/dev", dev);

	fp = fopen(buf, "r");
	if (!fp)
		return -errno;

	if (!fgets(buf, sizeof(buf), fp))
		goto close_sysfs;

	if (sscanf(buf, "%d:%d", &major, &minor) != 2)
		goto close_sysfs;

	path = tempnam("/tmp", NULL);
	err = mknod(path, (S_IFCHR | 0600), (major << 8) | minor);
	if (err)
		goto free_tempname;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "can't open %s, %m\n", path);
		goto free_tempname;
	}

	unlink(path);

free_tempname:
	free(path);
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

	*((uint32_t *) &scb[2]) = htonl(offset);
}
