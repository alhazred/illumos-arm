#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <libfdisk.h>
#include <sys/efi_partition.h>
#include <sys/vtoc.h>

#ifndef BC_ERROR
#define	BC_ERROR	 (1)
#endif
struct alpha_bb {
	uint64_t data[63];
	uint64_t sum;
};

static void
usage(char *progname)
{
	(void) fprintf(stdout, "Usage: %s bootblk raw-device\n", progname);
}

static void
update(const char *boot_dev, void *buf, size_t numsect)
{
	int devfd = open(boot_dev, O_RDONLY);
	if (devfd < 0) {
		fprintf(stderr, "open error %s\n", boot_dev);
		exit(BC_ERROR);
	}
	struct mboot *bootblk = buf;
	uint64_t *bb = buf;
	if (read(devfd, bootblk, sizeof(*bootblk)) != sizeof(*bootblk)) {
		fprintf(stderr, "read boot blk error %s\n", boot_dev);
		exit(BC_ERROR);
	}
	if (bootblk->signature != MBB_MAGIC) {
		fprintf(stderr, "not found boot partition %s\n", boot_dev);
		exit(BC_ERROR);
	}
	struct ipart *iparts = (struct ipart *)bootblk->parts;

	bb[60] = numsect;
	bb[62] = 0;
	int i;
	for (i = 0; i < FD_NUMPART; i++) {
		if (iparts[i].systid == UNUSED)
			continue;
		if (iparts[i].systid == ALPHABOOT) {
			if (iparts[i].numsect < numsect) {
				fprintf(stderr, "boot partition size is too small %s\n", boot_dev);
				exit(BC_ERROR);
			}
			bb[61] = iparts[i].relsect;
			break;
		}
		if (iparts[i].systid == EFI_PMBR) {
			struct dk_gpt	*efip;
			if (efi_alloc_and_read(devfd, &efip) < 0) {
				fprintf(stderr, "not found boot partition %s\n", boot_dev);
				exit(BC_ERROR);
			}
			int j;
			for (j = 0;  j < efip->efi_nparts; j++) {
				struct dk_part *part = &efip->efi_parts[j];
				if (part->p_tag == V_BIOS_BOOT) {
					if (part->p_size < numsect) {
						fprintf(stderr, "boot partition size is too small %s\n", boot_dev);
						exit(BC_ERROR);
					}
					bb[61] = part->p_start;
					break;
				}
			}
			if (j == efip->efi_nparts) {
				fprintf(stderr, "not found boot partition %s\n", boot_dev);
				exit(BC_ERROR);
			}
		}
	}
	if (i == FD_NUMPART) {
		fprintf(stderr, "not found boot partition %s\n", boot_dev);
		exit(BC_ERROR);
	}
	close(devfd);

	bb[63] = 0;
	for (i = 0; i < 63; i++) {
		bb[63] += bb[i];
	}
}

int main(int argc, char **argv)
{
	struct alpha_bb bb = {0};
	struct stat stat_buf;
	int i;

	if (argc != 3) {
		usage(argv[0]);
		exit(BC_ERROR);
	}

	const char *boot_file = argv[1];
	const char *boot_dev = argv[2];

	if (stat(boot_file, &stat_buf) != 0) {
		fprintf(stderr, "stat error %s\n", boot_file);
		usage(argv[0]);
		exit(BC_ERROR);
	}

	update(boot_dev, &bb, ((stat_buf.st_size + 0x1FF) & ~0x1FF) / 0x200);

	int devfd = open(boot_dev, O_WRONLY | O_NDELAY);
	if (devfd < 0) {
		fprintf(stderr, "open error %s\n", boot_dev);
		usage(argv[0]);
		exit(BC_ERROR);
	}
	if (write(devfd, &bb, sizeof(bb)) != sizeof(bb)) {
		fprintf(stderr, "write error %s\n", boot_dev);
		usage(argv[0]);
		exit(BC_ERROR);
	}
	if (lseek(devfd, bb.data[61] * 0x200, SEEK_SET) != bb.data[61] * 0x200) {
		fprintf(stderr, "seek error %s\n", boot_dev);
		usage(argv[0]);
		exit(BC_ERROR);
	}

	int fd = open(boot_file, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open error %s\n", boot_file);
		usage(argv[0]);
		exit(BC_ERROR);
	}

	for (;;) {
		char buf[0x200] = {0};
		ssize_t sz = read(fd, buf, sizeof(buf));
		if (sz < 0) {
			fprintf(stderr, "read error %s\n", boot_file);
			usage(argv[0]);
			exit(BC_ERROR);
		}
		if (sz == 0)
			break;
		if (write(devfd, buf, 0x200) != 0x200) {
			fprintf(stderr, "write error %s\n", boot_dev);
			usage(argv[0]);
			exit(BC_ERROR);
		}
	}
	close(devfd);
	close(fd);

	return 0;
}
