#include "fat.h"
#include "kmalloc.h"
#include "string.h"
#include "printk.h"
#include <stdint-gcc.h>

typedef struct FAT_BPB {
    uint8_t jmp[3];
    char oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t num_dirents;
    uint16_t tot_sectors;
    uint8_t mdt;
    uint16_t num_sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t num_hidden_sectors;
    uint32_t large_sector_count;
} __attribute__((packed)) FAT_BPB_t;

typedef struct FAT32 {
    FAT_BPB_t FAT_BPB;
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint8_t major_vers;
    uint8_t minor_vers;
    uint32_t root_cluster_number;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t zero[12];
    uint8_t drive_num;
    uint8_t nt_flags;
    uint8_t signature;
    uint32_t serial_num;
    char label[11];
    char sys_id[8];
    uint8_t boot_code[420];
    uint8_t boot_sig[2];
} __attribute__((packed)) FAT32_t;

// Directories are an array of dir ents in a cluster
typedef struct FAT_dir_ent {
    char name[11];
    uint8_t attr;
    uint8_t nt;
    uint8_t ct_tenths;
    uint16_t ct;
    uint16_t cd;
    uint16_t ad;
    uint16_t cluster_hi;
    uint16_t mt;
    uint16_t md;
    uint16_t cluster_lo;
    uint32_t size;
} __attribute__((packed)) FAT_dir_ent_t;

static inline int cluster_to_sector(FAT32_t *fat32, int cluster_num) {
    int sector_offset = fat32->FAT_BPB.reserved_sectors + fat32->sectors_per_fat * fat32->FAT_BPB.num_fats;
    return sector_offset + (cluster_num - 2);
}

void print_dir_ent(FAT_dir_ent_t *dir_ent) {
    printk("%s\n", dir_ent->name);
}

int FAT_setup(part_block_dev_t *part) {
    FAT32_t fat32;
    int root_sector;
    uint8_t block[512];
    FAT_dir_ent_t *dir_ent;

    part->ata.dev.read_block((block_dev_t *)part, 0, &fat32);

    // Validate signature
    if (fat32.signature != 0x28 && fat32.signature != 0x29) {
        printk("FAT_setup(): Failed to validate FAT signature\n");
        return -1;
    }

    // Get root of fat32
    root_sector = cluster_to_sector(&fat32, fat32.root_cluster_number);
    part->ata.dev.read_block((block_dev_t *)part, root_sector, block);

    // Print directory entries in root sector
    dir_ent = (FAT_dir_ent_t *)block;
    while (dir_ent->name[0] != 0) {
        if (dir_ent->name[0] == 0xE5) continue;
        print_dir_ent(dir_ent);
        dir_ent++;
    }

    return 1;

}