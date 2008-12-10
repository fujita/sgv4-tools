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

static char pname[] = "sgv2_inq";

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

static void sgv2_inq_cmd(int fd)
{
	unsigned char *cmd;
	struct sg_header *hdr;
	int cmd_len = 6;
	unsigned char buf[256 + sizeof(*hdr)];
	int ret;

	memset(buf, 0, sizeof(buf));

	hdr = (struct sg_header *)buf;
	cmd = buf + sizeof(*hdr);
	cmd[0] = 0x12;
	cmd[4] = 64;

	hdr->reply_len = sizeof(buf);

	ret = write(fd, hdr, sizeof(*hdr) + cmd_len);
	if (ret != sizeof(*hdr) + cmd_len) {
		printf("%d: %d\n", __LINE__, ret);
		exit(1);
	}

	ret = read(fd, buf, sizeof(buf));
	if (ret <= 0) {
		printf("%d: %d\n", __LINE__, ret);
		exit(1);
	}
	if (hdr->result) {
		printf("failed %d\n", hdr->result);
		exit(1);
	}

	printf("%d: %s\n", __LINE__, buf + sizeof(*hdr) + 8);
}

int main(int argc, char **argv)
{
	int longindex, ch;
	int fd;

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

	fd = open(argv[optind], O_RDWR);
	if (fd < 0) {
		printf("can't open %s\n", argv[optind]);
		exit(1);
	}

	sgv2_inq_cmd(fd);

	return 0;
}
