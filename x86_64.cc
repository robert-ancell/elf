/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "x86_64.h"

#define ADDRESS_16_PREFIX 0x66
#define ADDRESS_64_PREFIX 0x48 // REX.W

static void write_uint32(std::vector<uint8_t> &buffer, uint32_t value) {
  buffer.push_back((value >> 0) & 0xFF);
  buffer.push_back((value >> 8) & 0xFF);
  buffer.push_back((value >> 16) & 0xFF);
  buffer.push_back((value >> 24) & 0xFF);
}

static void write_uint64(std::vector<uint8_t> &buffer, uint64_t value) {
  buffer.push_back((value >> 0) & 0xFF);
  buffer.push_back((value >> 8) & 0xFF);
  buffer.push_back((value >> 16) & 0xFF);
  buffer.push_back((value >> 24) & 0xFF);
  buffer.push_back((value >> 32) & 0xFF);
  buffer.push_back((value >> 40) & 0xFF);
  buffer.push_back((value >> 48) & 0xFF);
  buffer.push_back((value >> 56) & 0xFF);
}

void x86_64_mov8_val(std::vector<uint8_t> &buffer, int reg, uint8_t value) {
  buffer.push_back(0xB0 + reg);
  buffer.push_back(value);
}

void x86_64_mov32_val(std::vector<uint8_t> &buffer, int reg, uint32_t value) {
  buffer.push_back(0xB8 + reg);
  write_uint32(buffer, value);
}

void x86_64_mov32_reg(std::vector<uint8_t> &buffer, int reg1, int reg2) {
  buffer.push_back(0x89);
  buffer.push_back(0xC0 | (reg1 << 3) | reg2);
}

void x86_64_mov32_mem(std::vector<uint8_t> &buffer, int reg, uint32_t offset) {
  buffer.push_back(0x89);
  buffer.push_back(0x0 | (reg << 3) | 0x5);
  write_uint32(buffer, offset);
}

void x86_64_mov64_val(std::vector<uint8_t> &buffer, int reg, uint64_t value) {
  buffer.push_back(ADDRESS_64_PREFIX);
  buffer.push_back(0xB8 + reg);
  write_uint64(buffer, value);
}

void x86_64_mov64_reg(std::vector<uint8_t> &buffer, int reg1, int reg2) {
  buffer.push_back(ADDRESS_64_PREFIX);
  buffer.push_back(0x89);
  buffer.push_back(0xC0 | (reg1 << 3) | reg2);
}

void x86_64_mov64_mem(std::vector<uint8_t> &buffer, int reg, uint32_t offset) {
  buffer.push_back(ADDRESS_64_PREFIX);
  buffer.push_back(0x89);
  buffer.push_back(0x0 | (reg << 3) | 0x5);
  write_uint32(buffer, offset);
}

void x86_64_op32(std::vector<uint8_t> &buffer, int op, int reg1, int reg2) {
  buffer.push_back(0x01);
  buffer.push_back(0xC0 | (reg1 << 3) | reg2);
}

void x86_64_op32_val(std::vector<uint8_t> &buffer, int op, int reg,
                     uint32_t value) {
  if (reg == X86_64_REG_ACCUMULATOR) {
    if (op == X86_64_OP_ADD)
      buffer.push_back(0x05);
    else if (op == X86_64_OP_OR)
      buffer.push_back(0x0D);
    else if (op == X86_64_OP_ADC)
      buffer.push_back(0x15);
    else if (op == X86_64_OP_SBB)
      buffer.push_back(0x1D);
    else if (op == X86_64_OP_AND)
      buffer.push_back(0x25);
    else if (op == X86_64_OP_SUB)
      buffer.push_back(0x2D);
    else if (op == X86_64_OP_XOR)
      buffer.push_back(0x35);
    else if (op == X86_64_OP_CMP)
      buffer.push_back(0x3D);
  } else {
    buffer.push_back(0x81);
    buffer.push_back(0x0 | (op << 3) | reg);
  }
  write_uint32(buffer, value);
}

void x86_64_op64(std::vector<uint8_t> &buffer, int op, int reg1, int reg2) {
  buffer.push_back(ADDRESS_64_PREFIX);
  buffer.push_back(0x01);
  buffer.push_back(0xC0 | (reg1 << 3) | reg2);
}

void x86_64_op64_val(std::vector<uint8_t> &buffer, int op, int reg,
                     uint64_t value) {
  buffer.push_back(ADDRESS_64_PREFIX);
  if (reg == X86_64_REG_ACCUMULATOR) {
    if (op == X86_64_OP_ADD)
      buffer.push_back(0x05);
    else if (op == X86_64_OP_OR)
      buffer.push_back(0x0D);
    else if (op == X86_64_OP_ADC)
      buffer.push_back(0x15);
    else if (op == X86_64_OP_SBB)
      buffer.push_back(0x1D);
    else if (op == X86_64_OP_AND)
      buffer.push_back(0x25);
    else if (op == X86_64_OP_SUB)
      buffer.push_back(0x2D);
    else if (op == X86_64_OP_XOR)
      buffer.push_back(0x35);
    else if (op == X86_64_OP_CMP)
      buffer.push_back(0x3D);
  } else {
    buffer.push_back(0x81);
    buffer.push_back(0x0 | (op << 3) | reg);
  }
  write_uint64(buffer, value);
}

void x86_64_push64(std::vector<uint8_t> &buffer, int reg) {
  buffer.push_back(0x50 + reg);
}

void x86_64_push_val8(std::vector<uint8_t> &buffer, uint8_t value) {
  buffer.push_back(0x6A);
  buffer.push_back(value);
}

void x86_64_push_val32(std::vector<uint8_t> &buffer, uint32_t value) {
  buffer.push_back(0x68);
  write_uint32(buffer, value);
}

void x86_64_pop64(std::vector<uint8_t> &buffer, int reg) {
  buffer.push_back(0x58 + reg);
}

void x86_64_jmp8(std::vector<uint8_t> &buffer, uint8_t offset) {
  buffer.push_back(0xEB);
  buffer.push_back(offset);
}

void x86_64_jmp8_cond(std::vector<uint8_t> &buffer, int cond, uint8_t offset) {
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
  buffer.push_back(opcode);
  buffer.push_back(offset);
}

void x86_64_jmp32(std::vector<uint8_t> &buffer, uint32_t offset) {
  buffer.push_back(0xE9);
  write_uint32(buffer, offset);
}

void x86_64_jmp32_cond(std::vector<uint8_t> &buffer, int cond,
                       uint32_t offset) {
  buffer.push_back(0x0F);
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
  buffer.push_back(opcode);
  write_uint32(buffer, offset);
}

void x86_64_syscall(std::vector<uint8_t> &buffer) {
  buffer.push_back(0x0F);
  buffer.push_back(0x05);
}
