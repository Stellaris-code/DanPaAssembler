#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "instructions.h"

const char* program =
"collatz:\n"
"movl 0 // get 'n'\n"
"pushl 0 // n\n"
"pushi #2\n"
"mod\n"
"pushi #0\n"
"eq\n"
"test\n"
"jf .L0\n"
"pushl 0 // n\n"
"pushi #2\n"
"idiv\n"
"ret\n"
"jmp .L1\n"
".L0:\n"
"pushi #3\n"
"pushl 0 // n\n"
"mul\n"
"pushi #1\n"
"add\n"
"ret\n"
".L1:\n"
"ret\n"
"main:\n"
"syscall #1\n"
"movl 0 // val = *sp\n"
".L2:\n"
"pushl 0 // val\n"
"call collatz\n"
"movl 0\n"
"pushl 0\n"
"syscall #0\n"
"pushl 0\n"
"pushi #1\n"
"neq\n"
"test\n"
"jt .L2\n"
"syscall #3 // exit\n"
"ret";

int main()
{
    const char* filename = "asm.dpa";
    const char* out_name = "D:/Compiegne C++/Projets C++/DanPaVM/build/in.bin";

    FILE *input = fopen(filename, "rb");
    fseek(input, 0, SEEK_END);
    long fsize = ftell(input);
    rewind(input);  /* same as rewind(f); */

    if (fsize <= 0)
    {
        fprintf(stderr, "could not read input file '%s'", filename);
        return -1;
    }

    uint8_t* source_buffer = malloc(fsize + 1);
    fread(source_buffer, 1, fsize, input);
    source_buffer[fsize] = '\0';
    fclose(input);

    register_instructions();

    asm_unit_t unit;
    unit.source = source_buffer;
    parse_file(&unit);

    // write output file
    FILE* file = fopen(out_name, "wb");

    // write the signature
    fwrite("DNPX", 1, 4, file);
    hash_value_t* init_addr_node = hash_table_get(&unit.labels, "_global_init");
    uint32_t addr = 0;
    if (!init_addr_node)
        printf("warning : no '_global_init' symbol !\n");
    else
        addr = init_addr_node->idx;

    // write main symbol location :
    fwrite(&addr, sizeof(uint32_t), 1, file);
    if (unit.strings.size >= 0x10000)
    {
        printf("warning : string table size is too large (doesn't fit in 16-bit) !\n");
    }
    // write string table size :
    fwrite(&unit.strings.size, sizeof(uint16_t), 1, file);
    for (int i = 0; i < unit.strings.size; ++i)
    {
        printf("string %d : '%s' (len: %d)\n", i, unit.strings.ptr[i].str, unit.strings.ptr[i].len);
        // string len
        fwrite(&unit.strings.ptr[i].len, sizeof(uint16_t), 1, file);
        // string data
        fwrite(unit.strings.ptr[i].str, sizeof(char), unit.strings.ptr[i].len, file);

        free((void*)unit.strings.ptr[i].str);
    }

    fwrite(unit.object_buffer.ptr, 1, unit.object_buffer.size, file);

    free(unit.object_buffer.ptr);
    free(unit.relocs.ptr);
    free(unit.strings.ptr);
    hash_table_clear(&unit.labels);

    fclose(file);
    return 0;
}
