#include "x86_64.h"

void
x86_64_mov32_val (Bytes *buffer, int reg, uint32_t value)
{
    bytes_add (buffer, 0xB8 + reg);
    bytes_add (buffer, (value >>  0) & 0xFF);
    bytes_add (buffer, (value >>  8) & 0xFF);
    bytes_add (buffer, (value >> 16) & 0xFF);
    bytes_add (buffer, (value >> 24) & 0xFF);
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
    bytes_add (buffer, (offset >>  0) & 0xFF);
    bytes_add (buffer, (offset >>  8) & 0xFF);
    bytes_add (buffer, (offset >> 16) & 0xFF);
    bytes_add (buffer, (offset >> 24) & 0xFF);
}

void
x86_64_mov64_val (Bytes *buffer, int reg, uint64_t value)
{
    bytes_add (buffer, 0x48);
    bytes_add (buffer, 0xB8 + reg);
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
x86_64_mov64_reg (Bytes *buffer, int reg1, int reg2)
{
    bytes_add (buffer, 0x48);
    bytes_add (buffer, 0x89);
    bytes_add (buffer, 0xC0 | (reg1 << 3) | reg2);
}

void
x86_64_mov64_mem (Bytes *buffer, int reg, uint32_t offset)
{
    bytes_add (buffer, 0x48);
    bytes_add (buffer, 0x89);
    bytes_add (buffer, 0x0 | (reg << 3) | 0x5);
    bytes_add (buffer, (offset >>  0) & 0xFF);
    bytes_add (buffer, (offset >>  8) & 0xFF);
    bytes_add (buffer, (offset >> 16) & 0xFF);
    bytes_add (buffer, (offset >> 24) & 0xFF);
}

void
x86_64_add32 (Bytes *buffer, int reg1, int reg2)
{
    bytes_add (buffer, 0x01);
    bytes_add (buffer, 0xC0 | (reg1 << 3) | reg2);
}

void
x86_64_add64 (Bytes *buffer, int reg1, int reg2)
{
    bytes_add (buffer, 0x48);
    bytes_add (buffer, 0x01);
    bytes_add (buffer, 0xC0 | (reg1 << 3) | reg2);
}

void
x86_64_syscall (Bytes *buffer)
{
    bytes_add (buffer, 0x0F);
    bytes_add (buffer, 0x05);
}
