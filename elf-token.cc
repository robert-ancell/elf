/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-token.h"

Token::Token(TokenType type, size_t offset, size_t length, const char *data)
    : type(type), offset(offset), length(length), data(data) {}

std::string Token::get_text() { return std::string(data + offset, length); }

bool Token::has_text(std::string value) {
  for (size_t i = 0; i < length; i++) {
    if (value[i] == '\0' || data[offset + i] != value[i])
      return false;
  }

  return value[length] == '\0';
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
  case TOKEN_TYPE_OPEN_BRACKET:
    return "OPEN_BRACKET";
  case TOKEN_TYPE_CLOSE_BRACKET:
    return "CLOSE_BRACKET";
  case TOKEN_TYPE_COMMA:
    return "COMMA";
  case TOKEN_TYPE_OPEN_BRACE:
    return "OPEN_BRACE";
  case TOKEN_TYPE_CLOSE_BRACE:
    return "CLOSE_BRACE";
  case TOKEN_TYPE_EOF:
    return "EOF";
  }

  return "UNKNOWN(" + std::to_string(type) + ")";
}
