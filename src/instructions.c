#include "instructions.h"

#include <stdlib.h>
#include <errno.h>

#include "asm_unit_info.h"

void die()
{
    ;
    abort();
}

static inline int32_t parse_imm_int(const char* str)
{
    if (*str != '#')
        die();
    ++str;

    char* endptr;
    errno = 0;
    int result = strtol(str, &endptr, 0);
    if (endptr == str)
        die();
    if (errno != 0)
        die();

    return result;
}

static inline float parse_imm_float(const char* str)
{
    if (*str != '#')
        die();
    ++str;

    char* endptr;
    errno = 0;
    float result = strtof(str, &endptr);
    if (endptr == str)
        die();
    if (errno != 0)
        die();

    return result;
}

static inline uint16_t parse_var(const char* str)
{
    char* endptr;
    errno = 0;
    int var = strtol(str, &endptr, 10);
    if (endptr == str)
        die();
    if (errno != 0)
        die();
    if (var < 0 || var >= 65536)
        die();

    return var;
}

#define DECLARE_0OP(name, opbyte) \
void ins_##name(const char* operand, void* asm_unit_voidp) \
{ \
    (void)(operand); \
    asm_unit_t* asm_unit = asm_unit_voidp; \
    DYNARRAY_ADD(asm_unit->object_buffer, opbyte); \
}

#define DECLARE_1OP_I_IMM(name, opbyte) \
void ins_##name(const char* operand, void* asm_unit_voidp) \
{ \
    asm_unit_t* asm_unit = asm_unit_voidp; \
    DYNARRAY_ADD(asm_unit->object_buffer, opbyte); \
    DYNARRAY_RESIZE(asm_unit->object_buffer, asm_unit->object_buffer.size + 4); \
    if (*operand != '#') /* ref to a label */ \
    { \
        DYNARRAY_ADD(asm_unit->relocs, (reloc_pair_t){asm_unit->object_buffer.size - 4, operand}); \
        *(uint32_t*)(asm_unit->object_buffer.ptr + asm_unit->object_buffer.size-4) = 0xdeadbeef; \
    } \
    else \
    { \
        *(uint32_t*)(asm_unit->object_buffer.ptr + asm_unit->object_buffer.size-4) = parse_imm_int(operand); \
    } \
}
#define DECLARE_1OP_F_IMM(name, opbyte) \
void ins_##name(const char* operand, void* asm_unit_voidp) \
{ \
    asm_unit_t* asm_unit = asm_unit_voidp; \
    DYNARRAY_ADD(asm_unit->object_buffer, opbyte); \
    DYNARRAY_RESIZE(asm_unit->object_buffer, asm_unit->object_buffer.size + 4); \
    *(float*)(asm_unit->object_buffer.ptr + asm_unit->object_buffer.size-4) = parse_imm_float(operand); \
}

#define DECLARE_1OP_B_IMM(name, opbyte) \
void ins_##name(const char* operand, void* asm_unit_voidp) \
{ \
    asm_unit_t* asm_unit = asm_unit_voidp; \
    DYNARRAY_ADD(asm_unit->object_buffer, opbyte); \
    DYNARRAY_RESIZE(asm_unit->object_buffer, asm_unit->object_buffer.size + 1); \
    *(int8_t*)(asm_unit->object_buffer.ptr + asm_unit->object_buffer.size-1) = (int8_t)parse_imm_int(operand); \
}

#define DECLARE_1OP_VAR(name, opbyte) \
void ins_##name(const char* operand, void* asm_unit_voidp) \
{ \
    asm_unit_t* asm_unit = asm_unit_voidp; \
    DYNARRAY_ADD(asm_unit->object_buffer, opbyte); \
    DYNARRAY_RESIZE(asm_unit->object_buffer, asm_unit->object_buffer.size + 2); \
    *(uint16_t*)(asm_unit->object_buffer.ptr + asm_unit->object_buffer.size-2) = parse_var(operand); \
}

