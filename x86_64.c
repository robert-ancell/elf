#include "x86_64.h"

void
x86_64_mov (Bytes *buffer, int reg, uint32_t value)
{
    bytes_add (buffer, 0xB8 + reg);
    bytes_add (buffer, (value >>  0) & 0xFF);
    bytes_add (buffer, (value >>  8) & 0xFF);
    bytes_add (buffer, (value >> 16) & 0xFF);
    bytes_add (buffer, (value >> 24) & 0xFF);
}

void
x86_64_syscall (Bytes *buffer)
{
    bytes_add (buffer, 0x0F);
    bytes_add (buffer, 0x05);
}
