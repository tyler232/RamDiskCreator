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
#include <ctype.h>

#define DEFAULT_RAMDISK_SIZE (16 * 1024 * 1024) // 16 MB
#define DEFAULT_RAMDISK_IMAGE_PATH "/var/tmp/ramdisk.img"

size_t parse_size_with_unit(const char *size_str) {
    size_t size = 0;
    char unit = 0;

    while (*size_str && isdigit(*size_str)) {
        size = size * 10 + (*size_str - '0');
        size_str++;
    }

    if (*size_str) {
        unit = tolower(*size_str);
    }

    switch (unit) {
        case 'k': // KB
            size *= 1024;
            break;
        case 'm': // MB
            size *= 1024 * 1024;
            break;
        case 'g': // GB
            size *= 1024 * 1024 * 1024;
            break;
        case 0: // no unit specified, assume bytes
            break;
        default:
            fprintf(stderr, "Invalid unit specified. Use K, M, or G.\n");
            return 0;
    }

    return size;
}

void print_help(const char *program_name) {
    printf("Usage: %s [-s size] [-h]\n", program_name);
    printf("Options:\n");
    printf("  -s, --size   Set the RAM disk size (e.g., 16M, 1G)\n");
    printf("  -p, --path   Set the path for the RAM disk image (default: /var/tmp/ramdisk.img)\n");
    printf("  -h, --help   Display this help message\n");
}

int main(int argc, char *argv[]) {
    size_t ramdisk_size = DEFAULT_RAMDISK_SIZE;
    const char *ramdisk_image_path = DEFAULT_RAMDISK_IMAGE_PATH;
    
    static struct option long_options[] = {
        {"size", required_argument, NULL, 's'},
        {"path", required_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "s:p:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 's': // Custom RAM disk size
                ramdisk_size = parse_size_with_unit(optarg);
                if (ramdisk_size == 0) {
                    fprintf(stderr, "Invalid RAM disk size specified.\n");
                    return 1;
                }
                break;
            case 'p': // Custom RAM disk image path
                ramdisk_image_path = optarg;
                break;
            case 'h': // print help
                print_help(argv[0]);
                return 0;
            default:
                fprintf(stderr, "Invalid option: %c\n", opt);
                print_help(argv[0]);
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

    printf("Creating RAM disk of size %zu\n", ramdisk_size);

    // Create ramdisk
    int fd = open(ramdisk_image_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
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

    printf("Formatting ramdisk with command: mkfs.ext4 %s\n", ramdisk_image_path);
    char format_cmd[256];
    snprintf(format_cmd, sizeof(format_cmd), "mkfs.ext4 %s", ramdisk_image_path);
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