#define DECLARE_1OP_LBL(name, opbyte) \
void ins_##name(const char* label, void* asm_unit_voidp) \
{ \
    asm_unit_t* asm_unit = asm_unit_voidp; \
    DYNARRAY_ADD(asm_unit->object_buffer, opbyte); \
    DYNARRAY_ADD(asm_unit->relocs, (reloc_pair_t){asm_unit->object_buffer.size, label}); \
    DYNARRAY_RESIZE(asm_unit->object_buffer, asm_unit->object_buffer.size + 4); \
    *(uint32_t*)(asm_unit->object_buffer.ptr + asm_unit->object_buffer.size-4) = 0xdeadbeef; \
}

DECLARE_0OP(brk,  0xFF)
DECLARE_0OP(chknotnul,  0xF1)
DECLARE_0OP(isnull,  0xF2)

DECLARE_0OP(strlen,  0x20)
DECLARE_0OP(strcat,  0x21)
DECLARE_0OP(stradd,  0x22)
DECLARE_0OP(streq,  0x23)
DECLARE_0OP(nop, 0x90)
DECLARE_0OP(add, 0xA0)
DECLARE_0OP(sub, 0xA1)
DECLARE_0OP(mul, 0xA2)
DECLARE_0OP(idiv, 0xA3)
DECLARE_0OP(mod, 0xA4)
DECLARE_0OP(inc, 0xA5)
DECLARE_0OP(dec, 0xA6)
DECLARE_1OP_VAR(incl, 0xA7)
DECLARE_1OP_VAR(decl, 0xA8)
DECLARE_0OP(shl, 0xA9)
DECLARE_0OP(shr, 0xAA)
DECLARE_0OP(ret, 0xB0)
DECLARE_0OP(cvtf2i,0xC0)
DECLARE_0OP(cvti2f,0xC1)
DECLARE_0OP(cvti2s,0xC2)
DECLARE_0OP(cvtf2s,0xC3)
DECLARE_0OP(calli, 0x34)

DECLARE_0OP(eq,  0xE0)
DECLARE_0OP(neq, 0xE1)
DECLARE_0OP(lt, 0xE2)
DECLARE_0OP(land, 0xE6)
DECLARE_0OP(lor, 0xE7)
DECLARE_0OP(lnot, 0xE8)
DECLARE_0OP(feq, 0xE9)
DECLARE_0OP(alloc,  0xD0)
DECLARE_0OP(copy, 0xD1)
DECLARE_0OP(load,  0xD2)
DECLARE_0OP(store, 0xD3)
DECLARE_0OP(memsize,  0xD4)
DECLARE_0OP(memresize,  0xD5)
DECLARE_0OP(arraycat,  0xD6)
DECLARE_0OP(find,   0xD8)
DECLARE_0OP(findi,   0xD9)
DECLARE_0OP(mkrange,   0xDA)
DECLARE_0OP(randi, 0x40)
DECLARE_0OP(randf, 0x41)
DECLARE_0OP(randa, 0x42)
DECLARE_0OP(pow, 0x43)
DECLARE_0OP(ln, 0x44)
DECLARE_0OP(log10, 0x45)
DECLARE_0OP(exp, 0x46)
DECLARE_0OP(sqrt, 0x47)
DECLARE_0OP(abs, 0x48)
DECLARE_0OP(fabs, 0x49)
DECLARE_0OP(ceil, 0x4A)
DECLARE_0OP(floor, 0x4B)
DECLARE_0OP(rad2deg, 0x4C)
DECLARE_0OP(deg2rad, 0x4D)

DECLARE_1OP_VAR(eql, 0x60)
DECLARE_1OP_VAR(neql, 0x61)
DECLARE_1OP_VAR(ltl, 0x62)

DECLARE_0OP(cos, 0x50)
DECLARE_0OP(sin, 0x51)
DECLARE_0OP(tan, 0x52)
DECLARE_0OP(acos, 0x53)
DECLARE_0OP(asin, 0x54)
DECLARE_0OP(atan, 0x55)
DECLARE_0OP(atan2, 0x56)

DECLARE_0OP(pop,  0x10)
DECLARE_1OP_I_IMM(pushi,   0x11)
DECLARE_1OP_I_IMM(stackcpy,   0xD7)
DECLARE_1OP_F_IMM(pushf,   0x12)
DECLARE_1OP_I_IMM(syscall, 0xF0)

