#include "x86_64.h"

#define ADDRESS_16_PREFIX 0x66
#define ADDRESS_64_PREFIX 0x48 // REX.W

static void
write_uint32 (Bytes *buffer, uint32_t value)
{
    bytes_add (buffer, (value >>  0) & 0xFF);
    bytes_add (buffer, (value >>  8) & 0xFF);
    bytes_add (buffer, (value >> 16) & 0xFF);
    bytes_add (buffer, (value >> 24) & 0xFF);
}

static void
write_uint64 (Bytes *buffer, uint64_t value)
{
    bytes_add (buffer, (value >>  0) & 0xFF);
    bytes_add (buffer, (value >>  8) & 0xFF);
    bytes_add (buffer, (value >> 16) & 0xFF);
    bytes_add (buffer, (value >> 24) & 0xFF);
    bytes_add (buffer, (value >> 32) & 0xFF);
    bytes_add (buffer, (value >> 40) & 0xFF);
    bytes_add (buffer, (value >> 48) & 0xFF);
    bytes_add (buffer, (value >> 56) & 0xFF);
}

void
x86_64_mov8_val (Bytes *buffer, int reg, uint8_t value)
{
    bytes_add (buffer, 0xB0 + reg);
    bytes_add (buffer, value);
}

void
x86_64_mov32_val (Bytes *buffer, int reg, uint32_t value)
{
    bytes_add (buffer, 0xB8 + reg);
    write_uint32 (buffer, value);
}

void
x86_64_mov32_reg (Bytes *buffer, int reg1, int reg2)
{
    bytes_add (buffer, 0x89);
    bytes_add (buffer, 0xC0 | (reg1 << 3) | reg2);
}

void
x86_64_mov32_mem (Bytes *buffer, int reg, uint32_t offset)
{
    bytes_add (buffer, 0x89);
    bytes_add (buffer, 0x0 | (reg << 3) | 0x5);
    write_uint32 (buffer, offset);
}

void
x86_64_mov64_val (Bytes *buffer, int reg, uint64_t value)
{
    bytes_add (buffer, ADDRESS_64_PREFIX);
    bytes_add (buffer, 0xB8 + reg);
    write_uint64 (buffer, value);
}

void
x86_64_mov64_reg (Bytes *buffer, int reg1, int reg2)
{
    bytes_add (buffer, ADDRESS_64_PREFIX);
    bytes_add (buffer, 0x89);
    bytes_add (buffer, 0xC0 | (reg1 << 3) | reg2);
}

void
x86_64_mov64_mem (Bytes *buffer, int reg, uint32_t offset)
{
    bytes_add (buffer, ADDRESS_64_PREFIX);
    bytes_add (buffer, 0x89);
    bytes_add (buffer, 0x0 | (reg << 3) | 0x5);
    write_uint32 (buffer, offset);
}

void
x86_64_op32 (Bytes *buffer, int op, int reg1, int reg2)
{
    bytes_add (buffer, 0x01);
    bytes_add (buffer, 0xC0 | (reg1 << 3) | reg2);
}

void
x86_64_op32_val (Bytes *buffer, int op, int reg, uint32_t value)
{
    if (reg == X86_64_REG_ACCUMULATOR) {
        if (op == X86_64_OP_ADD)
            bytes_add (buffer, 0x05);
        else if (op == X86_64_OP_OR)
            bytes_add (buffer, 0x0D);
        else if (op == X86_64_OP_ADC)
            bytes_add (buffer, 0x15);
        else if (op == X86_64_OP_SBB)
            bytes_add (buffer, 0x1D);
        else if (op == X86_64_OP_AND)
            bytes_add (buffer, 0x25);
        else if (op == X86_64_OP_SUB)
            bytes_add (buffer, 0x2D);
        else if (op == X86_64_OP_XOR)
            bytes_add (buffer, 0x35);
        else if (op == X86_64_OP_CMP)
            bytes_add (buffer, 0x3D);
    }
    else {
        bytes_add (buffer, 0x81);
        bytes_add (buffer, 0x0 | (op << 3) | reg);
    }
    write_uint32 (buffer, value);
}

void
x86_64_op64 (Bytes *buffer, int op, int reg1, int reg2)
{
    bytes_add (buffer, ADDRESS_64_PREFIX);
    bytes_add (buffer, 0x01);
    bytes_add (buffer, 0xC0 | (reg1 << 3) | reg2);
}

void
x86_64_op64_val (Bytes *buffer, int op, int reg, uint64_t value)
{
    bytes_add (buffer, ADDRESS_64_PREFIX);
    if (reg == X86_64_REG_ACCUMULATOR) {
        if (op == X86_64_OP_ADD)
            bytes_add (buffer, 0x05);
        else if (op == X86_64_OP_OR)
            bytes_add (buffer, 0x0D);
        else if (op == X86_64_OP_ADC)
            bytes_add (buffer, 0x15);
        else if (op == X86_64_OP_SBB)
            bytes_add (buffer, 0x1D);
        else if (op == X86_64_OP_AND)
            bytes_add (buffer, 0x25);
        else if (op == X86_64_OP_SUB)
            bytes_add (buffer, 0x2D);
        else if (op == X86_64_OP_XOR)
            bytes_add (buffer, 0x35);
        else if (op == X86_64_OP_CMP)
            bytes_add (buffer, 0x3D);
    }
    else {
        bytes_add (buffer, 0x81);
        bytes_add (buffer, 0x0 | (op << 3) | reg);
    }
    write_uint64 (buffer, value);
}

