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

static StackFrame *stack_frame_new(Operation *operation) {
  StackFrame *frame = new StackFrame;
  memset(frame, 0, sizeof(StackFrame));
  frame->operation = operation;

  return frame;
}

static void stack_frame_free(StackFrame *frame) {
  free(frame->variables);
  delete frame;
}

typedef struct {
  const char *data;
  size_t data_length;

  Token **tokens;
  size_t offset;

  StackFrame **stack;
  size_t stack_length;
} Parser;

static Parser *parser_new(const char *data, size_t data_length) {
  Parser *parser = new Parser;
  memset(parser, 0, sizeof(Parser));
  parser->data = data;
  parser->data_length = data_length;

  return parser;
}

static void parser_free(Parser *parser) {
  for (int i = 0; parser->tokens[i] != nullptr; i++)
    parser->tokens[i]->unref();
  free(parser->tokens);

  for (size_t i = 0; i < parser->stack_length; i++)
    stack_frame_free(parser->stack[i]);
  free(parser->stack);

  delete parser;
}

static void push_stack(Parser *parser, Operation *operation) {
  parser->stack_length++;
  parser->stack = static_cast<StackFrame **>(
      realloc(parser->stack, sizeof(StackFrame *) * parser->stack_length));
  parser->stack[parser->stack_length - 1] = stack_frame_new(operation);
}

static void add_stack_variable(Parser *parser,
                               OperationVariableDefinition *definition) {
  StackFrame *frame = parser->stack[parser->stack_length - 1];

  frame->variables_length++;
  frame->variables = static_cast<OperationVariableDefinition **>(
      realloc(frame->variables,
              sizeof(OperationVariableDefinition *) * frame->variables_length));
  frame->variables[frame->variables_length - 1] = definition;
}

static void pop_stack(Parser *parser) {
  stack_frame_free(parser->stack[parser->stack_length - 1]);
  parser->stack_length--;
  parser->stack = static_cast<StackFrame **>(
      realloc(parser->stack, sizeof(StackFrame *) * parser->stack_length));
}

static void print_token_error(Parser *parser, Token *token,
                              std::string message) {
  size_t line_offset = 0;
  size_t line_number = 1;
  for (size_t i = 0; i < token->offset; i++) {
    if (parser->data[i] == '\n') {
      line_offset = i + 1;
      line_number++;
    }
  }

  printf("Line %zi:\n", line_number);
  for (size_t i = line_offset;
       parser->data[i] != '\0' && parser->data[i] != '\n'; i++)
    printf("%c", parser->data[i]);
  printf("\n");
  for (size_t i = line_offset; i < token->offset; i++)
    printf(" ");
  for (size_t i = 0; i < token->length; i++)
    printf("^");
  printf("\n");
  printf("%s\n", message.c_str());
}

static bool is_number_char(char c) { return c >= '0' && c <= '9'; }

