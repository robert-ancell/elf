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

// Registers
#define X86_64_REG_ACCUMULATOR 0
#define X86_64_REG_COUNTER 1
#define X86_64_REG_DATA 2
#define X86_64_REG_BASE 3
#define X86_64_REG_STACK_POINTER 4
#define X86_64_REG_STACK_BASE_POINTER 5
#define X86_64_REG_SOURCE 6
#define X86_64_REG_DESTINATION 7

void x86_64_mov32 (Bytes *buffer, int reg, uint32_t value);

void x86_64_mov32_mem (Bytes *buffer, int reg, uint32_t offset);

void x86_64_mov64 (Bytes *buffer, int reg, uint64_t value);

void x86_64_mov64_mem (Bytes *buffer, int reg, uint32_t offset);

void x86_64_add32 (Bytes *buffer, int reg1, int reg2);

void x86_64_add64 (Bytes *buffer, int reg1, int reg2);

void x86_64_syscall (Bytes *buffer);