DECLARE_1OP_VAR(pushs,   0x13)
DECLARE_1OP_VAR(pushl,   0x14)
DECLARE_1OP_VAR(pushg,   0x15)
DECLARE_1OP_VAR(movl,   0x16)
DECLARE_1OP_VAR(movg,   0x17)
DECLARE_1OP_VAR(copyl,   0x18)
DECLARE_0OP(dup,  0x19)
DECLARE_1OP_VAR(getaddrl,   0x1A)
DECLARE_1OP_VAR(getaddrg,   0x1B)
DECLARE_0OP(cmov, 0x1C)
DECLARE_0OP(pushnull, 0x1D)
DECLARE_1OP_B_IMM(pushib,   0x1E)

DECLARE_1OP_LBL(jt,  0x30)
DECLARE_1OP_LBL(jf,  0x31)
DECLARE_1OP_LBL(jmp, 0x32)
DECLARE_1OP_LBL(call, 0x33)

void register_instructions()
{
    ins_callbacks = mk_hash_table(211); // prime

#define REGISTER_INS(name) \
    hash_table_insert(&ins_callbacks, #name, (hash_value_t){.fn_ptr = ins_##name});

    REGISTER_INS(brk);
    REGISTER_INS(chknotnul);
    REGISTER_INS(nop);
    REGISTER_INS(dup);
    REGISTER_INS(strlen);
    REGISTER_INS(strcat);
    REGISTER_INS(stradd);
    REGISTER_INS(streq);
    REGISTER_INS(add);
    REGISTER_INS(sub);
    REGISTER_INS(mul);
    REGISTER_INS(idiv);
    REGISTER_INS(mod);
    REGISTER_INS(inc);
    REGISTER_INS(dec);
    REGISTER_INS(shl);
    REGISTER_INS(shr);
    REGISTER_INS(ret);
    REGISTER_INS(cvtf2i);
    REGISTER_INS(cvti2f);
    REGISTER_INS(cvti2s);
    REGISTER_INS(cvtf2s);
    REGISTER_INS(calli);

    REGISTER_INS(randi);
    REGISTER_INS(randf);
    REGISTER_INS(randa);
    REGISTER_INS(sin);
    REGISTER_INS(cos);
    REGISTER_INS(tan);
    REGISTER_INS(asin);
    REGISTER_INS(acos);
    REGISTER_INS(atan);
    REGISTER_INS(atan2);
    REGISTER_INS(pow)
    REGISTER_INS(ln)
    REGISTER_INS(log10)
    REGISTER_INS(exp)
    REGISTER_INS(sqrt)
    REGISTER_INS(abs)
    REGISTER_INS(fabs)
    REGISTER_INS(ceil)
    REGISTER_INS(floor)
    REGISTER_INS(rad2deg)
    REGISTER_INS(deg2rad)

    REGISTER_INS(eq);
    REGISTER_INS(neq);
    REGISTER_INS(lt);

    REGISTER_INS(eql);
    REGISTER_INS(neql);
    REGISTER_INS(ltl);

    REGISTER_INS(land);
    REGISTER_INS(lor);
    REGISTER_INS(lnot);
    REGISTER_INS(feq);
    REGISTER_INS(alloc);
    REGISTER_INS(copy);
    REGISTER_INS(pop);
    REGISTER_INS(load);
    REGISTER_INS(store);
    REGISTER_INS(memsize);
    REGISTER_INS(memresize);
    REGISTER_INS(arraycat);
    REGISTER_INS(stackcpy);
    REGISTER_INS(find);
    REGISTER_INS(findi);
    REGISTER_INS(mkrange);

    REGISTER_INS(pushi);
    REGISTER_INS(pushf);
    REGISTER_INS(pushs);
    REGISTER_INS(syscall);
    REGISTER_INS(pushl);
    REGISTER_INS(pushg);
    REGISTER_INS(movl);
    REGISTER_INS(movg);
    REGISTER_INS(copyl);
    REGISTER_INS(getaddrl);
    REGISTER_INS(getaddrg);
    REGISTER_INS(cmov);
    REGISTER_INS(pushnull);
    REGISTER_INS(pushib);
    REGISTER_INS(isnull);

    REGISTER_INS(incl);
    REGISTER_INS(decl);

    REGISTER_INS(jt);
    REGISTER_INS(jf);
    REGISTER_INS(jmp);
    REGISTER_INS(call);
}