void
x86_64_push64 (Bytes *buffer, int reg)
{
    bytes_add (buffer, 0x50 + reg);
}

void
x86_64_push_val8 (Bytes *buffer, uint8_t value)
{
    bytes_add (buffer, 0x6A);
    bytes_add (buffer, value);
}

void
x86_64_push_val32 (Bytes *buffer, uint32_t value)
{
    bytes_add (buffer, 0x68);
    write_uint32 (buffer, value);
}

void
x86_64_pop64 (Bytes *buffer, int reg)
{
    bytes_add (buffer, 0x58 + reg);
}

void
x86_64_jmp8 (Bytes *buffer, uint8_t offset)
{
    bytes_add (buffer, 0xEB);
    bytes_add (buffer, offset);
}

void
x86_64_jmp8_cond (Bytes *buffer, int cond, uint8_t offset)
{
    uint8_t opcode = 0;
    switch (cond) {
    case X86_64_COND_OVERFLOW:
        opcode = 0x70;
        break;
    case X86_64_COND_NOT_OVERFLOW:
        opcode = 0x71;
        break;
    case X86_64_COND_BELOW:
        opcode = 0x72;
        break;
    case X86_64_COND_ABOVE_EQUAL:
        opcode = 0x73;
        break;
    case X86_64_COND_EQUAL:
        opcode = 0x74;
        break;
    case X86_64_COND_NOT_EQUAL:
        opcode = 0x75;
        break;
    case X86_64_COND_BELOW_EQUAL:
        opcode = 0x76;
        break;
    case X86_64_COND_ABOVE:
        opcode = 0x77;
        break;
    case X86_64_COND_SIGN:
        opcode = 0x78;
        break;
    case X86_64_COND_NOT_SIGN:
        opcode = 0x79;
        break;
    case X86_64_COND_PARITY_EVEN:
        opcode = 0x7A;
        break;
    case X86_64_COND_PARITY_ODD:
        opcode = 0x7A;
        break;
    case X86_64_COND_LESS:
        opcode = 0x7C;
        break;
    case X86_64_COND_GREATER_EQUAL:
        opcode = 0x7D;
        break;
    case X86_64_COND_LESS_EQUAL:
        opcode = 0x7E;
        break;
    case X86_64_COND_GREATER:
        opcode = 0x7F;
        break;
    }
    bytes_add (buffer, opcode);
    bytes_add (buffer, offset);
}

void
x86_64_jmp32 (Bytes *buffer, uint32_t offset)
{
    bytes_add (buffer, 0xE9);
    write_uint32 (buffer, offset);
}

void
x86_64_jmp32_cond (Bytes *buffer, int cond, uint32_t offset)
{
    bytes_add (buffer, 0x0F);
    uint8_t opcode = 0;
    switch (cond) {
    case X86_64_COND_OVERFLOW:
        opcode = 0x80;
        break;
    case X86_64_COND_NOT_OVERFLOW:
        opcode = 0x81;
        break;
    case X86_64_COND_BELOW:
        opcode = 0x82;
        break;
    case X86_64_COND_ABOVE_EQUAL:
        opcode = 0x83;
        break;
    case X86_64_COND_EQUAL:
        opcode = 0x84;
        break;
    case X86_64_COND_NOT_EQUAL:
        opcode = 0x85;
        break;
    case X86_64_COND_BELOW_EQUAL:
        opcode = 0x86;
        break;
    case X86_64_COND_ABOVE:
        opcode = 0x87;
        break;
    case X86_64_COND_SIGN:
        opcode = 0x88;
        break;
    case X86_64_COND_NOT_SIGN:
        opcode = 0x89;
        break;
    case X86_64_COND_PARITY_EVEN:
        opcode = 0x8A;
        break;
    case X86_64_COND_PARITY_ODD:
        opcode = 0x8A;
        break;
    case X86_64_COND_LESS:
        opcode = 0x8C;
        break;
    case X86_64_COND_GREATER_EQUAL:
        opcode = 0x8D;
        break;
    case X86_64_COND_LESS_EQUAL:
        opcode = 0x8E;
        break;
    case X86_64_COND_GREATER:
        opcode = 0x8F;
        break;
    }
    bytes_add (buffer, opcode);
    write_uint32 (buffer, offset);
}

void
x86_64_syscall (Bytes *buffer)
{
    bytes_add (buffer, 0x0F);
    bytes_add (buffer, 0x05);
}
