#ifndef INSTRUCTIONS_H_INCLUDED
#define INSTRUCTIONS_H_INCLUDED

#include "hash_table.h"

#include <stdint.h>

hash_table_t ins_callbacks;

void register_instructions();

#endif // INSTRUCTIONS_H_INCLUDED
