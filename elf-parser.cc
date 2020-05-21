/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-parser.h"

#include <stdio.h>
#include <vector>

struct StackFrame {
  Operation *operation;

  std::vector<OperationVariableDefinition *> variables;

  StackFrame(Operation *operation) : operation(operation) {}
};

struct Parser {
  const char *data;
  size_t data_length;

  std::vector<Token *> tokens;
  size_t offset;

  std::vector<StackFrame *> stack;

  Parser(const char *data, size_t data_length)
      : data(data), data_length(data_length), offset(0) {}

  void push_stack(Operation *operation);
  void add_stack_variable(OperationVariableDefinition *definition);
  void pop_stack();
  void print_token_error(Token *token, std::string message);
  bool is_data_type(Token *token);
  bool is_parameter_name(Token *token);
  bool token_text_matches(Token *a, Token *b);
  bool is_boolean(Token *token);
  OperationVariableDefinition *find_variable(Token *token);
  OperationFunctionDefinition *find_function(Token *token);
  bool is_builtin_function(Token *token);
  bool has_member(Operation *value, Token *member);
  Token *current_token();
  void next_token();
  bool parse_parameters(std::vector<Operation *> &parameters);
  Operation *parse_value();
  Operation *parse_expression();
  OperationFunctionDefinition *get_current_function();
  bool parse_sequence();
};

void Parser::push_stack(Operation *operation) {
  stack.push_back(new StackFrame(operation));
}

void Parser::add_stack_variable(OperationVariableDefinition *definition) {
  StackFrame *frame = stack.back();
  frame->variables.push_back(definition);
}

void Parser::pop_stack() { stack.pop_back(); }

