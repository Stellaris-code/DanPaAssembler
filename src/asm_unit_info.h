#ifndef ASM_UNIT_INFO_H_INCLUDED
#define ASM_UNIT_INFO_H_INCLUDED

#include "hash_table.h"
#include "dynarray.h"

typedef struct reloc_pair_t
{
    size_t reloc_index;
    const char* target_label;
} reloc_pair_t;

typedef struct string_constant_t
{
    unsigned int id;
    const char* str;
    uint16_t len;
} string_constant_t;

typedef struct asm_unit_t
{
    const char* source;
    hash_table_t labels;
    DYNARRAY(reloc_pair_t) relocs;
    DYNARRAY(uint8_t) object_buffer;
    DYNARRAY(string_constant_t) strings;
} asm_unit_t;

#endif // ASM_UNIT_INFO_H_INCLUDED
