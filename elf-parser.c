#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "elf-parser.h"

static Operation *
make_variable_definition (Token *data_type, Token *name, Operation *value)
{
    OperationVariableDefinition *o = malloc (sizeof (OperationVariableDefinition));
    memset (o, 0, sizeof (OperationVariableDefinition));
    o->type = OPERATION_TYPE_VARIABLE_DEFINITION;
    o->data_type = data_type;
    o->name = name;
    o->value = value;
    return (Operation *) o;
}

static Operation *
make_variable_assignment (Token *name, Operation *value)
{
    OperationVariableAssignment *o = malloc (sizeof (OperationVariableAssignment));
    memset (o, 0, sizeof (OperationVariableAssignment));
    o->type = OPERATION_TYPE_VARIABLE_ASSIGNMENT;
    o->name = name;
    o->value = value;
    return (Operation *) o;
}

static Operation *
make_function_call (Token *name)
{
    OperationFunctionCall *o = malloc (sizeof (OperationFunctionCall));
    memset (o, 0, sizeof (OperationFunctionCall));
    o->type = OPERATION_TYPE_FUNCTION_CALL;
    o->name = name;
    return (Operation *) o;
}

static Operation *
make_return (Operation *value)
{
    OperationReturn *o = malloc (sizeof (OperationReturn));
    memset (o, 0, sizeof (OperationReturn));
    o->type = OPERATION_TYPE_RETURN;
    o->value = value;
    return (Operation *) o;
}

static Operation *
make_number_constant (Token *value)
{
    OperationNumberConstant *o = malloc (sizeof (OperationNumberConstant));
    memset (o, 0, sizeof (OperationNumberConstant));
    o->type = OPERATION_TYPE_NUMBER_CONSTANT;
    o->value = value;
    return (Operation *) o;
}

static Operation *
make_text_constant (Token *value)
{
    OperationTextConstant *o = malloc (sizeof (OperationTextConstant));
    memset (o, 0, sizeof (OperationTextConstant));
    o->type = OPERATION_TYPE_TEXT_CONSTANT;
    o->value = value;
    return (Operation *) o;
}

static Operation *
make_variable_value (Token *name)
{
    OperationVariableValue *o = malloc (sizeof (OperationVariableValue));
    memset (o, 0, sizeof (OperationVariableValue));
    o->type = OPERATION_TYPE_VARIABLE_VALUE;
    o->name = name;
    return (Operation *) o;
}

static Operation *
make_binary (Token *operator, Operation *a, Operation *b)
{
    OperationBinary *o = malloc (sizeof (OperationBinary));
    memset (o, 0, sizeof (OperationBinary));
    o->type = OPERATION_TYPE_BINARY;
    o->operator = operator;
    o->a = a;
    o->b = b;
    return (Operation *) o;
}

static bool
is_number_char (char c)
{
    return c >= '0' && c <= '9';
}

