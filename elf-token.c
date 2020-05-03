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

#include "utils.h"

static int
hex_digit (char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else
        return -1;
}

char *
token_get_text (Token *token, const char *data)
{
    char *result = malloc (sizeof (char) * (token->length + 1));
    for (size_t i = 0; i < token->length; i++)
        result[i] = data[token->offset + i];
    result[token->length] = '\0';
    return result;
}

bool
token_has_text (Token *token, const char *data, const char *value)
{
    for (int i = 0; i < token->length; i++) {
        if (value[i] == '\0' || data[token->offset + i] != value[i])
            return false;
    }

    return value[token->length] == '\0';
}

bool
token_parse_boolean_constant (Token *token, const char *data)
{
    autofree_str text = token_get_text (token, data);
    return str_equal (text, "true");
}

uint64_t
token_parse_number_constant (Token *token, const char *data)
{
    uint64_t value = 0;

    for (size_t i = 0; i < token->length; i++)
        value = value * 10 + data[token->offset + i] - '0';

    return value;
}

char *
token_parse_text_constant (Token *token, const char *data)
{
    autofree_bytes buffer = bytes_new (token->length);

    // Iterate over the characters inside the quotes
    bool in_escape = false;
    for (size_t i = 1; i < (token->length - 1); i++) {
        char c = data[token->offset + i];

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
            else if (c == 'u' || c == 'U') {
                size_t length = c == 'u' ? 4 : 8;

                if (i + length > token->length)
                    break;

                uint32_t unichar = 0;
                bool valid = true;
                for (int j = 0; j < length; j++) {
                    int digit = hex_digit(data[token->offset + i + 1 + j]);
                    if (digit < 0)
                        valid = false;
                    else
                        unichar = unichar << 4 | digit;
                }
                i += length;
                if (valid)
                    utf8_encode (buffer, unichar);
                else
                    bytes_add (buffer, '?');
                continue;
            }
        }

        bytes_add (buffer, c);
    }
    bytes_add (buffer, '\0');
    bytes_trim (buffer);

    return (char *) bytes_steal (buffer);
}

char *
token_to_string (Token *token)
{
    switch (token->type) {
    case TOKEN_TYPE_COMMENT:
        return str_printf ("COMMENT");
    case TOKEN_TYPE_WORD:
        return str_printf ("WORD");
    case TOKEN_TYPE_MEMBER:
        return str_printf ("MEMBER");
    case TOKEN_TYPE_NUMBER:
        return str_printf ("NUMBER");
    case TOKEN_TYPE_TEXT:
        return str_printf ("TEXT");
    case TOKEN_TYPE_ASSIGN:
        return str_printf ("ASSIGN");
    case TOKEN_TYPE_NOT:
        return str_printf ("NOT");
    case TOKEN_TYPE_EQUAL:
        return str_printf ("EQUAL");
    case TOKEN_TYPE_NOT_EQUAL:
        return str_printf ("NOT_EQUAL");
    case TOKEN_TYPE_GREATER:
        return str_printf ("GREATER");
    case TOKEN_TYPE_GREATER_EQUAL:
        return str_printf ("GREATER_EQUAL");
    case TOKEN_TYPE_LESS:
        return str_printf ("LESS");
    case TOKEN_TYPE_LESS_EQUAL:
        return str_printf ("LESS_EQUAL");
    case TOKEN_TYPE_ADD:
        return str_printf ("ADD");
    case TOKEN_TYPE_SUBTRACT:
        return str_printf ("SUBTRACT");
    case TOKEN_TYPE_MULTIPLY:
        return str_printf ("MULTIPLY");
    case TOKEN_TYPE_DIVIDE:
        return str_printf ("DIVIDE");
    case TOKEN_TYPE_OPEN_PAREN:
        return str_printf ("OPEN_PAREN");
    case TOKEN_TYPE_CLOSE_PAREN:
        return str_printf ("CLOSE_PAREN");
    case TOKEN_TYPE_COMMA:
        return str_printf ("COMMA");
    case TOKEN_TYPE_OPEN_BRACE:
        return str_printf ("OPEN_BRACE");
    case TOKEN_TYPE_CLOSE_BRACE:
        return str_printf ("CLOSE_BRACE");
    }

    return str_printf ("UNKNOWN(%d)", token->type);
}
