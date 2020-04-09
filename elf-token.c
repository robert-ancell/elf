#include "elf-token.h"

#include <stdbool.h>
#include <string.h>

#include "utils.h"

char *
token_get_text (Token *token, const char *data)
{
    char *result = malloc (sizeof (char) * (token->length + 1));
    for (size_t i = 0; i < token->length; i++)
        result[i] = data[token->offset + i];
    result[token->length] = '\0';
    return result;
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
    char *value = malloc (sizeof (char) * (token->length - 1));
    size_t length = 0;

    // Iterate over the characters inside the quotes
    bool in_escape = false;
    for (size_t i = 1; i < (token->length - 1); i++) {
        char c = data[token->offset + i];

        if (!in_escape && c == '\\') {
            in_escape = true;
            continue;
        }

        if (in_escape) {
            if (c == 'n')
                c = '\n';
            else if (c == 'r')
                c = '\r';
        }

        value[length] = c;
        length++;
    }

    return value;
}

char *
token_to_string (Token *token)
{
    switch (token->type) {
    case TOKEN_TYPE_WORD:
        return strdup_printf ("WORD");
    case TOKEN_TYPE_NUMBER:
        return strdup_printf ("NUMBER");
    case TOKEN_TYPE_TEXT:
        return strdup_printf ("TEXT");
    case TOKEN_TYPE_ASSIGN:
        return strdup_printf ("ASSIGN");
    case TOKEN_TYPE_ADD:
        return strdup_printf ("ADD");
    case TOKEN_TYPE_SUBTRACT:
        return strdup_printf ("SUBTRACT");
    case TOKEN_TYPE_MULTIPLY:
        return strdup_printf ("MULTIPLY");
    case TOKEN_TYPE_DIVIDE:
        return strdup_printf ("DIVIDE");
    case TOKEN_TYPE_OPEN_PAREN:
        return strdup_printf ("OPEN_PAREN");
    case TOKEN_TYPE_CLOSE_PAREN:
        return strdup_printf ("CLOSE_PAREN");
    case TOKEN_TYPE_COMMA:
        return strdup_printf ("COMMA");
    case TOKEN_TYPE_OPEN_BRACE:
        return strdup_printf ("OPEN_BRACE");
    case TOKEN_TYPE_CLOSE_BRACE:
        return strdup_printf ("CLOSE_BRACE");
    }

    return strdup_printf ("UNKNOWN(%d)", token->type);
}
