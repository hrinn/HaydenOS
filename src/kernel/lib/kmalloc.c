#include "kmalloc.h"
#include "mmu.h"
#include "string.h"
#include "debug.h"

#define NUM_POOLS 7
#define PAGE_OFFSET 12

typedef struct free_list {
    struct free_list *next;
} free_list_t;

typedef struct kmalloc_pool {
    const int block_size;
    int avail;
    struct free_list *head;
} kmalloc_pool_t;

typedef struct block_header {
    struct kmalloc_pool *pool;
    size_t size;
} block_header_t;

static kmalloc_pool_t pools[NUM_POOLS] = {
    {32, 0, NULL},
    {64, 0, NULL},
    {128, 0, NULL},
    {256, 0, NULL},
    {512, 0, NULL},
    {1024, 0, NULL},
    {2048, 0, NULL}
};

// Counts the number of pages required to fit (size) bytes
static int num_pages(size_t size) {
    return ((size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)) >> PAGE_OFFSET;
}

static void allocate_blocks(kmalloc_pool_t *pool) {
    int i;
    free_list_t *free_node;

    DEBUG_PRINT("Allocating %d blocks for %dB pool\n", PAGE_SIZE / pool->block_size, pool->block_size);

    // Allocate a page
    uint8_t *page = MMU_alloc_page();

    // Divide the page into PAGE_SIZE / block_size blocks
    // With a linked list node at beginning of each block
    for (i = 0; i < PAGE_SIZE; i += pool->block_size) {
        free_node = (free_list_t *)(page + i);

        if (i < PAGE_SIZE - pool->block_size) {
            free_node->next = (free_list_t *)(page + i + pool->block_size);
        } else {
            free_node->next = NULL;
        }
    }

    // Set head of linked list
    pool->head = (free_list_t *)page;

    // Set avail number
    pool->avail = PAGE_SIZE / pool->block_size;
}

static free_list_t *pop_free_block(kmalloc_pool_t *pool) {
    free_list_t *block = pool->head;
    pool->head = pool->head->next;
    pool->avail--;
    DEBUG_PRINT("Retrieving block at %p from %dB pool\n", (void *)block, pool->block_size);
    return block;
}

// Allocates memory on the kernel heap of (size) bytes
void *kmalloc(size_t size) {
    size_t full_size = size + sizeof(block_header_t);
    block_header_t *header;
    int i;

    // Determine adequate pool
    for (i = 0; i < NUM_POOLS; i++) {
        if (full_size <= pools[i].block_size) {
            // Adequate block size
            if (pools[i].avail == 0) {
                allocate_blocks(&pools[i]);
            }

            // Retrieve block and place header information
            header = (block_header_t *)pop_free_block(&pools[i]);
            header->pool = &pools[i];
            header->size = full_size;
            // Return memory address starting after header
            return (void *)(header + 1);
        }
    }

    // No adequate pool
    header = (block_header_t *)MMU_alloc_pages(num_pages(full_size));
    header->pool = NULL;
    header->size = full_size;
    DEBUG_PRINT("%ld too large, allocating %d pages starting at %p\n", full_size, num_pages(full_size), (void *)header);
    return (void *)(header + 1);
}

// Allocates memory on the kenrnel heap of (size * nmemb) bytes
// Initializes region to 0
void *kcalloc(size_t nmemb, size_t size) {
    void *addr = kmalloc(nmemb * size);
    memset(addr, 0, nmemb * size);
    return addr;
}

static void push_free_block(kmalloc_pool_t *pool, free_list_t *node) {
    node->next = pool->head;
    pool->head = node;
    pool->avail++;
    DEBUG_PRINT("Adding block at %p to %dB pool\n", (void *)node, pool->block_size);
}

// Frees memory allocated on the kernel heap
// Must free same address that was returned by kmalloc/calloc
void kfree(void *addr) {
    block_header_t *header;
    
    if (addr == NULL) return;

    header = ((block_header_t *)addr) - 1;

    if (header->pool != NULL) {
        // Return block to free list
        push_free_block(header->pool, (free_list_t *)header);
    } else {
        // Deallocate pages
        DEBUG_PRINT("Deallocating pages of len %ldB starting at %p\n", header->size, (void *)header);
        MMU_free_pages((void *)header, num_pages(header->size));
    }
}