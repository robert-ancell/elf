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
  TOKEN_TYPE_EOF,
} TokenType;

struct Token {
  TokenType type;
  size_t offset;
  size_t length;
  const char *data;

  Token(TokenType type, size_t offset, size_t length, const char *data);

  std::string get_text();

  bool has_text(std::string value);

  std::string to_string();
};
