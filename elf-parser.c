/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

typedef struct {
    Operation *operation;

    OperationVariableDefinition **variables;
    size_t variables_length;
} StackFrame;

static StackFrame *
stack_frame_new (Operation *operation)
{
    StackFrame *frame = malloc (sizeof (StackFrame));
    memset (frame, 0, sizeof (StackFrame));
    frame->operation = operation;

    return frame;
}

static void
stack_frame_free (StackFrame *frame)
{
    free (frame->variables);
    free (frame);
}

typedef struct {
    const char *data;
    size_t data_length;

    Token **tokens;
    size_t offset;

    StackFrame **stack;
    size_t stack_length;
} Parser;

static Parser *
parser_new (const char *data, size_t data_length)
{
    Parser *parser = malloc (sizeof (Parser));
    memset (parser, 0, sizeof (Parser));
    parser->data = data;
    parser->data_length = data_length;

    return parser;
}

static void
parser_free (Parser *parser)
{
    // FIXME: Need to keep these - they are referred to by the operations
    //for (int i = 0; parser->tokens[i] != NULL; i++)
    //  free (parser->tokens[i]);
    //free (parser->tokens);

    for (size_t i = 0; i < parser->stack_length; i++)
       stack_frame_free (parser->stack[i]);
    free (parser->stack);

    free (parser);
}

static void
push_stack (Parser *parser, Operation *operation)
{
    parser->stack_length++;
    parser->stack = realloc (parser->stack, sizeof (StackFrame *) * parser->stack_length);
    parser->stack[parser->stack_length - 1] = stack_frame_new (operation);
}

static void
add_stack_variable (Parser *parser, OperationVariableDefinition *definition)
{
    StackFrame *frame = parser->stack[parser->stack_length - 1];

    frame->variables_length++;
    frame->variables = realloc (frame->variables, sizeof (OperationVariableDefinition *) * frame->variables_length);
    frame->variables[frame->variables_length - 1] = definition;
}

static void
pop_stack (Parser *parser)
{
    stack_frame_free (parser->stack[parser->stack_length - 1]);
    parser->stack_length--;
    parser->stack = realloc (parser->stack, sizeof (StackFrame *) * parser->stack_length);
}

