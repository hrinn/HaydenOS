#include "fat.h"
#include "kmalloc.h"
#include "string.h"
#include "printk.h"
#include <stdint-gcc.h>

#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LFN (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)

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
    inode_t root_inode;
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

typedef struct FAT_long_dir_ent {
    uint8_t order;
    uint16_t first[5];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint16_t middle[6];
    uint16_t zero;
    uint16_t last[2];
} __attribute__((packed)) FAT_long_dir_ent_t;

static inline int cluster_to_sector(FAT32_t *fat32, int cluster_num) {
    int sector_offset = fat32->FAT_BPB.reserved_sectors + fat32->sectors_per_fat * fat32->FAT_BPB.num_fats;
    return sector_offset + (cluster_num - 2);
}

void print_dir_ent(FAT_dir_ent_t *dir_ent) {
    char dir_name[12];
    memset(dir_name, 0, 12);
    strncpy(dir_name, dir_ent->name, 11);
    printk("%s\n", dir_name);
}

void print_long_dir_ent(FAT_long_dir_ent_t *long_dir_ent) {
    char dir_name[14];
    int i;
    memset(dir_name, 0, 14);

    for (i = 0; i < 5; i++) {
        dir_name[i] = long_dir_ent->first[i] & 0xFF;
    }

    for (i = 5; i < 11; i++) {
        dir_name[i] = long_dir_ent->middle[i - 5] & 0xFF;
    }

    for (i = 11; i < 13; i++) {
        dir_name[i] = long_dir_ent->middle[i - 11] & 0xFF;
    }

    printk("%s\n", dir_name);
}

void read_directory(block_dev_t *dev, FAT32_t *fat, int cluster_num) {
    uint8_t block[512];
    FAT_dir_ent_t *dir_ent;
    int sector = cluster_to_sector(fat, cluster_num);

    dev->read_block(dev, sector, block);

    printk("Directory at sector %d:\n", sector);

    // Print directory entries in root sector
    dir_ent = (FAT_dir_ent_t *)block;
    while (dir_ent->name[0] != 0) {
        if (dir_ent->name[0] != 0xE5) {
            if (dir_ent->attr == FAT_ATTR_LFN) {
                print_long_dir_ent((FAT_long_dir_ent_t *)dir_ent);
                dir_ent++;
            } else {
                print_dir_ent(dir_ent);
            }
        }
        dir_ent++;
    }
}

superblock_t *FAT_detect(block_dev_t *dev) {
    FAT32_t *fat = (FAT32_t *)kmalloc(sizeof(FAT32_t));
    superblock_t *superblock;

    dev->read_block(dev, 0, &fat->FAT_BPB);

    if (fat->signature != 0x28 && fat->signature != 0x29) {
        printk("FAT_detect(): Failed to validate FAT signature\n");
        kfree(fat);
        return NULL;
    }

    // Valid FAT32 FS
    superblock = (superblock_t *)kmalloc(sizeof(superblock_t));
    superblock->type = "FAT32";
    superblock->name = fat->label;
    superblock->root_inode = (inode_t *)fat;
    // TODO: set function pointers

    return superblock;
}