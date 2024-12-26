#include "mbr.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <sys/statvfs.h>

#define DEFAULT_RAMDISK_SIZE (16 * 1024 * 1024) // 16 MB
#define RAMDISK_IMAGE_PATH "/var/tmp/ramdisk.img"

int main(int argc, char *argv[]) {
    size_t ramdisk_size = DEFAULT_RAMDISK_SIZE;

    int opt;
    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
            case 's': // Custom RAM disk size
                ramdisk_size = strtoull(optarg, NULL, 10) * 1024 * 1024; // Convert MB to bytes
                if (ramdisk_size == 0) {
                    fprintf(stderr, "Invalid RAM disk size specified.\n");
                    return 1;
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-s size_in_MB]\n", argv[0]);
                return 1;
        }
    }

    // check if requested RAM disk size exceeds available memory
    long available_memory = sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
    if (ramdisk_size > (size_t)available_memory) {
        fprintf(stderr, "Error: Not enough memory to create a ramdisk of size %zu MB.\n", ramdisk_size / (1024 * 1024));
        return 1;
    }

    // check if requested disk image size exceeds available disk space
    struct statvfs stat;
    if (statvfs("/var/tmp", &stat) != 0) {
        perror("Failed to get filesystem stats for /var/tmp");
        return 1;
    }
    long available_disk_space = stat.f_bsize * stat.f_bavail;
    if (ramdisk_size > (size_t)available_disk_space) {
        fprintf(stderr, "Error: Not enough disk space for a ramdisk of size %zu MB.\n", ramdisk_size / (1024 * 1024));
        return 1;
    }

    printf("Creating RAM disk of size %zu MB\n", ramdisk_size / (1024 * 1024));

    // Create ramdisk
    int fd = open(RAMDISK_IMAGE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create ramdisk");
        return 1;
    }

    if (ftruncate(fd, ramdisk_size) != 0) {
        perror("Failed to set ramdisk size");
        close(fd);
        return 1;
    }

    char *ramdisk = mmap(NULL, ramdisk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ramdisk == MAP_FAILED) {
        perror("Failed to mmap ramdisk");
        close(fd);
        return 1;
    }

    create_partition_table(ramdisk);

    // Cleanup memory mapping
    munmap(ramdisk, ramdisk_size);
    close(fd);

    printf("Ramdisk and partition table created successfully.\n");

    printf("Formatting ramdisk with command: mkfs.ext4 %s\n", RAMDISK_IMAGE_PATH);
    char format_cmd[256];
    snprintf(format_cmd, sizeof(format_cmd), "mkfs.ext4 %s", RAMDISK_IMAGE_PATH);
    system(format_cmd);

    printf("Associating ramdisk with loop device.\n");
    system("losetup /dev/loop0 /var/tmp/ramdisk.img");

    printf("Mounting ramdisk to /mnt/ramdisk.\n");
    system("mount /dev/loop0 /mnt/ramdisk");

    printf("Setting permissions on /mnt/ramdisk.\n");
    system("chmod 777 /mnt/ramdisk");

    printf("Ramdisk mounted successfully.\n");

    return 0;
}
