#include "elf-token.h"

#include <string.h>

#include "utils.h"

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
