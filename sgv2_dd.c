#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <arpa/inet.h>

static char pname[] = "sgv2_dd";

static struct option const long_options[] =
{
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0},
};

static void usage(int status)
{
	if (status)
		fprintf(stderr, "Try `%s --help' for more information.\n",
			pname);
	else {
		printf("Usage: %s [OPTIONS]... <sg device>\n", pname);
		printf("\
  -h, --help              display this help and exit\n\
");
		printf("\n\
Examples:\n\
  $ %s /dev/sg1\n\
", pname);
	}
	exit(status);
}

#define SECTOR_SIZE 512

static void setup_rw_scb10(unsigned char *scb, int scb_len, unsigned char cmd,
			   unsigned long len, unsigned long offset)
{
	memset(scb, 0, scb_len);

	scb[0] = cmd;
	scb[7] = (unsigned char)(((len / SECTOR_SIZE) >> 8) & 0xff);
	scb[8] = (unsigned char)((len / SECTOR_SIZE) & 0xff);

	*((uint32_t *) &scb[2]) = htonl(offset / SECTOR_SIZE);
}

static int sgv2_read(int fd, char *p, int len, unsigned offset)
{
	unsigned char *cmd;
	struct sg_header *hdr;
	unsigned char *buf;
	int cmd_len = 10;
	int total_len = len + sizeof(*hdr) + cmd_len;
	int ret;

	printf("%s %d len %d off %u\n", __func__, __LINE__, len, offset);

	buf = malloc(total_len);
	memset(buf, 0, total_len);

	hdr = (struct sg_header *)buf;
	cmd = buf + sizeof(*hdr);

	setup_rw_scb10(cmd, cmd_len, READ_10, len, offset);

	hdr->reply_len = sizeof(*hdr) + len;

	ret = write(fd, hdr, cmd_len + sizeof(*hdr));
	if (ret != sizeof(*hdr) + cmd_len) {
		printf("%s %d fail, %d\n", __func__, __LINE__, ret);
		return ret;
	}

	memset(hdr, 0, sizeof(*hdr));

	ret = read(fd, buf, sizeof(*hdr) + len);
	if (ret <= 0) {
		printf("%s %d fail, %d\n", __func__, __LINE__, ret);
		return ret;
	}
	if (hdr->result) {
		printf("%s %d fail, %d\n", __func__, __LINE__, hdr->result);
		return ret;
	}

	memcpy(p, buf + sizeof(*hdr), len);

	free(buf);

	return 0;
}

static int sgv2_write(int fd, char *p, int len, unsigned offset)
{
	unsigned char *cmd;
	struct sg_header *hdr;
	unsigned char *buf;
	int cmd_len = 10;
	int total_len = len + sizeof(*hdr) + cmd_len;
	int ret;

	printf("%s %d len %d off %u\n", __func__, __LINE__, len, offset);

	buf = malloc(total_len);
	memset(buf, 0, total_len);

	hdr = (struct sg_header *)buf;
	cmd = buf + sizeof(*hdr);

	setup_rw_scb10(cmd, cmd_len, WRITE_10, len, offset);

	hdr->reply_len = sizeof(*hdr);

	memcpy(buf + sizeof(*hdr) + cmd_len, p, len);

	ret = write(fd, hdr, total_len);
	if (ret != total_len) {
		printf("%s %d fail, %d\n", __func__, __LINE__, ret);
		return ret;
	}

	memset(hdr, 0, sizeof(*hdr));

	ret = read(fd, buf, sizeof(*hdr));
	if (ret <= 0) {
		printf("%s %d fail, %d\n", __func__, __LINE__, ret);
		return ret;
	}
	if (hdr->result) {
		printf("%s %d fail, %d\n", __func__, __LINE__, hdr->result);
		return hdr->result;
	}

	free(buf);

	return 0;
}

int main(int argc, char **argv)
{
	int longindex, ch;
	int in_fd, of_fd, i;
	char *p;
	char *if_file, *of_file;
	int count, bs;
	int ret = -EINVAL;
	void *buf;
	unsigned if_offset, of_offset;
	int if_sg, of_sg;

	while ((ch = getopt_long(argc, argv, "h", long_options,
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

	if_sg = !!strstr(if_file, "/dev/sg");
	of_sg = !!strstr(of_file, "/dev/sg");

	printf("%d %d\n", if_sg, of_sg);

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

	buf = malloc(bs);
	if (!buf) {
		ret = -ENOMEM;
		goto out;
	}

	in_fd = open(if_file, O_RDWR);
	if (in_fd < 0) {
		printf("can't open if, %s %s\n", strerror(errno), if_file);
		goto out;
	}

	of_fd = open(of_file, O_RDWR |O_CREAT);
	if (of_fd < 0) {
		printf("can't open of, %s %s\n", strerror(errno), of_file);
		goto out;
	}

	for (i = 0; i < count; i++) {
		if (if_sg) {
			ret = sgv2_read(in_fd, buf, bs, if_offset);
			if (ret)
				break;
		} else {
			ret = pread(in_fd, buf, bs, if_offset);
			if (ret != bs) {
				printf("%s %d: %d\n", __func__, __LINE__, ret);
				goto out;
			}
		}

		if (of_sg) {
			ret = sgv2_write(of_fd, buf, bs, of_offset);
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

	free(buf);

out:
	return ret;
}