void Parser::print_token_error(Token *token, std::string message) {
  size_t line_offset = 0;
  size_t line_number = 1;
  for (size_t i = 0; i < token->offset; i++) {
    if (data[i] == '\n') {
      line_offset = i + 1;
      line_number++;
    }
  }

  printf("Line %zi:\n", line_number);
  for (size_t i = line_offset; data[i] != '\0' && data[i] != '\n'; i++)
    printf("%c", data[i]);
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

static std::vector<Token *> elf_lex(const char *data, size_t data_length) {
  std::vector<Token *> tokens;
  Token *current_token = nullptr;

  for (size_t offset = 0; offset < data_length; offset++) {
    // FIXME: Support UTF-8
    char c = data[offset];

    if (current_token != nullptr && token_is_complete(data, current_token, c))
      current_token = nullptr;

    if (current_token == nullptr) {
      // Skip whitespace
      if (c == ' ' || c == '\r' || c == '\n')
        continue;

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

      Token *token = new Token(type, offset, 1);
      tokens.push_back(token);

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

bool Parser::is_data_type(Token *token) {
  const char *builtin_types[] = {"bool",  "uint8",  "int8",  "uint16",
                                 "int16", "uint32", "int32", "uint64",
                                 "int64", "utf8",   nullptr};

  if (token->type != TOKEN_TYPE_WORD)
    return false;

  for (int i = 0; builtin_types[i] != nullptr; i++)
    if (token->has_text(data, builtin_types[i]))
      return true;

  return false;
}

bool Parser::is_parameter_name(Token *token) {
  if (token->type != TOKEN_TYPE_WORD)
    return false;

  // FIXME: Don't allow reserved words / data types

  return true;
}

bool Parser::token_text_matches(Token *a, Token *b) {
  if (a->length != b->length)
    return false;

  for (size_t i = 0; i < a->length; i++)
    if (data[a->offset + i] != data[b->offset + i])
      return false;

  return true;
}

bool Parser::is_boolean(Token *token) {
  return token->has_text(data, "true") || token->has_text(data, "false");
}

OperationVariableDefinition *Parser::find_variable(Token *token) {
  if (token->type != TOKEN_TYPE_WORD)
    return nullptr;

  for (auto i = stack.rbegin(); i != stack.rend(); i++) {
    StackFrame *frame = *i;

    for (auto j = frame->variables.begin(); j != frame->variables.end(); j++) {
      OperationVariableDefinition *definition = *j;

      if (token_text_matches(definition->name, token))
        return definition;
    }
  }

  return nullptr;
}

OperationFunctionDefinition *Parser::find_function(Token *token) {
  if (token->type != TOKEN_TYPE_WORD)
    return nullptr;

  for (auto i = stack.rbegin(); i != stack.rend(); i++) {
    Operation *operation = (*i)->operation;

    size_t n_children = operation->get_n_children();
    for (size_t j = 0; j < n_children; j++) {
      Operation *child = operation->get_child(j);

      OperationFunctionDefinition *op =
          dynamic_cast<OperationFunctionDefinition *>(child);
      if (op != nullptr && token_text_matches(op->name, token))
        return op;
    }
  }

  return nullptr;
}

bool Parser::is_builtin_function(Token *token) {
  const char *builtin_functions[] = {"print", nullptr};

  if (token->type != TOKEN_TYPE_WORD)
    return false;

  for (int i = 0; builtin_functions[i] != nullptr; i++)
    if (token->has_text(data, builtin_functions[i]))
      return true;

  return false;
}

bool Parser::has_member(Operation *value, Token *member) {
  auto data_type = value->get_data_type(data);

  // FIXME: Super hacky. The nullptr is because Elf can't determine the return
  // value of a member yet i.e. "foo".upper.length
  if (data_type == "" || data_type == "utf8") {
    return member->has_text(data, ".length") ||
           member->has_text(data, ".upper") || member->has_text(data, ".lower");
  }

  return false;
}

Token *Parser::current_token() {
  return offset < tokens.size() ? tokens[offset] : 0;
}

void Parser::next_token() { offset++; }

bool Parser::parse_parameters(std::vector<Operation *> &parameters) {

  Token *open_paren_token = current_token();
  if (open_paren_token == nullptr ||
      open_paren_token->type != TOKEN_TYPE_OPEN_PAREN)
    return true;
  next_token();

  bool closed = false;
  while (current_token() != nullptr) {
    Token *t = current_token();
    if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
      next_token();
      closed = true;
      break;
    }

    if (parameters.size() > 0) {
      if (t->type != TOKEN_TYPE_COMMA) {
        print_token_error(current_token(), "Missing comma");
        return false;
      }
      next_token();
      t = current_token();
    }

    Operation *value = parse_expression();
    if (value == nullptr) {
      print_token_error(current_token(), "Invalid parameter");
      return false;
    }

    parameters.push_back(value);
  }

  if (!closed) {
    print_token_error(current_token(), "Unclosed paren");
    return false;
  }

  return true;
}

Operation *Parser::parse_value() {
  Token *token = current_token();
  if (token == nullptr)
    return nullptr;

  OperationFunctionDefinition *f;
  OperationVariableDefinition *v;
  autofree_operation value = nullptr;
  if (token->type == TOKEN_TYPE_NUMBER) {
    next_token();
    value = new OperationNumberConstant(token);
  } else if (token->type == TOKEN_TYPE_TEXT) {
    next_token();
    value = new OperationTextConstant(token);
  } else if (is_boolean(token)) {
    next_token();
    value = new OperationBooleanConstant(token);
  } else if ((v = find_variable(token)) != nullptr) {
    next_token();
    value = new OperationVariableValue(token, v);
  } else if ((f = find_function(token)) != nullptr ||
             is_builtin_function(token)) {
    Token *name = token;
    next_token();

    std::vector<Operation *> parameters;
    if (!parse_parameters(parameters))
      return nullptr;

    value = new OperationFunctionCall(name, parameters, f);
  }

  if (value == nullptr)
    return nullptr;

  token = current_token();
  while (token != nullptr && token->type == TOKEN_TYPE_MEMBER) {
    if (!has_member(value, token)) {
      print_token_error(token, "Member not available");
      return nullptr;
    }
    next_token();

    std::vector<Operation *> parameters;
    if (!parse_parameters(parameters))
      return nullptr;

    value = new OperationMemberValue(value, token, parameters);
    token = current_token();
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

Operation *Parser::parse_expression() {
  autofree_operation a = parse_value();
  if (a == nullptr)
    return nullptr;

  Token *op = current_token();
  if (op == nullptr)
    return a->ref();

  if (!token_is_binary_operator(op))
    return a->ref();
  next_token();

  autofree_operation b = parse_value();
  if (b == nullptr) {
    print_token_error(current_token(),
                      "Missing second value in binary operation");
    return nullptr;
  }

  auto a_type = a->get_data_type(data);
  auto b_type = b->get_data_type(data);
  if (a_type != b_type) {
    print_token_error(op,
                      "Can't combine " + a_type + " and " + b_type + " types");
    return nullptr;
  }

  return new OperationBinary(op, a, b);
}

OperationFunctionDefinition *Parser::get_current_function() {
  for (auto i = stack.rbegin(); i != stack.rend(); i++) {
    OperationFunctionDefinition *op =
        dynamic_cast<OperationFunctionDefinition *>((*i)->operation);
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

bool Parser::parse_sequence() {
  Operation *parent = stack.back()->operation;

  while (current_token() != nullptr) {
    Token *token = current_token();

    // Stop when sequence ends
    if (token->type == TOKEN_TYPE_CLOSE_BRACE)
      break;

    // Ignore comments
    if (token->type == TOKEN_TYPE_COMMENT) {
      next_token();
      continue;
    }

    autofree_operation op = nullptr;
    OperationVariableDefinition *v;
    if (is_data_type(token)) {
      Token *data_type = token;
      next_token();

      Token *name = current_token(); // FIXME: Check valid name
      next_token();

      Token *assignment_token = current_token();
      if (assignment_token == nullptr) {
        op = new OperationVariableDefinition(data_type, name, nullptr);
        add_stack_variable((OperationVariableDefinition *)op);
      } else if (assignment_token->type == TOKEN_TYPE_ASSIGN) {
        next_token();

        autofree_operation value = parse_expression();
        if (value == nullptr) {
          print_token_error(current_token(), "Invalid value for variable");
          return false;
        }

        auto variable_type = data_type->get_text(data);
        auto value_type = value->get_data_type(data);
        if (!can_assign(variable_type, value_type)) {
          auto message = "Variable is of type " + variable_type +
                         ", but value is of type " + value_type;
          print_token_error(name, message);
          return false;
        }

        op = new OperationVariableDefinition(data_type, name, value);
        add_stack_variable((OperationVariableDefinition *)op);
      } else if (assignment_token->type == TOKEN_TYPE_OPEN_PAREN) {
        next_token();

        std::vector<OperationVariableDefinition *> parameters;
        bool closed = false;
        while (current_token() != nullptr) {
          Token *t = current_token();
          if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
            next_token();
            closed = true;
            break;
          }

          if (parameters.size() > 0) {
            if (t->type != TOKEN_TYPE_COMMA) {
              print_token_error(current_token(), "Missing comma");
              return false;
            }
            next_token();
            t = current_token();
          }

          if (!is_data_type(t)) {
            print_token_error(current_token(), "Parameter not a data type");
            return false;
          }
          data_type = t;
          next_token();

          Token *name = current_token();
          if (!is_parameter_name(name)) {
            print_token_error(current_token(), "Not a parameter name");
            return false;
          }
          next_token();

          parameters.push_back(
              new OperationVariableDefinition(data_type, name, nullptr));
        }

        if (!closed) {
          print_token_error(current_token(), "Unclosed paren");
          return false;
        }

        Token *open_brace = current_token();
        if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
          print_token_error(current_token(), "Missing function open brace");
          return false;
        }
        next_token();

        op = new OperationFunctionDefinition(data_type, name, parameters);
        push_stack(op);

        for (auto i = parameters.begin(); i != parameters.end(); i++)
          add_stack_variable(*i);

        if (!parse_sequence())
          return false;

        Token *close_brace = current_token();
        if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
          print_token_error(current_token(), "Missing function close brace");
          return false;
        }
        next_token();

        pop_stack();
      } else {
        op = new OperationVariableDefinition(data_type, name, nullptr);
        add_stack_variable((OperationVariableDefinition *)op);
      }
    } else if (token->has_text(data, "if")) {
      next_token();

      autofree_operation condition = parse_expression();
      if (condition == nullptr) {
        print_token_error(current_token(), "Not valid if condition");
        return false;
      }

      Token *open_brace = current_token();
      if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
        print_token_error(current_token(), "Missing if open brace");
        return false;
      }
      next_token();

      op = new OperationIf(token, condition);
      push_stack(op);
      if (!parse_sequence())
        return false;

      Token *close_brace = current_token();
      if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
        print_token_error(current_token(), "Missing if close brace");
        return false;
      }
      next_token();

      pop_stack();
    } else if (token->has_text(data, "else")) {
      Operation *last_operation = parent->get_last_child();
      OperationIf *if_operation = dynamic_cast<OperationIf *>(last_operation);
      if (if_operation == nullptr) {
        print_token_error(current_token(), "else must follow if");
        return false;
      }

      next_token();

      Token *open_brace = current_token();
      if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
        print_token_error(current_token(), "Missing else open brace");
        return false;
      }
      next_token();

      op = new OperationElse(token);
      if_operation->else_operation = (OperationElse *)op;
      push_stack(op);
      if (!parse_sequence())
        return false;

      Token *close_brace = current_token();
      if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
        print_token_error(current_token(), "Missing else close brace");
        return false;
      }
      next_token();

      pop_stack();
    } else if (token->has_text(data, "while")) {
      next_token();

      autofree_operation condition = parse_expression();
      if (condition == nullptr) {
        print_token_error(current_token(), "Not valid while condition");
        return false;
      }

      Token *open_brace = current_token();
      if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
        print_token_error(current_token(), "Missing while open brace");
        return false;
      }
      next_token();

      op = new OperationWhile(condition);
      push_stack(op);
      if (!parse_sequence())
        return false;

      Token *close_brace = current_token();
      if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
        print_token_error(current_token(), "Missing while close brace");
        return false;
      }
      next_token();

      pop_stack();
    } else if (token->has_text(data, "return")) {
      next_token();

      Operation *value = parse_expression();
      if (value == nullptr) {
        print_token_error(current_token(), "Not valid return value");
        return false;
      }

      op = new OperationReturn(value, get_current_function());
    } else if (token->has_text(data, "assert")) {
      next_token();

      autofree_operation expression = parse_expression();
      if (expression == nullptr) {
        print_token_error(current_token(), "Not valid assertion expression");
        return false;
      }

      op = new OperationAssert(token, expression);
    } else if ((v = find_variable(token)) != nullptr) {
      Token *name = token;
      next_token();

      Token *assignment_token = current_token();
      if (assignment_token->type != TOKEN_TYPE_ASSIGN) {
        print_token_error(current_token(), "Missing assignment token");
        return false;
      }
      next_token();

      autofree_operation value = parse_expression();
      if (value == nullptr) {
        print_token_error(current_token(), "Invalid value for variable");
        return false;
      }

      auto variable_type = v->get_data_type(data);
      auto value_type = value->get_data_type(data);
      if (!can_assign(variable_type, value_type)) {
        print_token_error(name, "Variable is of type " + variable_type +
                                    ", but value is of type " +
                                    value_type.c_str());
        return false;
      }

      op = new OperationVariableAssignment(name, value, v);
    } else if ((op = parse_value()) != nullptr) {
      // FIXME: Only allow functions and variable values
    } else {
      print_token_error(current_token(), "Unexpected token");
      return false;
    }

    parent->add_child(op);
  }

  return true;
}

OperationModule *elf_parse(const char *data, size_t data_length) {
  auto parser = new Parser(data, data_length);

  parser->tokens = elf_lex(data, data_length);

  Operation *module = new OperationModule();
  parser->push_stack(module);

  if (!parser->parse_sequence()) {
    delete parser;
    return nullptr;
  }

  if (parser->current_token() != nullptr) {
    printf("Expected end of input\n");
    return nullptr;
  }

  delete parser;

  return static_cast<OperationModule *>(module->ref());
}
