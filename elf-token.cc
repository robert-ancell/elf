/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-token.h"

#include <stdbool.h>

static int hex_digit(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else
    return -1;
}

Token::Token(TokenType type, size_t offset, size_t length)
    : type(type), offset(offset), length(length), ref_count(1) {}

std::string Token::get_text(const char *data) {
  std::string value;
  for (size_t i = 0; i < length; i++)
    value += data[offset + i];
  return value;
}

bool Token::has_text(const char *data, std::string value) {
  for (size_t i = 0; i < length; i++) {
    if (value[i] == '\0' || data[offset + i] != value[i])
      return false;
  }

  return value[length] == '\0';
}

bool Token::parse_boolean_constant(const char *data) {
  return get_text(data) == "true";
}

uint64_t Token::parse_number_constant(const char *data) {
  uint64_t value = 0;

  for (size_t i = 0; i < length; i++)
    value = value * 10 + data[offset + i] - '0';

  return value;
}

std::string Token::parse_text_constant(const char *data) {
  std::string value;
  // Iterate over the characters inside the quotes
  bool in_escape = false;
  for (size_t i = 1; i < (length - 1); i++) {
    char c = data[offset + i];
    size_t n_remaining = length - 1;

    if (!in_escape && c == '\\') {
      in_escape = true;
      continue;
    }

    if (in_escape) {
      in_escape = false;
      if (c == '\"')
        c = '\"';
      if (c == '\'')
        c = '\'';
      if (c == '\\')
        c = '\\';
      if (c == 'n')
        c = '\n';
      else if (c == 'r')
        c = '\r';
      else if (c == 't')
        c = '\t';
      else if (c == 'x') {
        if (n_remaining < 3)
          break;

        int digit0 = hex_digit(data[offset + i + 1]);
        int digit1 = hex_digit(data[offset + i + 2]);
        i += 2;
        if (digit0 >= 0 && digit1 >= 0)
          value += digit0 << 4 | digit1;
        else
          value += '?';
        continue;
      } else if (c == 'u' || c == 'U') {
        size_t length = c == 'u' ? 4 : 8;

        if (n_remaining < 1 + length)
          break;

        uint32_t unichar = 0;
        bool valid = true;
        for (size_t j = 0; j < length; j++) {
          int digit = hex_digit(data[offset + i + 1 + j]);
          if (digit < 0)
            valid = false;
          else
            unichar = unichar << 4 | digit;
        }
        i += length;
        if (valid) {
          if (unichar < (1 << 7)) {
            value += unichar;
          } else if (unichar < (1 << 11)) {
            value += 0xC0 | (unichar >> 6);
            value += 0x80 | ((unichar >> 0) & 0x3F);
          } else if (unichar < (1 << 16)) {
            value += 0xE0 | (unichar >> 12);
            value += 0x80 | ((unichar >> 6) & 0x3F);
            value += 0x80 | ((unichar >> 0) & 0x3F);
          } else if (unichar < (1 << 21)) {
            value += 0xF0 | (unichar >> 18);
            value += 0x80 | ((unichar >> 12) & 0x3F);
            value += 0x80 | ((unichar >> 6) & 0x3F);
            value += 0x80 | ((unichar >> 0) & 0x3F);
          } else {
            value += '?';
          }
        } else
          value += '?';
        continue;
      }
    }

    value += c;
  }

  return value;
}

std::string Token::to_string() {
  switch (type) {
  case TOKEN_TYPE_COMMENT:
    return "COMMENT";
  case TOKEN_TYPE_WORD:
    return "WORD";
  case TOKEN_TYPE_MEMBER:
    return "MEMBER";
  case TOKEN_TYPE_NUMBER:
    return "NUMBER";
  case TOKEN_TYPE_TEXT:
    return "TEXT";
  case TOKEN_TYPE_ASSIGN:
    return "ASSIGN";
  case TOKEN_TYPE_NOT:
    return "NOT";
  case TOKEN_TYPE_EQUAL:
    return "EQUAL";
  case TOKEN_TYPE_NOT_EQUAL:
    return "NOT_EQUAL";
  case TOKEN_TYPE_GREATER:
    return "GREATER";
  case TOKEN_TYPE_GREATER_EQUAL:
    return "GREATER_EQUAL";
  case TOKEN_TYPE_LESS:
    return "LESS";
  case TOKEN_TYPE_LESS_EQUAL:
    return "LESS_EQUAL";
  case TOKEN_TYPE_ADD:
    return "ADD";
  case TOKEN_TYPE_SUBTRACT:
    return "SUBTRACT";
  case TOKEN_TYPE_MULTIPLY:
    return "MULTIPLY";
  case TOKEN_TYPE_DIVIDE:
    return "DIVIDE";
  case TOKEN_TYPE_OPEN_PAREN:
    return "OPEN_PAREN";
  case TOKEN_TYPE_CLOSE_PAREN:
    return "CLOSE_PAREN";
  case TOKEN_TYPE_COMMA:
    return "COMMA";
  case TOKEN_TYPE_OPEN_BRACE:
    return "OPEN_BRACE";
  case TOKEN_TYPE_CLOSE_BRACE:
    return "CLOSE_BRACE";
  }

  return "UNKNOWN(" + std::to_string(type) + ")";
}

Token *Token::ref() {
  ref_count++;
  return this;
}

void Token::unref() {
  ref_count--;
  if (ref_count == 0)
    delete this;
}
