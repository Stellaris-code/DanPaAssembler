#include "parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hash_table.h"
#include "instructions.h"

static const char* source_ptr;
static const char* start_of_line;
static int current_line = 1;

uint8_t buffer[4096];

const char* parse_label()
{
    const char* start = source_ptr;
    while (isalnum(*source_ptr) || *source_ptr == '.' || *source_ptr == '_')
        ++source_ptr;
    if (*source_ptr == ':') // it's a label !
    {
        char* label_str = malloc(source_ptr - start + 1);
        memcpy(label_str, start, source_ptr - start);
        label_str[source_ptr - start] = '\0';

        ++source_ptr;

        return label_str;
    }
    else
    {
        // revert
        source_ptr = start;
        return NULL;
    }
}

const char* parse_opcode()
{
    const char* start = source_ptr;

    while (isalnum(*source_ptr))
        ++source_ptr;

    if (start == source_ptr) // no opcode
    {
        abort();
    }

    char* opcode_str = malloc(source_ptr - start + 1);
    memcpy(opcode_str, start, source_ptr - start);
    opcode_str[source_ptr - start] = '\0';

    return opcode_str;
}

const char* parse_operand()
{
    const char* start = source_ptr;

    while (isalnum(*source_ptr) || *source_ptr == '#' || *source_ptr == '.' || *source_ptr == '_'
           || *source_ptr == '-' || *source_ptr == '+')
        ++source_ptr;

    if (start == source_ptr) // no opcode
    {
        return NULL;
    }

    char* operand_str = malloc(source_ptr - start + 1);
    memcpy(operand_str, start, source_ptr - start);
    operand_str[source_ptr - start] = '\0';

    return operand_str;
}

void consume_whitespace()
{
    while (isblank(*source_ptr))
    {
        ++source_ptr;
    }
}

void consume_comments()
{
    if (source_ptr[0] != '/' || source_ptr[1] != '/')
        return;

    while (*source_ptr && *source_ptr != '\n')
        ++source_ptr;
}

// assumes we've already consumed the first '"'
const char* end_of_string_lit(const char* str)
{
    while (*str)
    {
        if (str[0] == '"' && str[-1] != '\\')
            return str;
        ++str;
    }

    return NULL;
}

const char* parse_string_literal(const char* ptr, char** buf_ptr, int* len)
{
    if (*ptr != '"')
        return NULL;

    ++ptr;
    const char* literal_start = ptr;
    const char* literal_end   = end_of_string_lit(ptr);
    if (literal_end == NULL)
        return NULL;
    int literal_len = literal_end - literal_start;

    char* loc_buf_ptr = (char*)malloc(literal_len+1);
    *buf_ptr = loc_buf_ptr;
    fprintf(stderr, "parsing literal '%s'\n", literal_start);

    for (int i = 0; i < literal_len; ++i)
    {
        // escape
        if (literal_start[i] == '\\')
        {
            ++i;
            if (i == literal_len)
                abort();

            switch (literal_start[i])
            {
                case 'n':
                    *loc_buf_ptr++ = '\n';
                    break;
                case '\\':
                    *loc_buf_ptr++ = '\\';
                    break;
                case '"':
                    *loc_buf_ptr++ = '"';
                    break;
                default:
                    fprintf(stderr, "unrecognized escape character : '%c'\n", literal_start[i]);
            }
        }
        else
        {
            *loc_buf_ptr++ = literal_start[i];
        }
    }
    *loc_buf_ptr = '\0';

    ptr = literal_end + 1;

    *len = loc_buf_ptr - *buf_ptr;

    return ptr;
}

