#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define RAMDISK_SIZE (16 * 1024 * 1024) // 16 MB
#define SECTOR_SIZE 512
#define PARTITION1_START 2048          // offset in sectors
#define PARTITION1_SIZE (8 * 1024)     // size in sectors (4 MB)

typedef struct {
    char boot_code[446];
    struct {
        unsigned char boot_flag;   // (0x80 = active, 0x00 = inactive)
        unsigned char start_chs[3]; // start CHS (Cylinder-Head-Sector) address
        unsigned char partition_type;
        unsigned char end_chs[3];  // end CHS address
        unsigned int start_lba;   // start LBA
        unsigned int size;        // size in sectors
    } partition_entry[4];
    unsigned short boot_signature; // Boot signature (0xAA55)
} __attribute__((packed)) MBR;

void create_partition_table(char *ramdisk) {
    MBR *mbr = (MBR *)ramdisk;

    // Clear MBR
    memset(mbr, 0, SECTOR_SIZE);

    // Set up the first partition
    mbr->partition_entry[0].boot_flag = 0x80; // Active partition
    mbr->partition_entry[0].partition_type = 0x83; // Linux filesystem
    mbr->partition_entry[0].start_lba = PARTITION1_START;
    mbr->partition_entry[0].size = PARTITION1_SIZE;

    // Set boot signature
    mbr->boot_signature = 0xAA55;

    printf("Partition table created.\n");
}

int main() {
    const char *ramdisk_path = "/var/tmp/ramdisk.img";

    // Create ramdisk
    int fd = open(ramdisk_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create ramdisk");
        return 1;
    }

    if (ftruncate(fd, RAMDISK_SIZE) != 0) {
        perror("Failed to set ramdisk size");
        close(fd);
        return 1;
    }

    char *ramdisk = mmap(NULL, RAMDISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ramdisk == MAP_FAILED) {
        perror("Failed to mmap ramdisk");
        close(fd);
        return 1;
    }

    create_partition_table(ramdisk);

    // cleanup memory mapping
    munmap(ramdisk, RAMDISK_SIZE);
    close(fd);

    printf("Ramdisk and partition table created successfully.\n");

    printf("Formatting ramdisk with command: mkfs.ext4 %s\n", ramdisk_path);
    char format_cmd[256];
    snprintf(format_cmd, sizeof(format_cmd), "mkfs.ext4 %s", ramdisk_path);
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

