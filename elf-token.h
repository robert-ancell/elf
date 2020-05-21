/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdlib.h>
#include <string>

typedef enum {
  TOKEN_TYPE_COMMENT,
  TOKEN_TYPE_WORD,
  TOKEN_TYPE_MEMBER,
  TOKEN_TYPE_NUMBER,
  TOKEN_TYPE_TEXT,
  TOKEN_TYPE_ASSIGN,
  TOKEN_TYPE_NOT,
  TOKEN_TYPE_EQUAL,
  TOKEN_TYPE_NOT_EQUAL,
  TOKEN_TYPE_GREATER,
  TOKEN_TYPE_GREATER_EQUAL,
  TOKEN_TYPE_LESS,
  TOKEN_TYPE_LESS_EQUAL,
  TOKEN_TYPE_ADD,
  TOKEN_TYPE_SUBTRACT,
  TOKEN_TYPE_MULTIPLY,
  TOKEN_TYPE_DIVIDE,
  TOKEN_TYPE_OPEN_PAREN,
  TOKEN_TYPE_CLOSE_PAREN,
  TOKEN_TYPE_COMMA,
  TOKEN_TYPE_OPEN_BRACE,
  TOKEN_TYPE_CLOSE_BRACE,
} TokenType;

struct Token {
  TokenType type;
  size_t offset;
  size_t length;
  int ref_count;

  Token(TokenType type, size_t offset, size_t length);

  std::string get_text(const char *data);

  bool has_text(const char *data, std::string value);

  bool parse_boolean_constant(const char *data);

  uint64_t parse_number_constant(const char *data);

  std::string parse_text_constant(const char *data);

  std::string to_string();

  Token *ref();

  void unref();
};