void parse_string_directive(asm_unit_t* unit)
{
    source_ptr += 7; // skip ".string"
    consume_whitespace();

    char* endptr;
    errno = 0;
    int string_id = strtol(source_ptr, &endptr, 0);
    if (endptr == source_ptr)
        abort();
    if (errno != 0)
        abort();
    source_ptr = endptr;

    consume_whitespace();
    if (*source_ptr++ != ',')
        abort();
    consume_whitespace();

    char* string_contents;
    int len;
    source_ptr = parse_string_literal(source_ptr, &string_contents, &len);
    if (source_ptr == NULL)
        abort();

    if (len >= 0x10000)
        printf("warning : string literal is too large (length doesn't fit in 16-bit)\n");

    consume_whitespace();

    string_constant_t str_entry;
    str_entry.id = string_id;
    str_entry.str = string_contents;
    str_entry.len = len;

    DYNARRAY_ADD(unit->strings, str_entry);
}

int parse_directive(asm_unit_t* unit)
{
    if (strncmp(source_ptr, ".string", 7) == 0 && isspace(source_ptr[7]))
    {
        parse_string_directive(unit);
        return 1;
    }

    return 0;
}

int string_list_cmp(const void* vlhs, const void* vrhs)
{
    const string_constant_t* lhs = vlhs;
    const string_constant_t* rhs = vrhs;

    return lhs->id - rhs->id;
}

void parse_file(asm_unit_t* asm_unit)
{
    asm_unit->labels = mk_hash_table(1031); // prime number
    DYNARRAY_INIT(asm_unit->relocs, 256);
    DYNARRAY_INIT(asm_unit->strings, 256);
    DYNARRAY_INIT(asm_unit->object_buffer, 4096);

    source_ptr = start_of_line = asm_unit->source;
    current_line = 0;

    while (*source_ptr)
    {
        const char* label, *opcode, *operand;
        label = opcode = operand = NULL;
        ++current_line;
        start_of_line = source_ptr;

        consume_whitespace();
        if (*source_ptr == '\n')
        {
            ++source_ptr;
            continue;
        }

        if (*source_ptr == '/') // comment line
        {
            consume_comments();
        }
        else if (!parse_directive(asm_unit)) // handle assembler directives
        {
            while ((label = parse_label()))
            {
                hash_table_insert(&asm_unit->labels, label, (hash_value_t){.idx = asm_unit->object_buffer.size});

                consume_whitespace();
                consume_comments();
                if (*source_ptr == '\n')
                {
                    ++source_ptr;
                    continue;
                }
            }

            consume_whitespace();

            opcode = parse_opcode();

            consume_whitespace();

            operand = parse_operand();

            consume_whitespace();

            consume_comments();

            fprintf(stderr, "parsed %s %s\n", opcode, operand);

            hash_value_t* val = hash_table_get(&ins_callbacks, opcode);
            if (!val || !val->fn_ptr)
            {
                fprintf(stderr, "unknown opcode %s\n", opcode);
                abort();
            }
            // callback to write the instruction bytes
            val->fn_ptr(operand, asm_unit);
        }

        if (*source_ptr == '\0')
            break;
        if (*source_ptr != '\n') // wtf
        {
            printf("wot??\n");
            abort();
        }

        ++source_ptr;
    }

    // resolve relocations
    for (int i = 0; i < asm_unit->relocs.size; ++i)
    {
        hash_value_t* label_addr = hash_table_get(&asm_unit->labels, asm_unit->relocs.ptr[i].target_label);
        if (!label_addr)
        {
            fprintf(stderr, "label '%s' not found\n", asm_unit->relocs.ptr[i].target_label);
            abort();
        }

        *(uint32_t*)(asm_unit->object_buffer.ptr + asm_unit->relocs.ptr[i].reloc_index) = label_addr->idx;
    }

    for (int i = 0; i < asm_unit->labels.bucket_count; ++i)
        if (asm_unit->labels.buckets[i])
        {
            hash_node_t* chain = asm_unit->labels.buckets[i];
            while (chain)
            {
                printf("label '%s' (%d)\n", chain->key, chain->value.idx);
                chain = chain->next_node;
            }
        }

    /*
    for (int i = 0; i < asm_unit->object_buffer.size; ++i)
    {
        printf("%02x ", asm_unit->object_buffer.ptr[i]);
    }
    */

    qsort(asm_unit->strings.ptr, asm_unit->strings.size, sizeof(string_constant_t), string_list_cmp);
}
