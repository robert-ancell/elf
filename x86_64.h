/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdint.h>

#include "utils.h"

// 32 bit registers
#define X86_64_REG_EAX 0
#define X86_64_REG_ECX 1
#define X86_64_REG_EDX 2
#define X86_64_REG_EBX 3
#define X86_64_REG_ESP 4
#define X86_64_REG_EBP 5
#define X86_64_REG_ESI 6
#define X86_64_REG_EDI 7

// 64 bit registers
#define X86_64_REG_RAX 0
#define X86_64_REG_RCX 1
#define X86_64_REG_RDX 2
#define X86_64_REG_RBX 3
#define X86_64_REG_RSP 4
#define X86_64_REG_RBP 5
#define X86_64_REG_RSI 6
#define X86_64_REG_RDI 7

void x86_64_mov32 (Bytes *buffer, int reg, uint32_t value);

void x86_64_mov32_mem (Bytes *buffer, int reg, uint32_t offset);

void x86_64_mov64 (Bytes *buffer, int reg, uint64_t value);

void x86_64_mov64_mem (Bytes *buffer, int reg, uint32_t offset);

void x86_64_add32 (Bytes *buffer, int reg1, int reg2);

void x86_64_add64 (Bytes *buffer, int reg1, int reg2);

void x86_64_syscall (Bytes *buffer);