static void
print_token_error (Parser *parser, Token *token, const char *message)
{
    size_t line_offset = 0;
    size_t line_number = 1;
    for (size_t i = 0; i < token->offset; i++) {
        if (parser->data[i] == '\n') {
            line_offset = i + 1;
            line_number++;
        }
    }

    printf ("Line %zi:\n", line_number);
    for (size_t i = line_offset; parser->data[i] != '\0' && parser->data[i] != '\n'; i++)
        printf ("%c", parser->data[i]);
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

static bool
token_is_complete (const char *data, Token *token, char next_c)
{
   switch (token->type) {
   case TOKEN_TYPE_COMMENT:
       return next_c == '\n' || next_c == '\0';
   case TOKEN_TYPE_WORD:
       return !is_symbol_char (next_c);
   case TOKEN_TYPE_MEMBER:
       return !is_symbol_char (next_c);
   case TOKEN_TYPE_NUMBER:
       return !is_number_char (next_c);
   case TOKEN_TYPE_TEXT:
       return string_is_complete (data, token);
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
            else if (c == '!')
                token->type = TOKEN_TYPE_NOT;
            else if (c == '<')
                token->type = TOKEN_TYPE_LESS;
            else if (c == '>')
                token->type = TOKEN_TYPE_GREATER;
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
            else if (c == '.') // FIXME: Don't allow whitespace before it?
                token->type = TOKEN_TYPE_MEMBER;
            else if (is_symbol_char (c))
                token->type = TOKEN_TYPE_WORD;

            current_token = token;
        }
        else {
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

    return tokens;
}

static bool
is_data_type (Parser *parser, Token *token)
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
        if (token_has_text (token, parser->data, builtin_types[i]))
            return true;

    return false;
}

static bool
is_parameter_name (Parser *parser, Token *token)
{
    if (token->type != TOKEN_TYPE_WORD)
        return false;

    // FIXME: Don't allow reserved words / data types

    return true;
}

static bool
token_text_matches (Parser *parser, Token *a, Token *b)
{
    if (a->length != b->length)
        return false;

    for (size_t i = 0; i < a->length; i++)
        if (parser->data[a->offset + i] != parser->data[b->offset + i])
            return false;

    return true;
}

static bool
is_boolean (Parser *parser, Token *token)
{
    return token_has_text (token, parser->data, "true") || token_has_text (token, parser->data, "false");
}

static OperationVariableDefinition *
find_variable (Parser *parser, Token *token)
{
    if (token->type != TOKEN_TYPE_WORD)
        return NULL;

    for (size_t i = parser->stack_length; i > 0; i--) {
        StackFrame *frame = parser->stack[i - 1];

        for (size_t j = 0; j < frame->variables_length; j++) {
            OperationVariableDefinition *definition = frame->variables[j];

            if (token_text_matches (parser, definition->name, token))
                return definition;
        }
    }

    return NULL;
}

static OperationFunctionDefinition *
find_function (Parser *parser, Token *token)
{
    if (token->type != TOKEN_TYPE_WORD)
        return NULL;

    for (size_t i = parser->stack_length; i > 0; i--) {
        Operation *operation = parser->stack[i - 1]->operation;

        size_t n_children = operation_get_n_children (operation);
        for (size_t j = 0; j < n_children; j++) {
            Operation *child = operation_get_child (operation, j);

            if (child->type != OPERATION_TYPE_FUNCTION_DEFINITION)
               continue;

            OperationFunctionDefinition *op = (OperationFunctionDefinition *) child;
            if (token_text_matches (parser, op->name, token))
                return op;
        }
    }

    return NULL;
}

static bool
is_builtin_function (Parser *parser, Token *token)
{
    const char *builtin_functions[] = { "print",
                                        NULL };

    if (token->type != TOKEN_TYPE_WORD)
        return false;

    for (int i = 0; builtin_functions[i] != NULL; i++)
        if (token_has_text (token, parser->data, builtin_functions[i]))
            return true;

    return false;
}

static bool
has_member (Parser *parser, Operation *value, Token *member)
{
    autofree_str data_type = operation_get_data_type (value, parser->data);

    // FIXME: Super hacky. The NULL is because Elf can't determine the return value of a member yet
    // i.e. "foo".upper.length
    if (data_type == NULL || str_equal (data_type, "utf8")) {
        return token_has_text (member, parser->data, ".length") ||
               token_has_text (member, parser->data, ".upper") ||
               token_has_text (member, parser->data, ".lower");
    }

    return false;
}

static Token *
current_token (Parser *parser)
{
    return parser->tokens[parser->offset];
}

static void
next_token (Parser *parser)
{
    parser->offset++;
}

static Operation *parse_expression (Parser *parser);

static Operation **
parse_parameters (Parser *parser)
{
    Operation **parameters = malloc (sizeof (Operation *));
    size_t parameters_length = 0;
    parameters[0] = NULL;

    Token *open_paren_token = current_token (parser);
    if (open_paren_token == NULL || open_paren_token->type != TOKEN_TYPE_OPEN_PAREN)
        return parameters;
    next_token (parser);

    bool closed = false;
    while (current_token (parser) != NULL) {
        Token *t = current_token (parser);
        if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
            next_token (parser);
            closed = true;
            break;
        }

        if (parameters_length > 0) {
            if (t->type != TOKEN_TYPE_COMMA) {
                print_token_error (parser, current_token (parser), "Missing comma");
                return NULL;
            }
            next_token (parser);
            t = current_token (parser);
        }

        Operation *value = parse_expression (parser);
        if (value == NULL) {
            print_token_error (parser, current_token (parser), "Invalid parameter");
            return NULL;
        }

        parameters_length++;
        parameters = realloc (parameters, sizeof (Operation *) * (parameters_length + 1));
        parameters[parameters_length - 1] = value;
        parameters[parameters_length] = NULL;
    }

    if (!closed) {
        print_token_error (parser, current_token (parser), "Unclosed paren");
        return NULL;
    }

    return parameters;
}

