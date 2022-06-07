#ifndef ELF_H
#define ELF_H

#include "memdef.h"
#include "vfs.h"

virtual_addr_t ELF_mmap_file(inode_t *root, char *path);

#endif