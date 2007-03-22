#ifndef __LIBBSG_H
#define __LIBBSG_H

#define SECTOR_SIZE 512

extern int open_bsg_dev(char *in_file);

extern void setup_sgv4_hdr(struct sg_io_v4 *hdr, unsigned char *scb, int scb_len,
			   unsigned char *sense, int sense_len,
			   char *rbuf, int rlen, char *wbuf, int wlen);

extern void setup_rw_scb(unsigned char *scb, int scb_len, unsigned char cmd,
			 unsigned long len, unsigned long offset);
#endif
