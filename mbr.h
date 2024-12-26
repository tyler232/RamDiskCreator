#ifndef MBR_H
#define MBR_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SECTOR_SIZE 512
#define PARTITION1_START 2048          // offset in sectors
#define PARTITION1_SIZE (8 * 1024)     // size in sectors (4 MB)

typedef struct {
    char boot_code[446];
    struct {
        uint8_t boot_flag;   // (0x80 = active, 0x00 = inactive)
        uint8_t start_chs[3]; // start CHS (Cylinder-Head-Sector) address
        uint8_t partition_type;
        uint8_t end_chs[3];  // end CHS address
        uint32_t start_lba;   // start LBA
        uint32_t size;        // size in sectors
    } partition_entry[4];
    uint16_t boot_signature; // Boot signature (0xAA55)
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

#endif // MBR_H
