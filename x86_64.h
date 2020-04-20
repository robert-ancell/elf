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
#define X86_64_REG_8 8
#define X86_64_REG_9 9
#define X86_64_REG_10 10
#define X86_64_REG_11 11
#define X86_64_REG_12 12
#define X86_64_REG_13 13
#define X86_64_REG_14 14
#define X86_64_REG_15 15

// Operations
#define X86_64_OP_ADD 0
#define X86_64_OP_OR 1
#define X86_64_OP_ADC 2
#define X86_64_OP_SBB 3
#define X86_64_OP_AND 4
#define X86_64_OP_SUB 5
#define X86_64_OP_XOR 6
#define X86_64_OP_CMP 7

// Conditions
#define X86_64_COND_OVERFLOW 0
#define X86_64_COND_NOT_OVERFLOW 1
#define X86_64_COND_BELOW 2
#define X86_64_COND_ABOVE_EQUAL 3
#define X86_64_COND_EQUAL 4
#define X86_64_COND_NOT_EQUAL 5
#define X86_64_COND_BELOW_EQUAL 6
#define X86_64_COND_ABOVE 7
#define X86_64_COND_SIGN 8
#define X86_64_COND_NOT_SIGN 9
#define X86_64_COND_PARITY_EVEN 10
#define X86_64_COND_PARITY_ODD 11
#define X86_64_COND_LESS 12
#define X86_64_COND_GREATER_EQUAL 13
#define X86_64_COND_LESS_EQUAL 14
#define X86_64_COND_GREATER 15

void x86_64_mov8_val (Bytes *buffer, int reg, uint8_t value);

void x86_64_mov32_val (Bytes *buffer, int reg, uint32_t value);

void x86_64_mov32_mem (Bytes *buffer, int reg, uint32_t offset);

void x86_64_mov64_val (Bytes *buffer, int reg, uint64_t value);

void x86_64_mov64_mem (Bytes *buffer, int reg, uint32_t offset);

void x86_64_op32 (Bytes *buffer, int op, int reg1, int reg2);

void x86_64_op32_val (Bytes *buffer, int op, int reg, uint32_t value);

void x86_64_op64 (Bytes *buffer, int op, int reg1, int reg2);

void x86_64_op64_val (Bytes *buffer, int op, int reg, uint64_t value);

void x86_64_push64 (Bytes *buffer, int reg);

void x86_64_push_val8 (Bytes *buffer, uint8_t value);

void x86_64_push_val32 (Bytes *buffer, uint32_t value);

void x86_64_pop64 (Bytes *buffer, int reg);

void x86_64_jmp8 (Bytes *buffer, uint8_t offset);

void x86_64_jmp8_cond (Bytes *buffer, int cond, uint8_t offset);

void x86_64_jmp32 (Bytes *buffer, uint32_t offset);

void x86_64_jmp32_cond (Bytes *buffer, int cond, uint32_t offset);

void x86_64_syscall (Bytes *buffer);