static Operation *
parse_value (Parser *parser)
{
    Token *token = current_token (parser);
    if (token == NULL)
        return NULL;

    OperationFunctionDefinition *f;
    OperationVariableDefinition *v;
    Operation *value = NULL;
    if (token->type == TOKEN_TYPE_NUMBER) {
        next_token (parser);
        value = make_number_constant (token);
    }
    else if (token->type == TOKEN_TYPE_TEXT) {
        next_token (parser);
        value = make_text_constant (token);
    }
    else if (is_boolean (parser, token)) {
        next_token (parser);
        value = make_boolean_constant (token);
    }
    else if ((v = find_variable (parser, token)) != NULL) {
        next_token (parser);
        value = make_variable_value (token, v);
    }
    else if ((f = find_function (parser, token)) != NULL || is_builtin_function (parser, token)) {
        Token *name = token;
        next_token (parser);

        Operation **parameters = parse_parameters (parser);
        if (parameters == NULL)
            return NULL;

        value = make_function_call (name, parameters, f);
    }

    if (value == NULL)
        return NULL;

    token = current_token (parser);
    while (token != NULL && token->type == TOKEN_TYPE_MEMBER) {
        if (!has_member (parser, value, token)) {
            print_token_error (parser, token, "Member not available");
            return NULL;
        }
        next_token (parser);

        Operation **parameters = parse_parameters (parser);
        if (parameters == NULL)
            return NULL;

        value = make_member_value (value, token, parameters);
        token = current_token (parser);
    }

    return value;
}

static bool
token_is_binary_operator (Token *token)
{
    return token->type == TOKEN_TYPE_EQUAL ||
           token->type == TOKEN_TYPE_NOT_EQUAL ||
           token->type == TOKEN_TYPE_GREATER ||
           token->type == TOKEN_TYPE_GREATER_EQUAL ||
           token->type == TOKEN_TYPE_LESS ||
           token->type == TOKEN_TYPE_LESS_EQUAL ||
           token->type == TOKEN_TYPE_ADD ||
           token->type == TOKEN_TYPE_SUBTRACT ||
           token->type == TOKEN_TYPE_MULTIPLY ||
           token->type == TOKEN_TYPE_DIVIDE;
}

static Operation *
parse_expression (Parser *parser)
{
    Operation *a = parse_value (parser);
    if (a == NULL)
        return NULL;

    Token *operator = current_token (parser);
    if (operator == NULL)
        return a;

    if (!token_is_binary_operator (operator))
        return a;
    next_token (parser);

    Operation *b = parse_value (parser);
    if (b == NULL) {
        print_token_error (parser, current_token (parser), "Missing second value in binary operation");
        operation_free (a);
        return NULL;
    }

    return make_binary (operator, a, b);
}

static OperationFunctionDefinition *
get_current_function (Parser *parser)
{
   for (size_t i = parser->stack_length; i > 0; i--) {
       if (parser->stack[i - 1]->operation->type == OPERATION_TYPE_FUNCTION_DEFINITION)
           return (OperationFunctionDefinition *) parser->stack[i - 1]->operation;
   }

   return NULL;
}

static bool
can_assign (const char *variable_type, const char *value_type)
{
    if (str_equal (variable_type, value_type))
        return true;

    // Integers that can fit inside their parent type
    if (str_equal (variable_type, "uint16"))
        return str_equal (value_type, "uint8");
    else if (str_equal (variable_type, "uint32"))
        return str_equal (value_type, "uint16") ||
               str_equal (value_type, "uint8");
    else if (str_equal (variable_type, "uint64"))
        return str_equal (value_type, "uint32") ||
               str_equal (value_type, "uint16") ||
               str_equal (value_type, "uint8");
    else
        return false;
}

