#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "elf-parser.h"

static bool
is_number_char (char c)
{
    return c >= '0' && c <= '9';
}

static char
ending_char_is_escaped (const char *data, Token *token)
{
    if (token->length < 3)
        return false;

    return data[token->offset + token->length - 2] == '\\';
}

static char
string_is_complete (const char *data, Token *token)
{
    // Need at least an open and closing quote
    if (token->length < 2)
        return false;

    // Both start and end characters need to be the same
    char start_char = data[token->offset];
    char end_char = data[token->offset + token->length - 1];
    if (end_char != start_char)
        return false;

    // Closing quote needs to be non-escaped
    return !ending_char_is_escaped (data, token);
}

static bool
token_is_complete (const char *data, Token *token, char next_c)
{
   switch (token->type) {
   case TOKEN_TYPE_WORD:
       return isspace (next_c);
   case TOKEN_TYPE_NUMBER:
       return !is_number_char (next_c);
   case TOKEN_TYPE_TEXT:
       return string_is_complete (data, token);
   case TOKEN_TYPE_ASSIGN:
   case TOKEN_TYPE_ADD:
   case TOKEN_TYPE_SUBTRACT:
   case TOKEN_TYPE_MULTIPLY:
   case TOKEN_TYPE_DIVIDE:
   case TOKEN_TYPE_OPEN_PAREN:
   case TOKEN_TYPE_CLOSE_PAREN:
   case TOKEN_TYPE_OPEN_BRACE:
   case TOKEN_TYPE_CLOSE_BRACE:
       return true;
   }

   return false;
}

static Token **
elf_lex (const char *data, size_t data_length)
{
    Token **tokens = NULL;
    size_t tokens_length = 0;
    Token *current_token = NULL;

    tokens = malloc (sizeof (Token *));
    tokens[0] = NULL;
    for (size_t offset = 0; offset < data_length; offset++) {
        // FIXME: Support UTF-8
        char c = data[offset];

        if (current_token != NULL && token_is_complete (data, current_token, c))
            current_token = NULL;

        if (current_token == NULL) {
            // Skip whitespace
            if (isspace (c))
                continue;

            tokens_length++;
            tokens = realloc (tokens, sizeof (Token *) * (tokens_length + 1)); // FIXME: Double size each time
            tokens[tokens_length] = NULL;
            Token *token = tokens[tokens_length - 1] = malloc (sizeof (Token));
            memset (token, 0, sizeof (Token));
            token->type = TOKEN_TYPE_WORD;
            token->offset = offset;
            token->length = 1;

            if (c == '(')
                token->type = TOKEN_TYPE_OPEN_PAREN;
            else if (c == ')')
                token->type = TOKEN_TYPE_CLOSE_PAREN;
            else if (c == '{')
                token->type = TOKEN_TYPE_OPEN_BRACE;
            else if (c == '}')
                token->type = TOKEN_TYPE_CLOSE_BRACE;
            else if (c == '=')
                token->type = TOKEN_TYPE_ASSIGN;
            else if (c == '+')
                token->type = TOKEN_TYPE_ADD;
            else if (c == '-')
                token->type = TOKEN_TYPE_SUBTRACT;
            else if (c == '*')
                token->type = TOKEN_TYPE_MULTIPLY;
            else if (c == '/')
                token->type = TOKEN_TYPE_DIVIDE;
            else if (is_number_char (c))
                token->type = TOKEN_TYPE_NUMBER;
            else if (c == '"' || c == '\'')
                token->type = TOKEN_TYPE_TEXT;
            else
                token->type = TOKEN_TYPE_WORD;

            current_token = token;
        }
        else {
            current_token->length++;
        }
    }

    return tokens;
}

static void
tokens_free (Token **tokens)
{
    for (size_t i = 0; tokens[i] != NULL; i++)
        free (tokens[i]);
    free (tokens);
}

static bool
token_matches (const char *data, Token *token, const char *value)
{
    for (int i = 0; i < token->length; i++) {
        if (value[i] == '\0' || data[token->offset + i] != value[i])
            return false;
    }

    return value[token->length] == '\0';
}

static bool
is_data_type (const char *data, Token *token)
{
    const char *builtin_types[] = { "uint8", "int8",
                                    "uint16", "int16",
                                    "uint32", "int32",
                                    "uint64", "int64",
                                    "utf8",
                                    NULL };

    if (token->type != TOKEN_TYPE_WORD)
        return false;

    for (int i = 0; builtin_types[i] != NULL; i++)
        if (token_matches (data, token, builtin_types[i]))
            return true;

    return false;
}

static bool
is_variable (const char *data, Token *token)
{
    return token->type == TOKEN_TYPE_WORD; // FIXME
}

static Operation *
parse_value (const char *data, Token **tokens, size_t *offset)
{
    Token *token = tokens[*offset];
    if (token == NULL)
        return NULL;

    if (token->type == TOKEN_TYPE_NUMBER) {
        OperationNumberConstant *op = malloc (sizeof (OperationNumberConstant));
        memset (op, 0, sizeof (OperationNumberConstant));
        op->type = OPERATION_TYPE_NUMBER_CONSTANT;
        op->value = token;
        (*offset)++;
        return (Operation *) op;
    }
    else if (token->type == TOKEN_TYPE_TEXT) {
        OperationTextConstant *op = malloc (sizeof (OperationTextConstant));
        memset (op, 0, sizeof (OperationTextConstant));
        op->type = OPERATION_TYPE_TEXT_CONSTANT;
        op->value = token;
        (*offset)++;
        return (Operation *) op;
    }

    return NULL;
}

