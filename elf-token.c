#include <string.h>

#include "elf-token.h"

char *
token_to_string (Token *token)
{
    switch (token->type) {
    case TOKEN_TYPE_WORD:
        return strdup ("WORD");
    case TOKEN_TYPE_NUMBER:
        return strdup ("NUMBER");
    case TOKEN_TYPE_TEXT:
        return strdup ("TEXT");
    case TOKEN_TYPE_ASSIGN:
        return strdup ("ASSIGN");
    case TOKEN_TYPE_ADD:
        return strdup ("ADD");
    case TOKEN_TYPE_SUBTRACT:
        return strdup ("SUBTRACT");
    case TOKEN_TYPE_MULTIPLY:
        return strdup ("MULTIPLY");
    case TOKEN_TYPE_DIVIDE:
        return strdup ("DIVIDE");
    case TOKEN_TYPE_OPEN_PAREN:
        return strdup ("OPEN_PAREN");
    case TOKEN_TYPE_CLOSE_PAREN:
        return strdup ("CLOSE_PAREN");
    case TOKEN_TYPE_COMMA:
        return strdup ("COMMA");
    case TOKEN_TYPE_OPEN_BRACE:
        return strdup ("OPEN_BRACE");
    case TOKEN_TYPE_CLOSE_BRACE:
        return strdup ("CLOSE_BRACE");
    }

    return strdup ("UNKNOWN");
}