static bool is_symbol_char(char c) {
  return is_number_char(c) || (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') || c == '_';
}

static char string_is_complete(const char *data, Token *token) {
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

static bool token_is_complete(const char *data, Token *token, char next_c) {
  switch (token->type) {
  case TOKEN_TYPE_COMMENT:
    return next_c == '\n' || next_c == '\0';
  case TOKEN_TYPE_WORD:
    return !is_symbol_char(next_c);
  case TOKEN_TYPE_MEMBER:
    return !is_symbol_char(next_c);
  case TOKEN_TYPE_NUMBER:
    return !is_number_char(next_c);
  case TOKEN_TYPE_TEXT:
    return string_is_complete(data, token);
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

static Token **elf_lex(const char *data, size_t data_length) {
  Token **tokens = nullptr;
  size_t tokens_length = 0;
  Token *current_token = nullptr;

  tokens = static_cast<Token **>(malloc(sizeof(Token *)));
  tokens[0] = nullptr;
  for (size_t offset = 0; offset < data_length; offset++) {
    // FIXME: Support UTF-8
    char c = data[offset];

    if (current_token != nullptr && token_is_complete(data, current_token, c))
      current_token = nullptr;

    if (current_token == nullptr) {
      // Skip whitespace
      if (c == ' ' || c == '\r' || c == '\n')
        continue;

      tokens_length++;
      tokens = static_cast<Token **>(realloc(
          tokens, sizeof(Token *) *
                      (tokens_length + 1))); // FIXME: Double size each time
      tokens[tokens_length] = nullptr;

      TokenType type;
      if (c == '(')
        type = TOKEN_TYPE_OPEN_PAREN;
      else if (c == ')')
        type = TOKEN_TYPE_CLOSE_PAREN;
      else if (c == ',')
        type = TOKEN_TYPE_COMMA;
      else if (c == '{')
        type = TOKEN_TYPE_OPEN_BRACE;
      else if (c == '}')
        type = TOKEN_TYPE_CLOSE_BRACE;
      else if (c == '=')
        type = TOKEN_TYPE_ASSIGN;
      else if (c == '!')
        type = TOKEN_TYPE_NOT;
      else if (c == '<')
        type = TOKEN_TYPE_LESS;
      else if (c == '>')
        type = TOKEN_TYPE_GREATER;
      else if (c == '+')
        type = TOKEN_TYPE_ADD;
      else if (c == '-')
        type = TOKEN_TYPE_SUBTRACT;
      else if (c == '*')
        type = TOKEN_TYPE_MULTIPLY;
      else if (c == '/')
        type = TOKEN_TYPE_DIVIDE;
      else if (is_number_char(c))
        type = TOKEN_TYPE_NUMBER;
      else if (c == '"' || c == '\'')
        type = TOKEN_TYPE_TEXT;
      else if (c == '.') // FIXME: Don't allow whitespace before it?
        type = TOKEN_TYPE_MEMBER;
      else if (is_symbol_char(c))
        type = TOKEN_TYPE_WORD;

      Token *token = tokens[tokens_length - 1] = new Token(type, offset, 1);

      current_token = token;
    } else {
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

static bool is_data_type(Parser *parser, Token *token) {
  const char *builtin_types[] = {"bool",  "uint8",  "int8",  "uint16",
                                 "int16", "uint32", "int32", "uint64",
                                 "int64", "utf8",   nullptr};

  if (token->type != TOKEN_TYPE_WORD)
    return false;

  for (int i = 0; builtin_types[i] != nullptr; i++)
    if (token->has_text(parser->data, builtin_types[i]))
      return true;

  return false;
}

static bool is_parameter_name(Parser *parser, Token *token) {
  if (token->type != TOKEN_TYPE_WORD)
    return false;

  // FIXME: Don't allow reserved words / data types

  return true;
}

static bool token_text_matches(Parser *parser, Token *a, Token *b) {
  if (a->length != b->length)
    return false;

  for (size_t i = 0; i < a->length; i++)
    if (parser->data[a->offset + i] != parser->data[b->offset + i])
      return false;

  return true;
}

static bool is_boolean(Parser *parser, Token *token) {
  return token->has_text(parser->data, "true") ||
         token->has_text(parser->data, "false");
}

static OperationVariableDefinition *find_variable(Parser *parser,
                                                  Token *token) {
  if (token->type != TOKEN_TYPE_WORD)
    return nullptr;

  for (size_t i = parser->stack_length; i > 0; i--) {
    StackFrame *frame = parser->stack[i - 1];

    for (size_t j = 0; j < frame->variables_length; j++) {
      OperationVariableDefinition *definition = frame->variables[j];

      if (token_text_matches(parser, definition->name, token))
        return definition;
    }
  }

  return nullptr;
}

static OperationFunctionDefinition *find_function(Parser *parser,
                                                  Token *token) {
  if (token->type != TOKEN_TYPE_WORD)
    return nullptr;

  for (size_t i = parser->stack_length; i > 0; i--) {
    Operation *operation = parser->stack[i - 1]->operation;

    size_t n_children = operation->get_n_children();
    for (size_t j = 0; j < n_children; j++) {
      Operation *child = operation->get_child(j);

      OperationFunctionDefinition *op =
          dynamic_cast<OperationFunctionDefinition *>(child);
      if (op != nullptr && token_text_matches(parser, op->name, token))
        return op;
    }
  }

  return nullptr;
}

static bool is_builtin_function(Parser *parser, Token *token) {
  const char *builtin_functions[] = {"print", nullptr};

  if (token->type != TOKEN_TYPE_WORD)
    return false;

  for (int i = 0; builtin_functions[i] != nullptr; i++)
    if (token->has_text(parser->data, builtin_functions[i]))
      return true;

  return false;
}

static bool has_member(Parser *parser, Operation *value, Token *member) {
  auto data_type = value->get_data_type(parser->data);

  // FIXME: Super hacky. The nullptr is because Elf can't determine the return
  // value of a member yet i.e. "foo".upper.length
  if (data_type == "" || data_type == "utf8") {
    return member->has_text(parser->data, ".length") ||
           member->has_text(parser->data, ".upper") ||
           member->has_text(parser->data, ".lower");
  }

  return false;
}

static Token *current_token(Parser *parser) {
  return parser->tokens[parser->offset];
}

static void next_token(Parser *parser) { parser->offset++; }

static Operation *parse_expression(Parser *parser);

static Operation **parse_parameters(Parser *parser) {
  Operation **parameters =
      static_cast<Operation **>(malloc(sizeof(Operation *)));
  size_t parameters_length = 0;
  parameters[0] = nullptr;

  Token *open_paren_token = current_token(parser);
  if (open_paren_token == nullptr ||
      open_paren_token->type != TOKEN_TYPE_OPEN_PAREN)
    return parameters;
  next_token(parser);

  bool closed = false;
  while (current_token(parser) != nullptr) {
    Token *t = current_token(parser);
    if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
      next_token(parser);
      closed = true;
      break;
    }

    if (parameters_length > 0) {
      if (t->type != TOKEN_TYPE_COMMA) {
        print_token_error(parser, current_token(parser), "Missing comma");
        return nullptr;
      }
      next_token(parser);
      t = current_token(parser);
    }

    Operation *value = parse_expression(parser);
    if (value == nullptr) {
      print_token_error(parser, current_token(parser), "Invalid parameter");
      return nullptr;
    }

    parameters_length++;
    parameters = static_cast<Operation **>(
        realloc(parameters, sizeof(Operation *) * (parameters_length + 1)));
    parameters[parameters_length - 1] = value;
    parameters[parameters_length] = nullptr;
  }

  if (!closed) {
    print_token_error(parser, current_token(parser), "Unclosed paren");
    return nullptr;
  }

  return parameters;
}

static Operation *parse_value(Parser *parser) {
  Token *token = current_token(parser);
  if (token == nullptr)
    return nullptr;

  OperationFunctionDefinition *f;
  OperationVariableDefinition *v;
  autofree_operation value = nullptr;
  if (token->type == TOKEN_TYPE_NUMBER) {
    next_token(parser);
    value = new OperationNumberConstant(token);
  } else if (token->type == TOKEN_TYPE_TEXT) {
    next_token(parser);
    value = new OperationTextConstant(token);
  } else if (is_boolean(parser, token)) {
    next_token(parser);
    value = new OperationBooleanConstant(token);
  } else if ((v = find_variable(parser, token)) != nullptr) {
    next_token(parser);
    value = new OperationVariableValue(token, v);
  } else if ((f = find_function(parser, token)) != nullptr ||
             is_builtin_function(parser, token)) {
    Token *name = token;
    next_token(parser);

    Operation **parameters = parse_parameters(parser);
    if (parameters == nullptr)
      return nullptr;

    value = new OperationFunctionCall(name, parameters, f);
  }

  if (value == nullptr)
    return nullptr;

  token = current_token(parser);
  while (token != nullptr && token->type == TOKEN_TYPE_MEMBER) {
    if (!has_member(parser, value, token)) {
      print_token_error(parser, token, "Member not available");
      return nullptr;
    }
    next_token(parser);

    Operation **parameters = parse_parameters(parser);
    if (parameters == nullptr)
      return nullptr;

    value = new OperationMemberValue(value, token, parameters);
    token = current_token(parser);
  }

  return value->ref();
}

static bool token_is_binary_operator(Token *token) {
  return token->type == TOKEN_TYPE_EQUAL ||
         token->type == TOKEN_TYPE_NOT_EQUAL ||
         token->type == TOKEN_TYPE_GREATER ||
         token->type == TOKEN_TYPE_GREATER_EQUAL ||
         token->type == TOKEN_TYPE_LESS ||
         token->type == TOKEN_TYPE_LESS_EQUAL ||
         token->type == TOKEN_TYPE_ADD || token->type == TOKEN_TYPE_SUBTRACT ||
         token->type == TOKEN_TYPE_MULTIPLY || token->type == TOKEN_TYPE_DIVIDE;
}

static Operation *parse_expression(Parser *parser) {
  autofree_operation a = parse_value(parser);
  if (a == nullptr)
    return nullptr;

  Token *op = current_token(parser);
  if (op == nullptr)
    return a->ref();

  if (!token_is_binary_operator(op))
    return a->ref();
  next_token(parser);

  autofree_operation b = parse_value(parser);
  if (b == nullptr) {
    print_token_error(parser, current_token(parser),
                      "Missing second value in binary operation");
    return nullptr;
  }

  auto a_type = a->get_data_type(parser->data);
  auto b_type = b->get_data_type(parser->data);
  if (a_type != b_type) {
    print_token_error(parser, op,
                      "Can't combine " + a_type + " and " + b_type + " types");
    return nullptr;
  }

  return new OperationBinary(op, a, b);
}

static OperationFunctionDefinition *get_current_function(Parser *parser) {
  for (size_t i = parser->stack_length; i > 0; i--) {
    OperationFunctionDefinition *op =
        dynamic_cast<OperationFunctionDefinition *>(
            parser->stack[i - 1]->operation);
    if (op != nullptr)
      return op;
  }

  return nullptr;
}

static bool can_assign(std::string variable_type, std::string value_type) {
  if (variable_type == value_type)
    return true;

  // Integers that can fit inside their parent type
  if (variable_type == "uint16")
    return value_type == "uint8";
  else if (variable_type == "uint32")
    return value_type == "uint16" || value_type == "uint8";
  else if (variable_type == "uint64")
    return value_type == "uint32" || value_type == "uint16" ||
           value_type == "uint8";
  else
    return false;
}

static bool parse_sequence(Parser *parser) {
  Operation *parent = parser->stack[parser->stack_length - 1]->operation;

  while (current_token(parser) != nullptr) {
    Token *token = current_token(parser);

    // Stop when sequence ends
    if (token->type == TOKEN_TYPE_CLOSE_BRACE)
      break;

    // Ignore comments
    if (token->type == TOKEN_TYPE_COMMENT) {
      next_token(parser);
      continue;
    }

    autofree_operation op = nullptr;
    OperationVariableDefinition *v;
    if (is_data_type(parser, token)) {
      Token *data_type = token;
      next_token(parser);

      Token *name = current_token(parser); // FIXME: Check valid name
      next_token(parser);

      Token *assignment_token = current_token(parser);
      if (assignment_token == nullptr) {
        op = new OperationVariableDefinition(data_type, name, nullptr);
        add_stack_variable(parser, (OperationVariableDefinition *)op);
      } else if (assignment_token->type == TOKEN_TYPE_ASSIGN) {
        next_token(parser);

        autofree_operation value = parse_expression(parser);
        if (value == nullptr) {
          print_token_error(parser, current_token(parser),
                            "Invalid value for variable");
          return false;
        }

        auto variable_type = data_type->get_text(parser->data);
        auto value_type = value->get_data_type(parser->data);
        if (!can_assign(variable_type, value_type)) {
          auto message = "Variable is of type " + variable_type +
                         ", but value is of type " + value_type;
          print_token_error(parser, name, message);
          return false;
        }

        op = new OperationVariableDefinition(data_type, name, value);
        add_stack_variable(parser, (OperationVariableDefinition *)op);
      } else if (assignment_token->type == TOKEN_TYPE_OPEN_PAREN) {
        next_token(parser);

        OperationVariableDefinition **parameters =
            static_cast<OperationVariableDefinition **>(
                malloc(sizeof(OperationVariableDefinition *)));
        size_t parameters_length = 0;
        parameters[0] = nullptr;
        bool closed = false;
        while (current_token(parser) != nullptr) {
          Token *t = current_token(parser);
          if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
            next_token(parser);
            closed = true;
            break;
          }

          if (parameters_length > 0) {
            if (t->type != TOKEN_TYPE_COMMA) {
              print_token_error(parser, current_token(parser), "Missing comma");
              return false;
            }
            next_token(parser);
            t = current_token(parser);
          }

          if (!is_data_type(parser, t)) {
            print_token_error(parser, current_token(parser),
                              "Parameter not a data type");
            return false;
          }
          data_type = t;
          next_token(parser);

          Token *name = current_token(parser);
          if (!is_parameter_name(parser, name)) {
            print_token_error(parser, current_token(parser),
                              "Not a parameter name");
            return false;
          }
          next_token(parser);

          Operation *parameter =
              new OperationVariableDefinition(data_type, name, nullptr);

          parameters_length++;
          parameters = static_cast<OperationVariableDefinition **>(
              realloc(parameters, sizeof(OperationVariableDefinition *) *
                                      (parameters_length + 1)));
          parameters[parameters_length - 1] =
              (OperationVariableDefinition *)parameter;
          parameters[parameters_length] = nullptr;
        }

        if (!closed) {
          print_token_error(parser, current_token(parser), "Unclosed paren");
          return false;
        }

        Token *open_brace = current_token(parser);
        if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
          print_token_error(parser, current_token(parser),
                            "Missing function open brace");
          return false;
        }
        next_token(parser);

        op = new OperationFunctionDefinition(data_type, name, parameters);
        push_stack(parser, op);

        for (size_t i = 0; i < parameters_length; i++)
          add_stack_variable(parser, parameters[i]);

        if (!parse_sequence(parser))
          return false;

        Token *close_brace = current_token(parser);
        if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
          print_token_error(parser, current_token(parser),
                            "Missing function close brace");
          return false;
        }
        next_token(parser);

        pop_stack(parser);
      } else {
        op = new OperationVariableDefinition(data_type, name, nullptr);
        add_stack_variable(parser, (OperationVariableDefinition *)op);
      }
    } else if (token->has_text(parser->data, "if")) {
      next_token(parser);

      autofree_operation condition = parse_expression(parser);
      if (condition == nullptr) {
        print_token_error(parser, current_token(parser),
                          "Not valid if condition");
        return false;
      }

      Token *open_brace = current_token(parser);
      if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
        print_token_error(parser, current_token(parser),
                          "Missing if open brace");
        return false;
      }
      next_token(parser);

      op = new OperationIf(token, condition);
      push_stack(parser, op);
      if (!parse_sequence(parser))
        return false;

      Token *close_brace = current_token(parser);
      if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
        print_token_error(parser, current_token(parser),
                          "Missing if close brace");
        return false;
      }
      next_token(parser);

      pop_stack(parser);
    } else if (token->has_text(parser->data, "else")) {
      Operation *last_operation = parent->get_last_child();
      OperationIf *if_operation = dynamic_cast<OperationIf *>(last_operation);
      if (if_operation == nullptr) {
        print_token_error(parser, current_token(parser), "else must follow if");
        return false;
      }

      next_token(parser);

      Token *open_brace = current_token(parser);
      if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
        print_token_error(parser, current_token(parser),
                          "Missing else open brace");
        return false;
      }
      next_token(parser);

      op = new OperationElse(token);
      if_operation->else_operation = (OperationElse *)op;
      push_stack(parser, op);
      if (!parse_sequence(parser))
        return false;

      Token *close_brace = current_token(parser);
      if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
        print_token_error(parser, current_token(parser),
                          "Missing else close brace");
        return false;
      }
      next_token(parser);

      pop_stack(parser);
    } else if (token->has_text(parser->data, "while")) {
      next_token(parser);

      autofree_operation condition = parse_expression(parser);
      if (condition == nullptr) {
        print_token_error(parser, current_token(parser),
                          "Not valid while condition");
        return false;
      }

      Token *open_brace = current_token(parser);
      if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
        print_token_error(parser, current_token(parser),
                          "Missing while open brace");
        return false;
      }
      next_token(parser);

      op = new OperationWhile(condition);
      push_stack(parser, op);
      if (!parse_sequence(parser))
        return false;

      Token *close_brace = current_token(parser);
      if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
        print_token_error(parser, current_token(parser),
                          "Missing while close brace");
        return false;
      }
      next_token(parser);

      pop_stack(parser);
    } else if (token->has_text(parser->data, "return")) {
      next_token(parser);

      Operation *value = parse_expression(parser);
      if (value == nullptr) {
        print_token_error(parser, current_token(parser),
                          "Not valid return value");
        return false;
      }

      op = new OperationReturn(value, get_current_function(parser));
    } else if (token->has_text(parser->data, "assert")) {
      next_token(parser);

      autofree_operation expression = parse_expression(parser);
      if (expression == nullptr) {
        print_token_error(parser, current_token(parser),
                          "Not valid assertion expression");
        return false;
      }

      op = new OperationAssert(token, expression);
    } else if ((v = find_variable(parser, token)) != nullptr) {
      Token *name = token;
      next_token(parser);

      Token *assignment_token = current_token(parser);
      if (assignment_token->type != TOKEN_TYPE_ASSIGN) {
        print_token_error(parser, current_token(parser),
                          "Missing assignment token");
        return false;
      }
      next_token(parser);

      autofree_operation value = parse_expression(parser);
      if (value == nullptr) {
        print_token_error(parser, current_token(parser),
                          "Invalid value for variable");
        return false;
      }

      auto variable_type = v->get_data_type(parser->data);
      auto value_type = value->get_data_type(parser->data);
      if (!can_assign(variable_type, value_type)) {
        print_token_error(parser, name,
                          "Variable is of type " + variable_type +
                              ", but value is of type " + value_type.c_str());
        return false;
      }

      op = new OperationVariableAssignment(name, value, v);
    } else if ((op = parse_value(parser)) != nullptr) {
      // FIXME: Only allow functions and variable values
    } else {
      print_token_error(parser, current_token(parser), "Unexpected token");
      return false;
    }

    parent->add_child(op);
  }

  return true;
}

OperationModule *elf_parse(const char *data, size_t data_length) {
  Parser *parser = parser_new(data, data_length);

  parser->tokens = elf_lex(data, data_length);

  autofree_operation module = new OperationModule();
  push_stack(parser, module);

  if (!parse_sequence(parser)) {
    parser_free(parser);
    return nullptr;
  }

  if (current_token(parser) != nullptr) {
    printf("Expected end of input\n");
    return nullptr;
  }

  parser_free(parser);

  return static_cast<OperationModule *>(module->ref());
}
