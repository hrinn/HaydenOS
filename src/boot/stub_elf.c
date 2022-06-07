#include "stub_elf.h"
#include <stddef.h>

#define BITSIZE_64 2
#define LITTLE_ENDIAN 1
#define VERSION 1
#define X86_64 0
#define EXECUTABLE 2

typedef struct ELF_common_header {
    uint8_t magic[4];
    uint8_t bitsize;
    uint8_t endian;
    uint8_t version;
    uint8_t abi;
    uint64_t padding;
    uint16_t exe_type;
    uint16_t isa;
    uint32_t elf_version;
} ELF_common_header_t;

typedef struct ELF64_header {
    ELF_common_header_t common;
    uint64_t prog_entry_pos;
    uint64_t prog_table_pos;
    uint64_t sec_table_pos;
    uint32_t flags;
    uint16_t hdr_size;
    uint16_t prog_ent_size;
    uint16_t prog_ent_num;
    uint16_t sec_ent_size;
    uint16_t sec_ent_num;
    uint16_t sec_name_idx;
} ELF64_header_t;

typedef struct ELF64_prog_header {
    uint32_t type;
    uint32_t flags;
    uint64_t file_offset;
    uint64_t load_addr;
    uint64_t undefined;
    uint64_t file_size;
    uint64_t mem_size;
    uint64_t alignment;
} ELF64_prog_header_t;

void *memcpy(void *dest, const void *src, size_t n) {
    size_t i;

    uint8_t *d = (uint8_t *)dest;
    uint8_t *s = (uint8_t *)src;

    if (dest == NULL) return NULL;
    if (src == NULL) return dest;

    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

int strncmp( const char * s1, const char * s2, size_t n )
{
    while ( n && *s1 && ( *s1 == *s2 ) )
    {
        ++s1;
        ++s2;
        --n;
    }
    if ( n == 0 )
    {
        return 0;
    }
    else
    {
        return ( *(unsigned char *)s1 - *(unsigned char *)s2 );
    }
}

uint64_t ELF_mmap_module(uint64_t start, uint64_t end) {
    ELF64_header_t *header;
    ELF64_prog_header_t *prog_header;
    char *magic = "\177ELF";

    header = (ELF64_header_t *)start;

    if (strncmp((char *)header->common.magic, magic, 4) != 0 ||
        header->common.bitsize != BITSIZE_64 ||
        header->common.endian != LITTLE_ENDIAN ||
        header->common.version != VERSION ||
        header->common.abi != X86_64 ||
        header->common.exe_type != EXECUTABLE) {
        return 0;
    }

    for (prog_header = (ELF64_prog_header_t *)(start + header->prog_table_pos);
        prog_header < (ELF64_prog_header_t *)(start + header->prog_table_pos + header->prog_ent_num * header->prog_ent_size);
        prog_header++)
    {

    }

    return header->prog_entry_pos;
}