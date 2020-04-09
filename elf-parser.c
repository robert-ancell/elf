#include "elf-parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void
print_token_error (const char *data, Token *token, const char *message)
{
    size_t line_offset = 0;
    size_t line_number = 1;
    for (size_t i = 0; i < token->offset; i++) {
        if (data[i] == '\n') {
            line_offset = i + 1;
            line_number++;
        }
    }

    printf ("Line %zi:\n", line_number);
    for (size_t i = line_offset; data[i] != '\0' && data[i] != '\n'; i++)
        printf ("%c", data[i]);
    printf ("\n");
    for (size_t i = line_offset; i < token->offset; i++)
        printf (" ");
    for (size_t i = 0; i < token->length; i++)
        printf ("^");
    printf ("\n");
    printf ("%s\n", message);
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
    const char *builtin_types[] = { "bool",
                                    "uint8", "int8",
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
is_parameter_name (const char *data, Token *token)
{
    if (token->type != TOKEN_TYPE_WORD)
        return false;

    // FIXME: Don't allow reserved words / data types

    return true;
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
is_boolean (OperationFunctionDefinition *function, const char *data, Token *token)
{
    return token_has_text (data, token, "true") || token_has_text (data, token, "false");
}

static bool
is_variable_definition_with_name (Operation *operation, const char *data, Token *token)
{
    if (operation->type != OPERATION_TYPE_VARIABLE_DEFINITION)
        return false;

    OperationVariableDefinition *op = (OperationVariableDefinition *) operation;
    if (token_text_matches (data, op->name, token))
        return true;

    return false;
}

static bool
is_variable (OperationFunctionDefinition *function, const char *data, Token *token)
{
    for (int i = 0; function->parameters[i] != NULL; i++) {
        if (is_variable_definition_with_name (function->parameters[i], data, token))
            return true;
    }

    for (int i = 0; function->body[i] != NULL; i++) {
        if (is_variable_definition_with_name (function->body[i], data, token))
            return true;
    }

    return false;
}

static OperationFunctionDefinition *
find_function (OperationFunctionDefinition *function, const char *data, Token *token)
{
    if (token->type != TOKEN_TYPE_WORD)
        return NULL;

    for (int i = 0; function->body[i] != NULL; i++) {
        if (function->body[i]->type != OPERATION_TYPE_FUNCTION_DEFINITION)
            continue;

        OperationFunctionDefinition *op = (OperationFunctionDefinition *) function->body[i];
        if (token_text_matches (data, op->name, token))
            return op;
    }

    return NULL;
}

static bool
is_builtin_function (OperationFunctionDefinition *function, const char *data, Token *token)
{
    const char *builtin_functions[] = { "print",
                                        NULL };

    if (token->type != TOKEN_TYPE_WORD)
        return false;

    for (int i = 0; builtin_functions[i] != NULL; i++)
        if (token_has_text (data, token, builtin_functions[i]))
            return true;

    return false;
}

static Token *
current_token (Token **tokens, size_t *offset)
{
    return tokens[*offset];
}

static void
next_token (size_t *offset)
{
    (*offset)++;
}

static Operation *parse_expression (OperationFunctionDefinition *function, const char *data, Token **tokens, size_t *offset);

static Operation *
parse_value (OperationFunctionDefinition *function, const char *data, Token **tokens, size_t *offset)
{
    Token *token = current_token (tokens, offset);
    if (token == NULL)
        return NULL;

    OperationFunctionDefinition *f;
    if (token->type == TOKEN_TYPE_NUMBER) {
        next_token (offset);
        return make_number_constant (token);
    }
    else if (token->type == TOKEN_TYPE_TEXT) {
        next_token (offset);
        return make_text_constant (token);
    }
    else if (is_boolean (function, data, token)) {
        next_token (offset);
        return make_boolean_constant (token);
    }
    else if (is_variable (function, data, token)) {
        next_token (offset);
        return make_variable_value (token);
    }
    else if ((f = find_function (function, data, token)) != NULL || is_builtin_function (function, data, token)) {
        Token *name = token;
        next_token (offset);

        Operation **parameters = malloc (sizeof (Operation *));
        size_t parameters_length = 0;
        parameters[0] = NULL;

        Token *open_paren_token = current_token (tokens, offset);
        if (open_paren_token != NULL && open_paren_token->type == TOKEN_TYPE_OPEN_PAREN) {
            next_token (offset);

            bool closed = false;
            while (current_token (tokens, offset) != NULL) {
                Token *t = current_token (tokens, offset);
                if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
                    next_token (offset);
                    closed = true;
                    break;
                }

                if (parameters_length > 0) {
                    if (t->type != TOKEN_TYPE_COMMA) {
                        print_token_error (data, current_token (tokens, offset), "Missing comma");
                        return false;
                    }
                    next_token (offset);
                    t = current_token (tokens, offset);
                }

                Operation *value = parse_expression (function, data, tokens, offset);
                if (value == NULL) {
                    print_token_error (data, current_token (tokens, offset), "Invalid parameter");
                    return NULL;
                }

                parameters_length++;
                parameters = realloc (parameters, sizeof (Operation *) * (parameters_length + 1));
                parameters[parameters_length - 1] = value;
                parameters[parameters_length] = NULL;
            }

            if (!closed) {
                print_token_error (data, current_token (tokens, offset), "Unclosed paren");
                return NULL;
            }
        }

        return make_function_call (name, parameters, f);
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

    Token *operator = current_token (tokens, offset);
    if (operator == NULL)
        return a;

    if (!token_is_binary_operator (operator))
        return a;
    next_token (offset);

    Operation *b = parse_value (function, data, tokens, offset);
    if (b == NULL) {
        print_token_error (data, current_token (tokens, offset), "Missing second value in binary operation");
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
    while (current_token (tokens, offset) != NULL) {
        Token *token = current_token (tokens, offset);

        Operation *op = NULL;
        if (token->type == TOKEN_TYPE_CLOSE_BRACE && function->name != NULL) {
            next_token (offset);
            return true;
        } else if (is_data_type (data, token)) {
            Token *data_type = token;
            next_token (offset);

            Token *name = current_token (tokens, offset); // FIXME: Check valid name
            next_token (offset);

            Token *assignment_token = current_token (tokens, offset);
            if (assignment_token == NULL) {
                op = make_variable_definition (data_type, name, NULL);
            } else if (assignment_token->type == TOKEN_TYPE_ASSIGN) {
                next_token (offset);

                Operation *value = parse_expression (function, data, tokens, offset);
                if (value == NULL) {
                    print_token_error (data, current_token (tokens, offset), "Invalid value for variable");
                    return false;
                }
                op = make_variable_definition (data_type, name, value);
            } else if (assignment_token->type == TOKEN_TYPE_OPEN_PAREN) {
                next_token (offset);

                Operation **parameters = malloc (sizeof (Operation *));
                size_t parameters_length = 0;
                parameters[0] = NULL;
                bool closed = false;
                while (current_token (tokens, offset) != NULL) {
                    Token *t = current_token (tokens, offset);
                    if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
                        next_token (offset);
                        closed = true;
                        break;
                    }

                    if (parameters_length > 0) {
                        if (t->type != TOKEN_TYPE_COMMA) {
                            print_token_error (data, current_token (tokens, offset), "Missing comma");
                            return false;
                        }
                        next_token (offset);
                        t = current_token (tokens, offset);
                    }

                    if (!is_data_type (data, t)) {
                        print_token_error (data, current_token (tokens, offset), "Parameter not a data type");
                        return false;
                    }
                    data_type = t;
                    next_token (offset);

                    Token *name = current_token (tokens, offset);
                    if (!is_parameter_name (data, name)) {
                        print_token_error (data, current_token (tokens, offset), "Not a parameter name");
                        return false;
                    }
                    next_token (offset);

                    Operation *parameter = make_variable_definition (data_type, name, NULL);

                    parameters_length++;
                    parameters = realloc (parameters, sizeof (Operation *) * (parameters_length + 1));
                    parameters[parameters_length - 1] = parameter;
                    parameters[parameters_length] = NULL;
                }

                if (!closed) {
                    print_token_error (data, current_token (tokens, offset), "Unclosed paren");
                    return false;
                }

                Token *open_brace = current_token (tokens, offset);
                if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
                    print_token_error (data, current_token (tokens, offset), "Missing function open brace");
                    return false;
                }
                next_token (offset);

                op = make_function_definition (data_type, name, parameters);
                if (!parse_function_body ((OperationFunctionDefinition *) op, data, tokens, offset))
                    return false;
            } else {
                op = make_variable_definition (data_type, name, NULL);
            }
        }
        else if (token_has_text (data, token, "return")) {
            next_token (offset);

            Operation *value = parse_expression (function, data, tokens, offset);
            if (value == NULL) {
                print_token_error (data, current_token (tokens, offset), "Not valid return value");
                return false;
            }

            op = make_return (value);
        }
        else if (is_variable (function, data, token)) {
            Token *name = token;
            next_token (offset);

            Token *assignment_token = current_token (tokens, offset);
            if (assignment_token->type != TOKEN_TYPE_ASSIGN) {
                 print_token_error (data, current_token (tokens, offset), "Missing assignment token");
                 return false;
            }
            next_token (offset);

            Operation *value = parse_expression (function, data, tokens, offset);
            if (value == NULL) {
                 print_token_error (data, current_token (tokens, offset), "Invalid value for variable");
                 return false;
            }

            op = make_variable_assignment (name, value);
        }
        else if ((op = parse_value (function, data, tokens, offset)) != NULL) {
            // FIXME: Only allow functions and variable values
        }
        else {
            print_token_error (data, current_token (tokens, offset), "Unexpected token");
            return false;
        }

        body_length++;
        function->body = realloc (function->body, sizeof (Operation *) * (body_length + 1));
        function->body[body_length - 1] = op;
        function->body[body_length] = NULL;
    }

    if (function->name != NULL) {
        print_token_error (data, current_token (tokens, offset), "Missing close brace");
        return false;
    }

    return true;
}

OperationFunctionDefinition *
elf_parse (const char *data, size_t data_length)
{
    Token **tokens = elf_lex (data, data_length);

    Operation **parameters = malloc (sizeof (Operation *));
    parameters[0] = NULL;
    OperationFunctionDefinition *main_function = (OperationFunctionDefinition *) make_function_definition (NULL, NULL, parameters);

    size_t offset = 0;
    if (!parse_function_body (main_function, data, tokens, &offset)) {
        operation_free ((Operation *) main_function);
        tokens_free (tokens);
        return NULL;
    }

    // FIXME: Need to keep these - they are referred to by the operations
    //tokens_free (tokens);

    return main_function;
}
