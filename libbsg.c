#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

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
