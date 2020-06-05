/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-lexer.h"

#include <memory>
#include <stdio.h>

static bool is_number_char(char c) { return c >= '0' && c <= '9'; }

static bool is_symbol_char(char c) {
  return is_number_char(c) || (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') || c == '_';
}

static char string_is_complete(const char *data, std::shared_ptr<Token> token) {
  // Need at least an open and closing quote
  if (token->length < 2)
    return false;

  // Both start and end characters need to be the same
  char start_char = data[token->offset];
  char end_char = data[token->offset + token->length - 1];
  if (end_char != start_char)
    return false;

  bool in_escape = false;
  for (size_t i = 1; i < token->length - 1; i++) {
    if (in_escape) {
      in_escape = false;
      continue;
    }

    if (data[token->offset + i] == '\\')
      in_escape = true;
  }

  // Closing quote needs to be non-escaped
  return !in_escape;
}

static bool token_is_complete(const char *data, std::shared_ptr<Token> token,
                              char next_c) {
  switch (token->type) {
  case TOKEN_TYPE_COMMENT:
    return next_c == '\n' || next_c == '\0';
  case TOKEN_TYPE_WORD:
    return !is_symbol_char(next_c);
  case TOKEN_TYPE_MEMBER:
    return !is_symbol_char(next_c);
  case TOKEN_TYPE_NUMBER:
    return !is_number_char(next_c);
  case TOKEN_TYPE_TEXT:
    return string_is_complete(data, token);
  case TOKEN_TYPE_NOT:
    return next_c != '=';
  case TOKEN_TYPE_LESS:
    return next_c != '=';
  case TOKEN_TYPE_GREATER:
    return next_c != '=';
  case TOKEN_TYPE_ASSIGN:
    return next_c != '=';
  case TOKEN_TYPE_EQUAL:
  case TOKEN_TYPE_NOT_EQUAL:
  case TOKEN_TYPE_LESS_EQUAL:
  case TOKEN_TYPE_GREATER_EQUAL:
  case TOKEN_TYPE_ADD:
  case TOKEN_TYPE_SUBTRACT:
  case TOKEN_TYPE_MULTIPLY:
  case TOKEN_TYPE_DIVIDE:
  case TOKEN_TYPE_OPEN_PAREN:
  case TOKEN_TYPE_CLOSE_PAREN:
  case TOKEN_TYPE_COMMA:
  case TOKEN_TYPE_OPEN_BRACE:
  case TOKEN_TYPE_CLOSE_BRACE:
  case TOKEN_TYPE_EOF:
    return true;
  }

  return false;
}

std::vector<std::shared_ptr<Token>> elf_lex(const char *data,
                                            size_t data_length) {
  std::vector<std::shared_ptr<Token>> tokens;
  std::shared_ptr<Token> current_token = nullptr;

  for (size_t offset = 0; offset < data_length; offset++) {
    // FIXME: Support UTF-8
    char c = data[offset];

    if (current_token != nullptr && token_is_complete(data, current_token, c))
      current_token = nullptr;

    if (current_token == nullptr) {
      // Skip whitespace
      if (c == ' ' || c == '\r' || c == '\n')
        continue;

      TokenType type;
      if (c == '#')
        type = TOKEN_TYPE_COMMENT;
      else if (c == '(')
        type = TOKEN_TYPE_OPEN_PAREN;
      else if (c == ')')
        type = TOKEN_TYPE_CLOSE_PAREN;
      else if (c == ',')
        type = TOKEN_TYPE_COMMA;
      else if (c == '{')
        type = TOKEN_TYPE_OPEN_BRACE;
      else if (c == '}')
        type = TOKEN_TYPE_CLOSE_BRACE;
      else if (c == '=')
        type = TOKEN_TYPE_ASSIGN;
      else if (c == '!')
        type = TOKEN_TYPE_NOT;
      else if (c == '<')
        type = TOKEN_TYPE_LESS;
      else if (c == '>')
        type = TOKEN_TYPE_GREATER;
      else if (c == '+')
        type = TOKEN_TYPE_ADD;
      else if (c == '-')
        type = TOKEN_TYPE_SUBTRACT;
      else if (c == '*')
        type = TOKEN_TYPE_MULTIPLY;
      else if (c == '/')
        type = TOKEN_TYPE_DIVIDE;
      else if (is_number_char(c))
        type = TOKEN_TYPE_NUMBER;
      else if (c == '"' || c == '\'')
        type = TOKEN_TYPE_TEXT;
      else if (c == '.') // FIXME: Don't allow whitespace before it?
        type = TOKEN_TYPE_MEMBER;
      else if (is_symbol_char(c))
        type = TOKEN_TYPE_WORD;

      auto token = std::make_shared<Token>(type, offset, 1, data);
      tokens.push_back(token);

      current_token = token;
    } else {
      if (c == '=') {
        if (current_token->type == TOKEN_TYPE_ASSIGN)
          current_token->type = TOKEN_TYPE_EQUAL;
        else if (current_token->type == TOKEN_TYPE_NOT)
          current_token->type = TOKEN_TYPE_NOT_EQUAL;
        else if (current_token->type == TOKEN_TYPE_LESS)
          current_token->type = TOKEN_TYPE_LESS_EQUAL;
        else if (current_token->type == TOKEN_TYPE_GREATER)
          current_token->type = TOKEN_TYPE_GREATER_EQUAL;
      }

      current_token->length++;
    }
  }

  auto token = std::make_shared<Token>(TOKEN_TYPE_EOF, data_length, 0, data);
  tokens.push_back(token);

  return tokens;
}
