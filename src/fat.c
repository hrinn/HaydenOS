#include "fat.h"
#include "kmalloc.h"
#include "string.h"
#include "printk.h"
#include "memdef.h"
#include <stdbool.h>
#include <stdint-gcc.h>

#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LFN (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)

#define MAX_DE_LEN 11
#define MAX_LDE_LEN 255
#define DIR_ENTS_PER_SECTOR 16
#define TABLE_VAL_MAX 0x0FFFFFF8

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

typedef struct FAT_superblock {
    superblock_t superblock;
    FAT32_t fat32;
} FAT_superblock_t;

typedef struct FAT_inode {
    inode_t inode;
    uint8_t *data;
} FAT_inode_t;

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

int FAT_readdir(inode_t *inode, readdir_cb callback, void *p);

static inline int cluster_to_sector(FAT32_t *fat32, int cluster_num) {
    int sector_offset = fat32->FAT_BPB.reserved_sectors + fat32->sectors_per_fat * fat32->FAT_BPB.num_fats;
    return sector_offset + (cluster_num - 2);
}

void get_dir_ent_name(FAT_dir_ent_t *dir_ent, char *buffer) {
    strncpy(buffer, dir_ent->name, MAX_DE_LEN + 1);
}

// Takes a long directory entry and places its name at the correct position in the buffer
void get_long_dir_ent_name(FAT_long_dir_ent_t *dir_ent, char *buffer) {
    int i, num;

    // Position character pointer at beginning of this string portion
    num = (dir_ent->order & 0x3F) - 1;
    buffer += num * 13;

    for (i = 0; i < 5; i++) {
        buffer[i] = dir_ent->first[i] & 0xFF;
    }

    for (i = 5; i < 11; i++) {
        buffer[i] = dir_ent->middle[i - 5] & 0xFF;
    }

    for (i = 11; i < 13; i++) {
        buffer[i] = dir_ent->last[i - 11] & 0xFF;
    }
}

void FAT_inode_free(inode_t **inode) {
    FAT_inode_t *FAT_inode = (FAT_inode_t *)*inode;
    if (FAT_inode->data != NULL) {
        kfree(FAT_inode->data);
    }
    kfree(FAT_inode);
}

int FAT_read_cluster(superblock_t *sb, unsigned long cluster_num, uint8_t *buffer) {
    FAT_superblock_t *FAT_sb = (FAT_superblock_t *)sb;
    block_dev_t *dev = FAT_sb->superblock.dev;
    int sector_num = cluster_to_sector(&FAT_sb->fat32, cluster_num);
    if (dev->read_block(dev, sector_num, buffer) == -1) {
        printk("FAT_read_cluster(): Failed to read cluster %lu of block device\n", cluster_num);
        return -1;
    }
    return 1;
}

FAT_inode_t *FAT_init_inode(superblock_t *sb, unsigned long cluster_num) {
    FAT_inode_t *inode = (FAT_inode_t *)kcalloc(1, sizeof(FAT_inode_t));
    
    // Set inode fields
    inode->inode.st_ino = cluster_num;
    inode->inode.parent_superblock = sb;

    // Set inode methods
    inode->inode.readdir = VSPACE(FAT_readdir);
    inode->inode.free = VSPACE(FAT_inode_free);

    return inode;
}

// Returns an allocated inode associated with the cluster number
inode_t *FAT_read_inode(superblock_t *sb, unsigned long cluster_num) {
    FAT_inode_t *inode = FAT_init_inode(sb, cluster_num);
    inode->data = (uint8_t *)kmalloc(512);

    if (FAT_read_cluster(sb, cluster_num, inode->data) == -1) {
        return NULL;
    }

    return (inode_t *)inode;
}

uint32_t get_next_cluster_num(FAT_superblock_t *sb, uint32_t current_cluster_num) {
    // Get FAT associated with inode
    uint8_t FAT[512];
    block_dev_t *dev = sb->superblock.dev;
    int first_fat = sb->fat32.FAT_BPB.reserved_sectors;
    uint32_t table_val;
    
    uint32_t fat_offset = current_cluster_num * 4;
    uint32_t fat_sector = first_fat + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;

    dev->read_block(dev, fat_sector, FAT);

    table_val = *(uint32_t *)&FAT[ent_offset];
    return table_val & 0x0FFFFFFF;
}

// Takes a directory inode
// Calls the callback function on each entry associated with directory
int FAT_readdir(inode_t *inode, readdir_cb callback, void *p) {
    uint64_t cluster_num, ent_cluster_num;
    uint8_t *data;
    FAT_dir_ent_t *dir_ent;
    char name[MAX_LDE_LEN + 1];
    bool valid_classic_entry = true;
    inode_t *ent_inode;

    if ((inode->st_mode & S_IFDIR) == 0) {
        printk("FAT_readdir(): Attempted to read non directory\n");
        return -1;
    }

    data = (uint8_t *)kmalloc(512);

    for (cluster_num = inode->st_ino; cluster_num < TABLE_VAL_MAX; 
        cluster_num = get_next_cluster_num((FAT_superblock_t *)inode->parent_superblock, cluster_num))
    {
        // Read data associated with this cluster
        FAT_read_cluster((superblock_t *)inode->parent_superblock, cluster_num, data);

        dir_ent = (FAT_dir_ent_t *)data;

        while (dir_ent->name[0] != 0 && (uint8_t *)dir_ent < data + 512) {
            if (dir_ent->attr == FAT_ATTR_LFN) {
                // Long directory entries
                get_long_dir_ent_name((FAT_long_dir_ent_t *)dir_ent, name);
                valid_classic_entry = false;
            } else {
                // Normal directory entry
                if (valid_classic_entry) {
                    get_dir_ent_name(dir_ent, name);
                } else {
                    valid_classic_entry = true;
                }

                if (name[0] != '.') {
                    // Create inode for directory entry
                    ent_cluster_num = (dir_ent->cluster_hi << 16) | dir_ent->cluster_lo;
                    ent_inode = (inode_t *)FAT_init_inode(inode->parent_superblock, ent_cluster_num);
                    ent_inode->st_size = dir_ent->size;
                    ent_inode->parent_inode = inode;
                    if (dir_ent->attr == FAT_ATTR_DIRECTORY) ent_inode->st_mode |= S_IFDIR;
                    // TODO: set creation, mod times

                    callback(name, ent_inode, p);
                }
            }
            
            dir_ent++;
        }
    }

    kfree(data);
    return 1;
}

superblock_t *FAT_detect(block_dev_t *dev) {
    FAT_superblock_t *superblock = (FAT_superblock_t *)kmalloc(sizeof(FAT_superblock_t));

    dev->read_block(dev, 0, &superblock->fat32);

    if (superblock->fat32.signature != 0x28 && superblock->fat32.signature != 0x29) {
        printk("FAT_detect(): Failed to validate FAT signature\n");
        kfree(superblock);
        return NULL;
    }

    // Valid FAT32 FS, setup superblock
    printk("Detected FAT32 filesystem\n");
    superblock->superblock.type = "FAT32";
    superblock->superblock.name = superblock->fat32.label;
    superblock->superblock.dev = dev;
    superblock->superblock.read_inode = VSPACE(FAT_read_inode);

    // Setup root inode
    superblock->superblock.root_inode = (inode_t *)FAT_init_inode((superblock_t *)superblock, superblock->fat32.root_cluster_number);
    superblock->superblock.root_inode->st_mode |= S_IFDIR;


    return (superblock_t *)superblock;
}