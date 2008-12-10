#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include "libbsg.h"

static char pname[] = "sgv4_inq";
static int sgio;

static struct option const long_options[] =
{
	{"sgio", no_argument, 0, 's'},
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
  $ %s /sys/class/bsg/0:0:0:0\n\
", pname);
	}
	exit(status);
}

#define SECTOR_SIZE 512

static void sgv4_inq_cmd(int fd)
{
	struct sg_io_v4 hdr;
	unsigned char cmd[6];
	unsigned char sense[32];
	char buf[64];
	int ret;

	memset(buf, 0, sizeof(buf));
	memset(cmd, 0, sizeof(cmd));

	cmd[0] = 0x12;
	cmd[4] = 64;

	setup_sgv4_hdr(&hdr, cmd, sizeof(cmd), sense, sizeof(sense), buf, sizeof(buf),
		       NULL, 0);

	if (sgio) {
		ret = ioctl(fd, SG_IO, &hdr);
		if (ret) {
			fprintf(stderr, "fail to sgio, %m\n");
			exit(1);
		}
	} else {
		ret = write(fd, &hdr, sizeof(hdr));
		if (ret != sizeof(hdr)) {
			printf("%d: %d\n", __LINE__, ret);
			exit(1);
		}
		memset(&hdr, 0, sizeof(hdr));
		ret = read(fd, &hdr, sizeof(hdr));
		if (ret != sizeof(hdr)) {
			fprintf(stderr, "fail to wait for the response, %m\n");
			exit(1);
		}
	}

	printf("%d: %s\n", __LINE__, buf + 8);
}

int main(int argc, char **argv)
{
	int longindex, ch;
	int fd;

	while ((ch = getopt_long(argc, argv, "sh", long_options,
				 &longindex)) >= 0) {
		switch (ch) {
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

	fd = open_bsg_dev(argv[optind]);
	if (fd < 0) {
		printf("can't open %s\n", argv[optind]);
		exit(1);
	}

	sgv4_inq_cmd(fd);

	return 0;
}
