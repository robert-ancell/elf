#pragma once

#include <stdlib.h>

typedef enum {
    TOKEN_TYPE_WORD,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_TEXT,
    TOKEN_TYPE_ASSIGN,
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
} Token;

char *token_to_string (Token *token);