static bool
is_symbol_char (char c)
{
    return is_number_char (c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
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
       return !is_symbol_char (next_c);
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
   case TOKEN_TYPE_COMMA:
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
            if (c == ' ' || c == '\r' || c == '\n')
                continue;

            tokens_length++;
            tokens = realloc (tokens, sizeof (Token *) * (tokens_length + 1)); // FIXME: Double size each time
            tokens[tokens_length] = NULL;
            Token *token = tokens[tokens_length - 1] = malloc (sizeof (Token));
            memset (token, 0, sizeof (Token));
            token->offset = offset;
            token->length = 1;

            if (c == '(')
                token->type = TOKEN_TYPE_OPEN_PAREN;
            else if (c == ')')
                token->type = TOKEN_TYPE_CLOSE_PAREN;
            else if (c == ',')
                token->type = TOKEN_TYPE_COMMA;
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
            else if (is_symbol_char (c))
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
token_has_text (const char *data, Token *token, const char *value)
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
        if (token_has_text (data, token, builtin_types[i]))
            return true;

    return false;
}

static bool
token_text_matches (const char *data, Token *a, Token *b)
{
    if (a->length != b->length)
        return false;

    for (size_t i = 0; i < a->length; i++)
        if (data[a->offset + i] != data[b->offset + i])
            return false;

    return true;
}

static bool
is_variable (OperationFunctionDefinition *function, const char *data, Token *token)
{
    for (int i = 0; function->body[i] != NULL; i++) {
        if (function->body[i]->type != OPERATION_TYPE_VARIABLE_DEFINITION)
            continue;

        OperationVariableDefinition *op = (OperationVariableDefinition *) function->body[i];
        if (token_text_matches (data, op->name, token))
            return true;
    }

    return false;
}

static bool
is_function (OperationFunctionDefinition *function, const char *data, Token *token)
{
    return token_has_text (data, token, "print");
}

static Operation *parse_expression (OperationFunctionDefinition *function, const char *data, Token **tokens, size_t *offset);

static Operation *
parse_value (OperationFunctionDefinition *function, const char *data, Token **tokens, size_t *offset)
{
    Token *token = tokens[*offset];
    if (token == NULL)
        return NULL;

    if (token->type == TOKEN_TYPE_NUMBER) {
        (*offset)++;
        return make_number_constant (token);
    }
    else if (token->type == TOKEN_TYPE_TEXT) {
        (*offset)++;
        return make_text_constant (token);
    }
    else if (is_variable (function, data, token)) {
        (*offset)++;
        return make_variable_value (token);
    }
    else if (is_function (function, data, token)) {
        Token *name = token;
        (*offset)++;

        Token *open_paren_token = tokens[*offset];
        if (open_paren_token != NULL && open_paren_token->type == TOKEN_TYPE_OPEN_PAREN) {
            (*offset)++;

            bool closed = false;
            while (tokens[*offset] != NULL) {
                Token *param_token = tokens[*offset];
                if (param_token->type == TOKEN_TYPE_CLOSE_PAREN) {
                    (*offset)++;
                    closed = true;
                    break;
                }

                // FIXME comma separate

                Operation *value = parse_expression (function, data, tokens, offset);
                if (value == NULL) {
                    printf ("Invalid parameter\n");
                    return NULL;
                }
            }

            if (!closed) {
                printf ("Unclosed paren\n");
                return NULL;
            }
        }

        return make_function_call (name);
    }

    return NULL;
}

static bool
token_is_binary_operator (Token *token)
{
    return token->type == TOKEN_TYPE_ADD ||
           token->type == TOKEN_TYPE_SUBTRACT ||
           token->type == TOKEN_TYPE_MULTIPLY ||
           token->type == TOKEN_TYPE_DIVIDE;
}

static Operation *
parse_expression (OperationFunctionDefinition *function, const char *data, Token **tokens, size_t *offset)
{
    Operation *a = parse_value (function, data, tokens, offset);
    if (a == NULL)
        return NULL;

    Token *operator = tokens[*offset];
    if (operator == NULL)
        return a;

    if (!token_is_binary_operator (operator))
        return a;
    (*offset)++;

    Operation *b = parse_value (function, data, tokens, offset);
    if (b == NULL) {
        printf ("Missing second value in binary operation\n");
        operation_free (a);
        return NULL;
    }

    return make_binary (operator, a, b);
}

static bool
parse_function_body (OperationFunctionDefinition *function, const char *data, Token **tokens, size_t *offset)
{
    size_t body_length = 0;

    function->body = malloc (sizeof (Operation *));
    function->body[0] = NULL;
    while (tokens[*offset] != NULL) {
        Token *token = tokens[*offset];

        Operation *op = NULL;
        if (is_data_type (data, token)) {
            Token *data_type = token;
            (*offset)++;

            Token *name = tokens[*offset]; // FIXME: Check valid name
            (*offset)++;

            Token *assignment_token = tokens[*offset];
            if (assignment_token == NULL) {
                op = make_variable_definition (data_type, name, NULL);
            } else if (assignment_token->type == TOKEN_TYPE_ASSIGN) {
                (*offset)++;

                Operation *value = parse_expression (function, data, tokens, offset);
                if (value == NULL) {
                    printf ("Invalid value for variable\n");
                    return false;
                }
                op = make_variable_definition (data_type, name, value);
            } else {
                op = make_variable_definition (data_type, name, NULL);
            }
        }
        else if (token_has_text (data, token, "return")) {
            (*offset)++;

            Operation *value = parse_expression (function, data, tokens, offset);
            if (value == NULL) {
                printf ("Not value return value\n");
                return false;
            }

            op = make_return (value);
        }
        else if (is_variable (function, data, token)) {
            Token *name = token;
            (*offset)++;

            Token *assignment_token = tokens[*offset];
            if (assignment_token->type != TOKEN_TYPE_ASSIGN) {
                 printf ("Missing assignment token\n");
                 return false;
            }
            (*offset)++;

            Operation *value = parse_expression (function, data, tokens, offset);
            if (value == NULL) {
                 printf ("Invalid value for variable\n");
                 return false;
            }

            op = make_variable_assignment (name, value);
        }
        else if ((op = parse_value (function, data, tokens, offset)) != NULL) {
            // FIXME: Only allow functions and variable values
        }
        else {
            printf ("Unexpected token\n");
            return false;
        }

        body_length++;
        function->body = realloc (function->body, sizeof (Operation *) * (body_length + 1));
        function->body[body_length - 1] = op;
        function->body[body_length] = NULL;
    }

    return true;
}

OperationFunctionDefinition *
elf_parse (const char *data, size_t data_length)
{
    Token **tokens = elf_lex (data, data_length);

    OperationFunctionDefinition *main_function = malloc (sizeof (OperationFunctionDefinition));
    memset (main_function, 0, sizeof (OperationFunctionDefinition));

    size_t offset = 0;
    if (!parse_function_body (main_function, data, tokens, &offset)) {
        operation_free ((Operation *) main_function);
        tokens_free (tokens);
        return NULL;
    }

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
    case OPERATION_TYPE_VARIABLE_VALUE:
        return strdup ("VARIABLE_VALUE");
    case OPERATION_TYPE_BINARY:
        return strdup ("BINARY");
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
    case OPERATION_TYPE_BINARY: {
        OperationBinary *op = (OperationBinary *) operation;
        operation_free (op->a);
        operation_free (op->b);
        break;
    }
    case OPERATION_TYPE_NUMBER_CONSTANT:
    case OPERATION_TYPE_TEXT_CONSTANT:
    case OPERATION_TYPE_VARIABLE_VALUE:
        break;
    }

    free (operation);
}
