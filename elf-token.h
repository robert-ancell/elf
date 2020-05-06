/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

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

typedef struct {
    TokenType type;
    size_t offset;
    size_t length;
    int ref_count;
} Token;

Token *token_new (TokenType type, size_t offset, size_t length);

char *token_get_text (Token *token, const char *data);

bool token_has_text (Token *token, const char *data, const char *value);

bool token_parse_boolean_constant (Token *token, const char *data);

uint64_t token_parse_number_constant (Token *token, const char *data);

char *token_parse_text_constant (Token *token, const char *data);

char *token_to_string (Token *token);

Token *token_ref (Token *token);

void token_unref (Token *token);