static Operation **
parse_sequence (const char *data, Token **tokens, size_t *offset)
{
    Operation **sequence = NULL;
    size_t sequence_length = 0;

    sequence = malloc (sizeof (Operation *));
    sequence[0] = NULL;
    while (tokens[*offset] != NULL) {
        Token *token = tokens[*offset];

        Operation *op = NULL;
        if (is_data_type (data, token)) {
            Token *data_type = token;
            (*offset)++;

            Token *name = tokens[*offset]; // FIXME: Check valid name
            (*offset)++;

            Token *assignment_token = tokens[*offset];
            Operation *value = NULL;
            if (assignment_token != NULL && assignment_token->type == TOKEN_TYPE_ASSIGN) {
                 (*offset)++;

                 value = parse_value (data, tokens, offset);
                 if (value == NULL) {
                    printf ("Invalid value for variable\n");
                    return NULL;
                }
            }

            OperationVariableDefinition *o = malloc (sizeof (OperationVariableDefinition));
            memset (o, 0, sizeof (OperationVariableDefinition));
            o->type = OPERATION_TYPE_VARIABLE_DEFINITION;
            o->data_type = data_type;
            o->name = name;
            o->value = value;
            op = (Operation *) o;
        }
        else if (token_matches (data, token, "return")) {
            (*offset)++;

            Operation *value = parse_value (data, tokens, offset);
            if (value == NULL) {
                printf ("Not value return value\n");
                return NULL;
            }

            OperationReturn *o = malloc (sizeof (OperationReturn));
            memset (o, 0, sizeof (OperationReturn));
            o->type = OPERATION_TYPE_RETURN;
            o->value = value;
            op = (Operation *) o;
        }
        else if (is_variable (data, token)) {
            Token *name = token;
            (*offset)++;

            Token *assignment_token = tokens[*offset];
            if (assignment_token->type != TOKEN_TYPE_ASSIGN) {
                 printf ("Missing assignment token\n");
                 return NULL;
            }
            (*offset)++;

            Operation *value = parse_value (data, tokens, offset);
            if (value == NULL) {
                 printf ("Invalid value for variable\n");
                 return NULL;
            }

            OperationVariableAssignment *o = malloc (sizeof (OperationVariableAssignment));
            memset (o, 0, sizeof (OperationVariableAssignment));
            o->type = OPERATION_TYPE_VARIABLE_ASSIGNMENT;
            o->name = name;
            o->value = value;
            op = (Operation *) o;
        }
        else {
            printf ("Unexpected token\n");
            return NULL;
        }

        sequence_length++;
        sequence = realloc (sequence, sizeof (Operation *) * (sequence_length + 1));
        sequence[sequence_length - 1] = op;
        sequence[sequence_length] = NULL;
    }

    return sequence;
}

OperationFunctionDefinition *
elf_parse (const char *data, size_t data_length)
{
    Token **tokens = elf_lex (data, data_length);

    size_t offset = 0;
    Operation **body = parse_sequence (data, tokens, &offset);
    if (body == NULL) {
        tokens_free (tokens);
        return NULL;
    }

    OperationFunctionDefinition *main_function = malloc (sizeof (OperationFunctionDefinition));
    memset (main_function, 0, sizeof (OperationFunctionDefinition));
    main_function->body = body;

    tokens_free (tokens);

    return main_function;
}

char *
operation_to_string (Operation *operation)
{
    switch (operation->type) {
    case OPERATION_TYPE_VARIABLE_DEFINITION:
        return strdup ("VARIABLE_DEFINITION");
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT:
        return strdup ("VARIABLE_ASSIGNMENT");
    case OPERATION_TYPE_FUNCTION_DEFINE:
        return strdup ("FUNCTION_DEFINE");
    case OPERATION_TYPE_FUNCTION_CALL:
        return strdup ("FUNCTION_CALL");
    case OPERATION_TYPE_RETURN:
        return strdup ("RETURN");
    case OPERATION_TYPE_NUMBER_CONSTANT:
        return strdup ("NUMBER_CONSTANT");
    case OPERATION_TYPE_TEXT_CONSTANT:
        return strdup ("TEXT_CONSTANT");
    }

    return strdup ("UNKNOWN");
}

void
operation_free (Operation *operation)
{
    if (operation == NULL)
        return;

    switch (operation->type) {
    case OPERATION_TYPE_VARIABLE_DEFINITION: {
        OperationVariableDefinition *op = (OperationVariableDefinition *) operation;
        operation_free (op->value);
        break;
    }
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT: {
        OperationVariableAssignment *op = (OperationVariableAssignment *) operation;
        operation_free (op->value);
        break;
    }
    case OPERATION_TYPE_FUNCTION_DEFINE: {
        OperationFunctionDefinition *op = (OperationFunctionDefinition *) operation;
        for (int i = 0; op->parameters[i] != NULL; i++)
            operation_free (op->parameters[i]);
        free (op->parameters);
        for (int i = 0; op->body[i] != NULL; i++)
            operation_free (op->body[i]);
        free (op->body);
        break;
    }
    case OPERATION_TYPE_FUNCTION_CALL: {
        OperationFunctionCall *op = (OperationFunctionCall *) operation;
        for (int i = 0; op->parameters[i] != NULL; i++)
            operation_free (op->parameters[i]);
        free (op->parameters);
        break;
    }
    case OPERATION_TYPE_RETURN: {
        OperationReturn *op = (OperationReturn *) operation;
        operation_free (op->value);
        break;
    }
    case OPERATION_TYPE_NUMBER_CONSTANT:
    case OPERATION_TYPE_TEXT_CONSTANT:
        break;
    }

    free (operation);
}
