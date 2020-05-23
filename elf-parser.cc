/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-parser.h"

#include <memory>
#include <stdio.h>
#include <vector>

struct StackFrame {
  std::shared_ptr<Operation> operation;

  std::vector<std::shared_ptr<OperationVariableDefinition>> variables;

  StackFrame(std::shared_ptr<Operation> operation) : operation(operation) {}
};

struct Parser {
  const char *data;
  size_t data_length;

  std::vector<std::shared_ptr<Token>> tokens;
  size_t offset;

  std::vector<StackFrame *> stack;

  std::shared_ptr<Token> error_token;
  std::string error_message;

  Parser(const char *data, size_t data_length)
      : data(data), data_length(data_length), offset(0) {}

  void push_stack(std::shared_ptr<Operation> operation);
  void
  add_stack_variable(std::shared_ptr<OperationVariableDefinition> definition);
  void pop_stack();
  void set_error(std::shared_ptr<Token> token, const std::string &message);
  void print_error();
  bool is_data_type(std::shared_ptr<Token> token);
  bool is_parameter_name(std::shared_ptr<Token> token);
  bool token_text_matches(std::shared_ptr<Token> a, std::shared_ptr<Token> b);
  std::shared_ptr<OperationVariableDefinition>
  find_variable(std::shared_ptr<Token> token);
  std::shared_ptr<OperationFunctionDefinition>
  find_function(std::shared_ptr<Token> token);
  bool is_builtin_function(std::shared_ptr<Token> token);
  bool has_member(std::shared_ptr<Operation> value,
                  std::shared_ptr<Token> member);
  std::shared_ptr<Token> current_token();
  void next_token();
  bool parse_parameters(std::vector<std::shared_ptr<Operation>> &parameters);
  std::shared_ptr<Operation> parse_value();
  std::shared_ptr<Operation> parse_number_value();
  std::shared_ptr<Operation> parse_text_value();
  std::shared_ptr<Operation> parse_expression();
  std::shared_ptr<Operation> parse_variable_value(std::shared_ptr<Token> &token,
                                                  const std::string &data_type);
  std::shared_ptr<OperationFunctionDefinition> get_current_function();
  bool parse_sequence();
};

void Parser::push_stack(std::shared_ptr<Operation> operation) {
  stack.push_back(new StackFrame(operation));
}

void Parser::add_stack_variable(
    std::shared_ptr<OperationVariableDefinition> definition) {
  StackFrame *frame = stack.back();
  frame->variables.push_back(definition);
}

void Parser::pop_stack() { stack.pop_back(); }

void Parser::set_error(std::shared_ptr<Token> token,
                       const std::string &message) {
  if (error_token)
    return;

  error_token = token;
  error_message = message;
}

void Parser::print_error() {
  size_t line_offset = 0;
  size_t line_number = 1;
  for (size_t i = 0; i < error_token->offset; i++) {
    if (data[i] == '\n') {
      line_offset = i + 1;
      line_number++;
    }
  }

  printf("Line %zi:\n", line_number);
  for (size_t i = line_offset; data[i] != '\0' && data[i] != '\n'; i++)
    printf("%c", data[i]);
  printf("\n");
  for (size_t i = line_offset; i < error_token->offset; i++)
    printf(" ");
  for (size_t i = 0; i < error_token->length; i++)
    printf("^");
  printf("\n");
  printf("%s\n", error_message.c_str());
}

static bool is_number_char(char c) { return c >= '0' && c <= '9'; }