static bool
parse_sequence (Parser *parser)
{
    Operation *parent = parser->stack[parser->stack_length - 1]->operation;

    while (current_token (parser) != NULL) {
        Token *token = current_token (parser);

        // Stop when sequence ends
        if (token->type == TOKEN_TYPE_CLOSE_BRACE)
            break;

        // Ignore comments
        if (token->type == TOKEN_TYPE_COMMENT) {
            next_token (parser);
            continue;
        }

        Operation *op = NULL;
        OperationVariableDefinition *v;
        if (is_data_type (parser, token)) {
            Token *data_type = token;
            next_token (parser);

            Token *name = current_token (parser); // FIXME: Check valid name
            next_token (parser);

            Token *assignment_token = current_token (parser);
            if (assignment_token == NULL) {
                op = make_variable_definition (data_type, name, NULL);
                add_stack_variable (parser, (OperationVariableDefinition *) op);
            } else if (assignment_token->type == TOKEN_TYPE_ASSIGN) {
                next_token (parser);

                Operation *value = parse_expression (parser);
                if (value == NULL) {
                    print_token_error (parser, current_token (parser), "Invalid value for variable");
                    return false;
                }

                autofree_str variable_type = token_get_text (data_type, parser->data);
                autofree_str value_type = operation_get_data_type (value, parser->data);
                if (!can_assign (variable_type, value_type)) {
                    autofree_str message = str_printf ("Variable is of type %s, but value is of type %s", variable_type, value_type);
                    print_token_error (parser, name, message);
                    return false;
                }

                op = make_variable_definition (data_type, name, value);
                add_stack_variable (parser, (OperationVariableDefinition *) op);
            } else if (assignment_token->type == TOKEN_TYPE_OPEN_PAREN) {
                next_token (parser);

                OperationVariableDefinition **parameters = malloc (sizeof (OperationVariableDefinition *));
                size_t parameters_length = 0;
                parameters[0] = NULL;
                bool closed = false;
                while (current_token (parser) != NULL) {
                    Token *t = current_token (parser);
                    if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
                        next_token (parser);
                        closed = true;
                        break;
                    }

                    if (parameters_length > 0) {
                        if (t->type != TOKEN_TYPE_COMMA) {
                            print_token_error (parser, current_token (parser), "Missing comma");
                            return false;
                        }
                        next_token (parser);
                        t = current_token (parser);
                    }

                    if (!is_data_type (parser, t)) {
                        print_token_error (parser, current_token (parser), "Parameter not a data type");
                        return false;
                    }
                    data_type = t;
                    next_token (parser);

                    Token *name = current_token (parser);
                    if (!is_parameter_name (parser, name)) {
                        print_token_error (parser, current_token (parser), "Not a parameter name");
                        return false;
                    }
                    next_token (parser);

                    Operation *parameter = make_variable_definition (data_type, name, NULL);

                    parameters_length++;
                    parameters = realloc (parameters, sizeof (OperationVariableDefinition *) * (parameters_length + 1));
                    parameters[parameters_length - 1] = (OperationVariableDefinition *) parameter;
                    parameters[parameters_length] = NULL;
                }

                if (!closed) {
                    print_token_error (parser, current_token (parser), "Unclosed paren");
                    return false;
                }

                Token *open_brace = current_token (parser);
                if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
                    print_token_error (parser, current_token (parser), "Missing function open brace");
                    return false;
                }
                next_token (parser);

                op = make_function_definition (data_type, name, parameters);
                push_stack (parser, op);

                for (size_t i = 0; i < parameters_length; i++)
                    add_stack_variable (parser, parameters[i]);

                if (!parse_sequence (parser))
                    return false;

                Token *close_brace = current_token (parser);
                if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
                    print_token_error (parser, current_token (parser), "Missing function close brace");
                    return false;
                }
                next_token (parser);

                pop_stack (parser);
            } else {
                op = make_variable_definition (data_type, name, NULL);
                add_stack_variable (parser, (OperationVariableDefinition *) op);
            }
        }
        else if (token_has_text (token, parser->data, "if")) {
            next_token (parser);

            Operation *condition = parse_expression (parser);
            if (condition == NULL) {
                print_token_error (parser, current_token (parser), "Not valid if condition");
                return false;
            }

            Token *open_brace = current_token (parser);
            if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
                print_token_error (parser, current_token (parser), "Missing if open brace");
                return false;
            }
            next_token (parser);

            op = make_if (condition);
            push_stack (parser, op);
            if (!parse_sequence (parser))
                return false;

            Token *close_brace = current_token (parser);
            if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
                print_token_error (parser, current_token (parser), "Missing if close brace");
                return false;
            }
            next_token (parser);

            pop_stack (parser);
        }
        else if (token_has_text (token, parser->data, "else")) {
            Operation *last_operation = operation_get_last_child (parent);
            if (last_operation == NULL || last_operation->type != OPERATION_TYPE_IF) {
                print_token_error (parser, current_token (parser), "else must follow if");
                return false;
            }
            OperationIf *if_operation = (OperationIf *) last_operation;

            next_token (parser);

            Token *open_brace = current_token (parser);
            if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
                print_token_error (parser, current_token (parser), "Missing else open brace");
                return false;
            }
            next_token (parser);

            op = make_else ();
            if_operation->else_operation = (OperationElse *) op;
            push_stack (parser, op);
            if (!parse_sequence (parser))
                return false;

            Token *close_brace = current_token (parser);
            if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
                print_token_error (parser, current_token (parser), "Missing else close brace");
                return false;
            }
            next_token (parser);

            pop_stack (parser);
        }
        else if (token_has_text (token, parser->data, "while")) {
            next_token (parser);

            Operation *condition = parse_expression (parser);
            if (condition == NULL) {
                print_token_error (parser, current_token (parser), "Not valid while condition");
                return false;
            }

            Token *open_brace = current_token (parser);
            if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
                print_token_error (parser, current_token (parser), "Missing while open brace");
                return false;
            }
            next_token (parser);

            op = make_while (condition);
            push_stack (parser, op);
            if (!parse_sequence (parser))
                return false;

            Token *close_brace = current_token (parser);
            if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
                print_token_error (parser, current_token (parser), "Missing while close brace");
                return false;
            }
            next_token (parser);

            pop_stack (parser);
        }
        else if (token_has_text (token, parser->data, "return")) {
            next_token (parser);

            Operation *value = parse_expression (parser);
            if (value == NULL) {
                print_token_error (parser, current_token (parser), "Not valid return value");
                return false;
            }

            op = make_return (value, get_current_function (parser));
        }
        else if (token_has_text (token, parser->data, "assert")) {
            next_token (parser);

            Operation *expression = parse_expression (parser);
            if (expression == NULL) {
                print_token_error (parser, current_token (parser), "Not valid assertion expression");
                return false;
            }

            op = make_assert (token, expression);
        }
        else if ((v = find_variable (parser, token)) != NULL) {
            Token *name = token;
            next_token (parser);

            Token *assignment_token = current_token (parser);
            if (assignment_token->type != TOKEN_TYPE_ASSIGN) {
                 print_token_error (parser, current_token (parser), "Missing assignment token");
                 return false;
            }
            next_token (parser);

            Operation *value = parse_expression (parser);
            if (value == NULL) {
                 print_token_error (parser, current_token (parser), "Invalid value for variable");
                 return false;
            }

            autofree_str variable_type = operation_get_data_type ((Operation *) v, parser->data);
            autofree_str value_type = operation_get_data_type (value, parser->data);
            if (!can_assign (variable_type, value_type)) {
                 autofree_str message = str_printf ("Variable is of type %s, but value is of type %s", variable_type, value_type);
                 print_token_error (parser, name, message);
                 return false;
            }

            op = make_variable_assignment (name, value, v);
        }
        else if ((op = parse_value (parser)) != NULL) {
            // FIXME: Only allow functions and variable values
        }
        else {
            print_token_error (parser, current_token (parser), "Unexpected token");
            return false;
        }

        operation_add_child (parent, op);
    }

    return true;
}

OperationModule *
elf_parse (const char *data, size_t data_length)
{
    Parser *parser = parser_new (data, data_length);

    parser->tokens = elf_lex (data, data_length);

    Operation *module = make_module ();
    push_stack (parser, module);

    if (!parse_sequence (parser)) {
        operation_free (module);
        parser_free (parser);
        return NULL;
    }

    if (current_token (parser) != NULL) {
        printf ("Expected end of input\n");
        return NULL;
    }

    parser_free (parser);

    return (OperationModule *) module;
}
