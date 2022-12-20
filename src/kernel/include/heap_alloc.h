#ifndef MMU_H
#define MMU_H

#include "memdef.h"

void *MMU_alloc_page();
void *MMU_alloc_pages(int num);
void MMU_free_page(void *);
void MMU_free_pages(void *, int num);

#endif