static bool is_symbol_char(char c) {
  return is_number_char(c) || (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') || c == '_';
}

static char string_is_complete(const char *data, std::shared_ptr<Token> token) {
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

static bool token_is_complete(const char *data, std::shared_ptr<Token> token,
                              char next_c) {
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

static std::vector<std::shared_ptr<Token>> elf_lex(const char *data,
                                                   size_t data_length) {
  std::vector<std::shared_ptr<Token>> tokens;
  std::shared_ptr<Token> current_token = nullptr;

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
      if (c == '#')
        type = TOKEN_TYPE_COMMENT;
      else if (c == '(')
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

      auto token = std::make_shared<Token>(type, offset, 1, data);
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

bool Parser::is_data_type(std::shared_ptr<Token> token) {
  // FIXME: Use a .elf file that defines these so they're like all types
  const char *builtin_types[] = {"bool",  "uint8",  "int8",  "uint16",
                                 "int16", "uint32", "int32", "uint64",
                                 "int64", "utf8",   nullptr};

  if (token->type != TOKEN_TYPE_WORD)
    return false;

  for (int i = 0; builtin_types[i] != nullptr; i++)
    if (token->has_text(builtin_types[i]))
      return true;

  return false;
}

bool Parser::is_parameter_name(std::shared_ptr<Token> token) {
  if (token->type != TOKEN_TYPE_WORD)
    return false;

  // FIXME: Don't allow reserved words / data types

  return true;
}

bool Parser::token_text_matches(std::shared_ptr<Token> a,
                                std::shared_ptr<Token> b) {
  if (a->length != b->length)
    return false;

  for (size_t i = 0; i < a->length; i++)
    if (data[a->offset + i] != data[b->offset + i])
      return false;

  return true;
}

std::shared_ptr<OperationVariableDefinition>
Parser::find_variable(std::shared_ptr<Token> token) {
  if (token->type != TOKEN_TYPE_WORD)
    return nullptr;

  for (auto i = stack.rbegin(); i != stack.rend(); i++) {
    StackFrame *frame = *i;

    for (auto j = frame->variables.begin(); j != frame->variables.end(); j++) {
      auto definition = *j;

      if (token_text_matches(definition->name, token))
        return definition;
    }
  }

  return nullptr;
}

std::shared_ptr<OperationFunctionDefinition>
Parser::find_function(std::shared_ptr<Token> token) {
  if (token->type != TOKEN_TYPE_WORD)
    return nullptr;

  for (auto i = stack.rbegin(); i != stack.rend(); i++) {
    auto operation = (*i)->operation;

    size_t n_children = operation->get_n_children();
    for (size_t j = 0; j < n_children; j++) {
      auto child = operation->get_child(j);

      auto op = std::dynamic_pointer_cast<OperationFunctionDefinition>(child);
      if (op != nullptr && token_text_matches(op->name, token))
        return op;
    }
  }

  return nullptr;
}

bool Parser::is_builtin_function(std::shared_ptr<Token> token) {
  const char *builtin_functions[] = {"print", nullptr};

  if (token->type != TOKEN_TYPE_WORD)
    return false;

  for (int i = 0; builtin_functions[i] != nullptr; i++)
    if (token->has_text(builtin_functions[i]))
      return true;

  return false;
}

bool Parser::has_member(std::shared_ptr<Operation> value,
                        std::shared_ptr<Token> member) {
  auto data_type = value->get_data_type();

  // FIXME: Super hacky. The nullptr is because Elf can't determine the return
  // value of a member yet i.e. "foo".upper.length
  if (data_type == "" || data_type == "utf8") {
    return member->has_text(".length") || member->has_text(".upper") ||
           member->has_text(".lower");
  }

  return false;
}

std::shared_ptr<Token> Parser::current_token() {
  return offset < tokens.size() ? tokens[offset] : 0;
}

void Parser::next_token() { offset++; }

bool Parser::parse_parameters(
    std::vector<std::shared_ptr<Operation>> &parameters) {
  auto open_paren_token = current_token();
  if (open_paren_token == nullptr ||
      open_paren_token->type != TOKEN_TYPE_OPEN_PAREN)
    return true;
  next_token();

  bool closed = false;
  while (current_token() != nullptr) {
    auto t = current_token();
    if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
      next_token();
      closed = true;
      break;
    }

    if (parameters.size() > 0) {
      if (t->type != TOKEN_TYPE_COMMA) {
        set_error(current_token(), "Missing comma");
        return false;
      }
      next_token();
      t = current_token();
    }

    auto value = parse_expression();
    if (value == nullptr) {
      set_error(current_token(), "Invalid parameter");
      return false;
    }

    parameters.push_back(value);
  }

  if (!closed) {
    set_error(current_token(), "Unclosed paren");
    return false;
  }

  return true;
}

std::shared_ptr<Operation> Parser::parse_value() {
  auto token = current_token();
  if (token == nullptr)
    return nullptr;

  std::shared_ptr<OperationFunctionDefinition> f;
  std::shared_ptr<OperationVariableDefinition> v;
  std::shared_ptr<Operation> value = nullptr;
  if (token->type == TOKEN_TYPE_NUMBER) {
    value = parse_number_value();
    if (value == nullptr)
      return nullptr;
  } else if (token->type == TOKEN_TYPE_TEXT) {
    value = parse_text_value();
    if (value == nullptr)
      return nullptr;
  } else if (token->has_text("true")) {
    next_token();
    value = std::make_shared<OperationBooleanConstant>(token, true);
  } else if (token->has_text("false")) {
    next_token();
    value = std::make_shared<OperationBooleanConstant>(token, false);
  } else if ((v = find_variable(token)) != nullptr) {
    next_token();
    value = std::make_shared<OperationVariableValue>(token, v);
  } else if ((f = find_function(token)) != nullptr ||
             is_builtin_function(token)) {
    auto name = token;
    next_token();

    std::vector<std::shared_ptr<Operation>> parameters;
    if (!parse_parameters(parameters))
      return nullptr;

    value = std::make_shared<OperationFunctionCall>(name, parameters, f);
  }

  if (value == nullptr)
    return nullptr;

  token = current_token();
  while (token != nullptr && token->type == TOKEN_TYPE_MEMBER) {
    if (!has_member(value, token)) {
      set_error(token, "Member not available");
      return nullptr;
    }
    next_token();

    std::vector<std::shared_ptr<Operation>> parameters;
    if (!parse_parameters(parameters))
      return nullptr;

    value = std::make_shared<OperationMemberValue>(value, token, parameters);
    token = current_token();
  }

  return value;
}

std::shared_ptr<Operation> Parser::parse_number_value() {
  auto token = current_token();

  uint64_t number = 0;
  for (size_t i = 0; i < token->length; i++) {
    auto new_number = number * 10 + token->data[token->offset + i] - '0';
    if (new_number < number) {
      set_error(token, "Number too large");
      return nullptr;
    }
    number = new_number;
  }
  std::string data_type;
  if (number <= UINT8_MAX)
    data_type = "uint8";
  else if (number <= UINT16_MAX)
    data_type = "uint16";
  else if (number <= UINT32_MAX)
    data_type = "uint32";
  else
    data_type = "uint64";
  next_token();

  return std::make_shared<OperationNumberConstant>(data_type, token, number);
}

static int hex_digit(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else
    return -1;
}

std::shared_ptr<Operation> Parser::parse_text_value() {
  auto token = current_token();

  std::string value;
  // Iterate over the characters inside the quotes
  bool in_escape = false;
  for (size_t i = 1; i < (token->length - 1); i++) {
    char c = data[token->offset + i];
    size_t n_remaining = token->length - 1;

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
      else if (c == 'x') {
        if (n_remaining < 3)
          break;

        int digit0 = hex_digit(token->data[token->offset + i + 1]);
        int digit1 = hex_digit(token->data[token->offset + i + 2]);
        i += 2;
        if (digit0 >= 0 && digit1 >= 0)
          value += digit0 << 4 | digit1;
        else
          value += '?';
        continue;
      } else if (c == 'u' || c == 'U') {
        size_t length = c == 'u' ? 4 : 8;

        if (n_remaining < 1 + length)
          break;

        uint32_t unichar = 0;
        bool valid = true;
        for (size_t j = 0; j < length; j++) {
          int digit = hex_digit(token->data[token->offset + i + 1 + j]);
          if (digit < 0)
            valid = false;
          else
            unichar = unichar << 4 | digit;
        }
        i += length;
        if (valid) {
          if (unichar < (1 << 7)) {
            value += unichar;
          } else if (unichar < (1 << 11)) {
            value += 0xC0 | (unichar >> 6);
            value += 0x80 | ((unichar >> 0) & 0x3F);
          } else if (unichar < (1 << 16)) {
            value += 0xE0 | (unichar >> 12);
            value += 0x80 | ((unichar >> 6) & 0x3F);
            value += 0x80 | ((unichar >> 0) & 0x3F);
          } else if (unichar < (1 << 21)) {
            value += 0xF0 | (unichar >> 18);
            value += 0x80 | ((unichar >> 12) & 0x3F);
            value += 0x80 | ((unichar >> 6) & 0x3F);
            value += 0x80 | ((unichar >> 0) & 0x3F);
          } else {
            value += '?';
          }
        } else
          value += '?';
        continue;
      }
    }

    value += c;
  }
  next_token();

  return std::make_shared<OperationTextConstant>(token, value);
}

static bool token_is_binary_boolean_operator(std::shared_ptr<Token> &token) {
  if (token->type != TOKEN_TYPE_WORD)
    return false;

  return token->has_text("and") || token->has_text("or") ||
         token->has_text("xor");
}

static bool token_is_binary_operator(std::shared_ptr<Token> &token) {
  return token->type == TOKEN_TYPE_EQUAL ||
         token->type == TOKEN_TYPE_NOT_EQUAL ||
         token->type == TOKEN_TYPE_GREATER ||
         token->type == TOKEN_TYPE_GREATER_EQUAL ||
         token->type == TOKEN_TYPE_LESS ||
         token->type == TOKEN_TYPE_LESS_EQUAL ||
         token->type == TOKEN_TYPE_ADD || token->type == TOKEN_TYPE_SUBTRACT ||
         token->type == TOKEN_TYPE_MULTIPLY ||
         token->type == TOKEN_TYPE_DIVIDE ||
         token_is_binary_boolean_operator(token);
}

// Returns true if from_type can be run-time converted to to_type
static bool can_convert(const std::string &from_type,
                        const std::string &to_type) {
  // FIXME: Signed integers
  if (from_type == "uint8")
    return to_type == "uint16" || to_type == "uint32" || to_type == "uint64";
  if (from_type == "uint16")
    return to_type == "uint32" || to_type == "uint64";
  if (from_type == "uint32")
    return to_type == "uint64";
  return false;
}

std::shared_ptr<Operation> Parser::parse_expression() {
  auto a = parse_value();
  if (a == nullptr)
    return nullptr;

  auto op = current_token();
  if (op == nullptr)
    return a;

  if (!token_is_binary_operator(op))
    return a;
  next_token();

  auto b = parse_value();
  if (b == nullptr) {
    set_error(current_token(), "Missing second value in binary operation");
    return nullptr;
  }

  auto a_type = a->get_data_type();
  auto b_type = b->get_data_type();
  if (a_type != b_type) {
    if (can_convert(a_type, b_type))
      a = std::make_shared<OperationConvert>(a, b_type);
    else if (can_convert(b_type, a_type))
      b = std::make_shared<OperationConvert>(b, a_type);
    else {
      set_error(op, "Can't combine " + a_type + " and " + b_type + " types");
      return nullptr;
    }
  }

  return std::make_shared<OperationBinary>(op, a, b);
}

std::shared_ptr<Operation>
Parser::parse_variable_value(std::shared_ptr<Token> &token,
                             const std::string &data_type) {
  auto value = parse_expression();
  if (value == nullptr) {
    set_error(current_token(), "Invalid value for variable");
    return nullptr;
  }

  // If types match, then nothing to do
  auto value_type = value->get_data_type();
  if (value_type == data_type)
    return value;

  // If can be converted, do that first
  if (can_convert(value_type, data_type))
    return std::make_shared<OperationConvert>(value, data_type);

  set_error(token, "Variable is of type " + data_type +
                       ", but value is of type " + value_type);
  return nullptr;
}

std::shared_ptr<OperationFunctionDefinition> Parser::get_current_function() {
  for (auto i = stack.rbegin(); i != stack.rend(); i++) {
    auto op =
        std::dynamic_pointer_cast<OperationFunctionDefinition>((*i)->operation);
    if (op != nullptr)
      return op;
  }

  return nullptr;
}

bool Parser::parse_sequence() {
  std::shared_ptr<Operation> parent = stack.back()->operation;

  while (current_token() != nullptr) {
    auto token = current_token();

    // Stop when sequence ends
    if (token->type == TOKEN_TYPE_CLOSE_BRACE)
      break;

    // Ignore comments
    if (token->type == TOKEN_TYPE_COMMENT) {
      next_token();
      continue;
    }

    std::shared_ptr<Operation> op = nullptr;
    std::shared_ptr<OperationVariableDefinition> v;
    if (is_data_type(token)) {
      auto data_type = token;
      next_token();

      auto name = current_token(); // FIXME: Check valid name
      next_token();

      auto assignment_token = current_token();
      if (assignment_token == nullptr) {
        auto definition = std::make_shared<OperationVariableDefinition>(
            data_type, name, nullptr);
        add_stack_variable(definition);
        op = definition;
      } else if (assignment_token->type == TOKEN_TYPE_ASSIGN) {
        next_token();

        auto value = parse_variable_value(name, data_type->get_text());
        if (value == nullptr)
          return false;

        auto definition = std::make_shared<OperationVariableDefinition>(
            data_type, name, value);
        add_stack_variable(definition);
        op = definition;
      } else if (assignment_token->type == TOKEN_TYPE_OPEN_PAREN) {
        next_token();

        std::vector<std::shared_ptr<OperationVariableDefinition>> parameters;
        bool closed = false;
        while (current_token() != nullptr) {
          auto t = current_token();
          if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
            next_token();
            closed = true;
            break;
          }

          if (parameters.size() > 0) {
            if (t->type != TOKEN_TYPE_COMMA) {
              set_error(current_token(), "Missing comma");
              return false;
            }
            next_token();
            t = current_token();
          }

          if (!is_data_type(t)) {
            set_error(current_token(), "Parameter not a data type");
            return false;
          }
          data_type = t;
          next_token();

          auto name = current_token();
          if (!is_parameter_name(name)) {
            set_error(current_token(), "Not a parameter name");
            return false;
          }
          next_token();

          parameters.push_back(std::make_shared<OperationVariableDefinition>(
              data_type, name, nullptr));
        }

        if (!closed) {
          set_error(current_token(), "Unclosed paren");
          return false;
        }

        auto open_brace = current_token();
        if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
          set_error(current_token(), "Missing function open brace");
          return false;
        }
        next_token();

        op = std::make_shared<OperationFunctionDefinition>(data_type, name,
                                                           parameters);
        push_stack(op);

        for (auto i = parameters.begin(); i != parameters.end(); i++)
          add_stack_variable(*i);

        if (!parse_sequence())
          return false;

        auto close_brace = current_token();
        if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
          set_error(current_token(), "Missing function close brace");
          return false;
        }
        next_token();

        pop_stack();
      } else {
        auto definition = std::make_shared<OperationVariableDefinition>(
            data_type, name, nullptr);
        add_stack_variable(definition);
        op = definition;
      }
    } else if (token->has_text("if")) {
      next_token();

      auto condition = parse_expression();
      if (condition == nullptr) {
        set_error(current_token(), "Not valid if condition");
        return false;
      }

      auto open_brace = current_token();
      if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
        set_error(current_token(), "Missing if open brace");
        return false;
      }
      next_token();

      op = std::make_shared<OperationIf>(token, condition);
      push_stack(op);
      if (!parse_sequence())
        return false;

      auto close_brace = current_token();
      if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
        set_error(current_token(), "Missing if close brace");
        return false;
      }
      next_token();

      pop_stack();
    } else if (token->has_text("else")) {
      std::shared_ptr<Operation> last_operation = parent->get_last_child();
      auto if_operation =
          std::dynamic_pointer_cast<OperationIf>(last_operation);
      if (if_operation == nullptr) {
        set_error(current_token(), "else must follow if");
        return false;
      }

      next_token();

      auto open_brace = current_token();
      if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
        set_error(current_token(), "Missing else open brace");
        return false;
      }
      next_token();

      auto else_op = std::make_shared<OperationElse>(token);
      if_operation->else_operation = else_op;
      op = else_op;
      push_stack(op);
      if (!parse_sequence())
        return false;

      auto close_brace = current_token();
      if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
        set_error(current_token(), "Missing else close brace");
        return false;
      }
      next_token();

      pop_stack();
    } else if (token->has_text("while")) {
      next_token();

      auto condition = parse_expression();
      if (condition == nullptr) {
        set_error(current_token(), "Not valid while condition");
        return false;
      }

      auto open_brace = current_token();
      if (open_brace->type != TOKEN_TYPE_OPEN_BRACE) {
        set_error(current_token(), "Missing while open brace");
        return false;
      }
      next_token();

      op = std::make_shared<OperationWhile>(condition);
      push_stack(op);
      if (!parse_sequence())
        return false;

      auto close_brace = current_token();
      if (close_brace->type != TOKEN_TYPE_CLOSE_BRACE) {
        set_error(current_token(), "Missing while close brace");
        return false;
      }
      next_token();

      pop_stack();
    } else if (token->has_text("return")) {
      next_token();

      std::shared_ptr<Operation> value = parse_expression();
      if (value == nullptr) {
        set_error(current_token(), "Not valid return value");
        return false;
      }

      op = std::make_shared<OperationReturn>(value, get_current_function());
    } else if (token->has_text("assert")) {
      next_token();

      auto expression = parse_expression();
      if (expression == nullptr) {
        set_error(current_token(), "Not valid assertion expression");
        return false;
      }

      op = std::make_shared<OperationAssert>(token, expression);
    } else if ((v = find_variable(token)) != nullptr) {
      auto name = token;
      next_token();

      auto assignment_token = current_token();
      if (assignment_token->type != TOKEN_TYPE_ASSIGN) {
        set_error(current_token(), "Missing assignment token");
        return false;
      }
      next_token();

      auto value = parse_variable_value(name, v->get_data_type());
      if (value == nullptr)
        return false;

      op = std::make_shared<OperationVariableAssignment>(name, value, v);
    } else if ((op = parse_value()) != nullptr) {
      // FIXME: Only allow functions and variable values
    } else {
      set_error(current_token(), "Unexpected token");
      return false;
    }

    parent->add_child(op);
  }

  return true;
}

std::shared_ptr<OperationModule> elf_parse(const char *data,
                                           size_t data_length) {
  Parser parser(data, data_length);

  parser.tokens = elf_lex(data, data_length);

  auto module = std::make_shared<OperationModule>();
  parser.push_stack(module);

  if (!parser.parse_sequence()) {
    parser.print_error();
    return nullptr;
  }

  if (parser.current_token()) {
    printf("Expected end of input\n");
    return nullptr;
  }

  return module;
}
