/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-parser.h"

#include "elf-lexer.h"

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

  std::shared_ptr<OperationModule> core_module;

  Parser(const char *data, size_t data_length)
      : data(data), data_length(data_length), offset(0) {}

  void push_stack(std::shared_ptr<Operation> operation);
  void
  add_stack_variable(std::shared_ptr<OperationVariableDefinition> definition);
  void pop_stack();
  void set_error(std::shared_ptr<Token> token, const std::string &message);
  void print_error();
  bool token_text_matches(std::shared_ptr<Token> a, std::shared_ptr<Token> b);
  std::shared_ptr<Operation> find_type(std::string name);
  std::shared_ptr<Operation> find_type(std::shared_ptr<Operation> operation,
                                       std::string name);
  std::shared_ptr<OperationVariableDefinition>
  find_variable(std::shared_ptr<Token> token);
  std::shared_ptr<OperationFunctionDefinition>
  find_function(std::shared_ptr<Token> token);
  std::shared_ptr<Token> current_token();
  void next_token();
  bool parse_parameters(std::vector<std::shared_ptr<Operation>> &parameters);
  std::shared_ptr<Operation> parse_value();
  std::shared_ptr<OperationTrue> parse_true();
  std::shared_ptr<OperationFalse> parse_false();
  std::shared_ptr<OperationNumberConstant> parse_number_constant();
  std::shared_ptr<OperationTextConstant> parse_text_constant();
  std::shared_ptr<OperationDataType> parse_data_type();
  std::shared_ptr<OperationSymbol> parse_symbol();
  std::shared_ptr<Operation> parse_expression();
  std::shared_ptr<Operation> parse_variable_value(std::shared_ptr<Token> &token,
                                                  const std::string &data_type);
  std::shared_ptr<OperationIf> parse_if();
  std::shared_ptr<OperationElse> parse_else(std::shared_ptr<Operation> &parent);
  std::shared_ptr<OperationWhile> parse_while();
  std::shared_ptr<OperationReturn> parse_return();
  std::shared_ptr<OperationAssert> parse_assert();
  std::shared_ptr<OperationPrimitiveDefinition> parse_primitive_definition();
  std::shared_ptr<OperationTypeDefinition> parse_type_definition();
  std::shared_ptr<OperationVariableDefinition> parse_variable_definition();
  std::shared_ptr<OperationFunctionDefinition> parse_function_definition();
  std::shared_ptr<Operation> parse_expression_or_assignment();
  std::shared_ptr<OperationFunctionDefinition> get_current_function();
  bool parse_sequence();
  bool resolve_operation(std::shared_ptr<Operation> operation);
  bool resolve_sequence(std::vector<std::shared_ptr<Operation>> &body);
  bool resolve_module(std::shared_ptr<OperationModule> &oeration);
  bool resolve_variable_definition(
      std::shared_ptr<OperationVariableDefinition> &operation);
  bool resolve_assignment(std::shared_ptr<OperationAssignment> &operation);
  bool resolve_if(std::shared_ptr<OperationIf> &operation);
  bool resolve_else(std::shared_ptr<OperationElse> &operation);
  bool resolve_while(std::shared_ptr<OperationWhile> &operation);
  bool resolve_data_type(std::shared_ptr<OperationDataType> &operation);
  bool resolve_symbol(std::shared_ptr<OperationSymbol> &operation);
  bool resolve_call(std::shared_ptr<OperationCall> &operation);
  bool resolve_function_definition(
      std::shared_ptr<OperationFunctionDefinition> &operation);
  bool
  resolve_type_definition(std::shared_ptr<OperationTypeDefinition> &operation);
  bool resolve_return(std::shared_ptr<OperationReturn> &operation);
  bool resolve_assert(std::shared_ptr<OperationAssert> &operation);
  bool resolve_member(std::shared_ptr<OperationMember> &operation);
  bool resolve_binary(std::shared_ptr<OperationBinary> &operation);
  bool resolve_convert(std::shared_ptr<OperationConvert> &operation);
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

bool Parser::token_text_matches(std::shared_ptr<Token> a,
                                std::shared_ptr<Token> b) {
  if (a->length != b->length)
    return false;

  for (size_t i = 0; i < a->length; i++)
    if (data[a->offset + i] != data[b->offset + i])
      return false;

  return true;
}

std::shared_ptr<Operation> Parser::find_type(std::string name) {
  if (core_module != nullptr) {
    auto definition = find_type(core_module, name);
    if (definition != nullptr)
      return definition;
  }

  for (auto i = stack.rbegin(); i != stack.rend(); i++) {
    auto definition = find_type((*i)->operation, name);
    if (definition != nullptr)
      return definition;
  }

  return nullptr;
}

std::shared_ptr<Operation>
Parser::find_type(std::shared_ptr<Operation> operation, std::string name) {
  size_t n_children = operation->get_n_children();
  for (size_t i = 0; i < n_children; i++) {
    auto child = operation->get_child(i);

    auto primitive_definition =
        std::dynamic_pointer_cast<OperationPrimitiveDefinition>(child);
    if (primitive_definition != nullptr &&
        primitive_definition->name->has_text(name))
      return primitive_definition;
    auto type_definition =
        std::dynamic_pointer_cast<OperationTypeDefinition>(child);
    if (type_definition != nullptr && type_definition->name->has_text(name))
      return type_definition;
  }

  return nullptr;
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

std::shared_ptr<Token> Parser::current_token() {
  return offset < tokens.size() ? tokens[offset] : 0;
}

void Parser::next_token() { offset++; }

bool Parser::parse_parameters(
    std::vector<std::shared_ptr<Operation>> &parameters) {
  auto open_paren_token = current_token();
  if (open_paren_token->type != TOKEN_TYPE_OPEN_PAREN)
    return true;
  next_token();

  while (current_token()->type != TOKEN_TYPE_EOF) {
    auto t = current_token();
    if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
      next_token();
      return true;
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

  set_error(current_token(), "Unclosed paren");
  return false;
}

std::shared_ptr<Operation> Parser::parse_value() {
  std::shared_ptr<Operation> op = parse_true();
  if (op == nullptr)
    op = parse_false();
  if (op == nullptr)
    op = parse_number_constant();
  if (op == nullptr)
    op = parse_text_constant();
  if (op == nullptr)
    op = parse_symbol();

  if (op == nullptr)
    return nullptr;

  while (true) {
    auto token = current_token();
    if (token->type == TOKEN_TYPE_OPEN_PAREN) {
      std::vector<std::shared_ptr<Operation>> parameters;
      if (!parse_parameters(parameters))
        return nullptr;

      op = std::make_shared<OperationCall>(op, parameters);
    } else if (token->type == TOKEN_TYPE_MEMBER) {
      next_token();
      op = std::make_shared<OperationMember>(op, token);
    } else
      return op;
  }
}

std::shared_ptr<OperationTrue> Parser::parse_true() {
  auto token = current_token();
  if (!token->has_text("true"))
    return nullptr;
  next_token();

  return std::make_shared<OperationTrue>(token);
}

std::shared_ptr<OperationFalse> Parser::parse_false() {
  auto token = current_token();
  if (!token->has_text("false"))
    return nullptr;
  next_token();

  return std::make_shared<OperationFalse>(token);
}

std::shared_ptr<OperationNumberConstant> Parser::parse_number_constant() {
  auto token = current_token();
  if (token->type != TOKEN_TYPE_NUMBER)
    return nullptr;

  uint64_t number = 0;
  for (size_t i = 0; i < token->length; i++) {
    auto new_number = number * 10 + token->data[token->offset + i] - '0';
    if (new_number < number) {
      set_error(token, "Number too large for 64 bit integer");
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

std::shared_ptr<OperationTextConstant> Parser::parse_text_constant() {
  auto token = current_token();
  if (token->type != TOKEN_TYPE_TEXT)
    return nullptr;

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

std::shared_ptr<OperationDataType> Parser::parse_data_type() {
  auto token = current_token();
  if (token->type != TOKEN_TYPE_WORD)
    return nullptr;
  next_token();

  return std::make_shared<OperationDataType>(token);
}

std::shared_ptr<OperationSymbol> Parser::parse_symbol() {
  auto token = current_token();
  if (token->type != TOKEN_TYPE_WORD)
    return nullptr;
  next_token();

  return std::make_shared<OperationSymbol>(token);
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

// Returns operation with the requested data type or nullptr if cannot
static std::shared_ptr<Operation>
convert_to_data_type(std::shared_ptr<Operation> &operation,
                     const std::string &to_type) {
  auto from_type = operation->get_data_type();
  if (from_type == to_type)
    return operation;

  // Convert unsigned constant numbers to signed ones
  auto number_constant =
      std::dynamic_pointer_cast<OperationNumberConstant>(operation);
  if (number_constant != nullptr && number_constant->sign_token == nullptr) {
    uint64_t max_magnitude = 0;
    if (to_type == "int8")
      max_magnitude = INT8_MAX;
    else if (to_type == "int16")
      max_magnitude = INT16_MAX;
    else if (to_type == "int32")
      max_magnitude = INT32_MAX;
    else if (to_type == "int64")
      max_magnitude = INT64_MAX;

    if (max_magnitude > 0) {
      if (number_constant->magnitude > max_magnitude)
        return nullptr;

      return std::make_shared<OperationNumberConstant>(
          to_type, number_constant->magnitude_token,
          number_constant->magnitude);
    }
  }

  bool can_convert = false;
  if (from_type == "uint8")
    can_convert =
        to_type == "uint16" || to_type == "uint32" || to_type == "uint64";
  else if (from_type == "int8")
    can_convert =
        to_type == "int16" || to_type == "int32" || to_type == "int64";
  else if (from_type == "uint16")
    can_convert = to_type == "uint32" || to_type == "uint64";
  else if (from_type == "int16")
    can_convert = to_type == "int32" || to_type == "int64";
  else if (from_type == "uint32")
    can_convert = to_type == "uint64";
  else if (from_type == "int32")
    can_convert = to_type == "int64";

  if (!can_convert)
    return nullptr;

  return std::make_shared<OperationConvert>(operation, to_type);
}

static bool is_signed(const std::string &data_type) {
  return data_type == "int8" || data_type == "int16" || data_type == "int32" ||
         data_type == "int64";
}

std::shared_ptr<Operation> Parser::parse_expression() {
  auto unary_operation = current_token();
  if (unary_operation->type == TOKEN_TYPE_SUBTRACT) {
    next_token();
    auto value_token = current_token(); // FIXME: Get the token(s) from 'value'
    auto value = parse_value();
    if (value == nullptr) {
      set_error(unary_operation, "Missing second value in unary operation");
      return nullptr;
    }

    auto number_constant =
        std::dynamic_pointer_cast<OperationNumberConstant>(value);
    if (number_constant != nullptr) {
      std::string data_type;
      if (number_constant->magnitude <= -INT8_MIN)
        data_type = "int8";
      else if (number_constant->magnitude <= -static_cast<int64_t>(INT16_MIN))
        data_type = "int16";
      else if (number_constant->magnitude <= -static_cast<int64_t>(INT32_MIN))
        data_type = "int32";
      else if (number_constant->magnitude <=
               9223372036854775808U) // NOTE: Can't use INT64_MIN as it can't be
                                     // inverted without overflowing
        data_type = "int64";
      else {
        set_error(number_constant->magnitude_token,
                  "Number too large for 64 bit signed integer");
        return nullptr;
      }

      return std::make_shared<OperationNumberConstant>(
          data_type, unary_operation, number_constant->magnitude_token,
          number_constant->magnitude);
    }

    auto data_type = value->get_data_type();
    if (!is_signed(data_type)) {
      set_error(value_token, "Cannot invert " + data_type);
      return nullptr;
    }

    return std::make_shared<OperationUnary>(unary_operation, value);
  }

  auto a = parse_value();
  if (a == nullptr)
    return nullptr;

  auto op = current_token();
  if (!token_is_binary_operator(op))
    return a;
  next_token();

  auto b = parse_value();
  if (b == nullptr) {
    set_error(op, "Missing second value in binary operation");
    return nullptr;
  }

  return std::make_shared<OperationBinary>(op, a, b);
}

std::shared_ptr<OperationIf> Parser::parse_if() {
  auto token = current_token();
  if (!token->has_text("if"))
    return nullptr;
  next_token();

  auto condition = parse_expression();
  if (condition == nullptr) {
    set_error(current_token(), "Not valid if condition");
    return nullptr;
  }

  if (current_token()->type != TOKEN_TYPE_OPEN_BRACE) {
    set_error(current_token(), "Missing if open brace");
    return nullptr;
  }
  next_token();

  auto op = std::make_shared<OperationIf>(token, condition);
  push_stack(op);
  if (!parse_sequence())
    return nullptr;

  if (current_token()->type != TOKEN_TYPE_CLOSE_BRACE) {
    set_error(current_token(), "Missing if close brace");
    return nullptr;
  }
  next_token();

  return op;
}

std::shared_ptr<OperationElse>
Parser::parse_else(std::shared_ptr<Operation> &parent) {
  auto token = current_token();
  if (!token->has_text("else"))
    return nullptr;
  next_token();

  std::shared_ptr<Operation> last_operation = parent->get_last_child();
  auto if_operation = std::dynamic_pointer_cast<OperationIf>(last_operation);
  if (if_operation == nullptr) {
    set_error(current_token(), "else must follow if");
    return nullptr;
  }

  if (current_token()->type != TOKEN_TYPE_OPEN_BRACE) {
    set_error(current_token(), "Missing else open brace");
    return nullptr;
  }
  next_token();

  auto op = std::make_shared<OperationElse>(token);
  if_operation->else_operation = op;
  push_stack(op);
  if (!parse_sequence())
    return nullptr;

  if (current_token()->type != TOKEN_TYPE_CLOSE_BRACE) {
    set_error(current_token(), "Missing else close brace");
    return nullptr;
  }
  next_token();

  pop_stack();

  return op;
}

std::shared_ptr<OperationWhile> Parser::parse_while() {
  auto token = current_token();
  if (!token->has_text("while"))
    return nullptr;
  next_token();

  auto condition = parse_expression();
  if (condition == nullptr) {
    set_error(current_token(), "Not valid while condition");
    return nullptr;
  }

  if (current_token()->type != TOKEN_TYPE_OPEN_BRACE) {
    set_error(current_token(), "Missing while open brace");
    return nullptr;
  }
  next_token();

  auto op = std::make_shared<OperationWhile>(condition);
  push_stack(op);
  if (!parse_sequence())
    return nullptr;

  if (current_token()->type != TOKEN_TYPE_CLOSE_BRACE) {
    set_error(current_token(), "Missing while close brace");
    return nullptr;
  }
  next_token();

  pop_stack();

  return op;
}

std::shared_ptr<OperationReturn> Parser::parse_return() {
  if (!current_token()->has_text("return"))
    return nullptr;
  next_token();

  auto value = parse_expression();
  if (value == nullptr) {
    set_error(current_token(), "Not valid return value");
    return nullptr;
  }

  return std::make_shared<OperationReturn>(value, get_current_function());
}

std::shared_ptr<OperationAssert> Parser::parse_assert() {
  auto token = current_token();
  if (!token->has_text("assert"))
    return nullptr;
  next_token();

  auto expression = parse_expression();
  if (expression == nullptr) {
    set_error(current_token(), "Not valid assertion expression");
    return nullptr;
  }

  return std::make_shared<OperationAssert>(token, expression);
}

std::shared_ptr<OperationPrimitiveDefinition>
Parser::parse_primitive_definition() {
  if (!current_token()->has_text("primitive"))
    return nullptr;
  next_token();

  auto name = current_token(); // FIXME: Check valid name
  if (name->type != TOKEN_TYPE_WORD) {
    set_error(name, "Expected type name");
    return nullptr;
  }
  next_token();

  if (current_token()->type != TOKEN_TYPE_OPEN_BRACE) {
    set_error(current_token(), "Missing primitive open brace");
    return nullptr;
  }
  next_token();

  auto op = std::make_shared<OperationPrimitiveDefinition>(name);
  push_stack(op);

  while (current_token() != nullptr) {
    auto token = current_token();

    // Stop when sequence ends
    if (token->type == TOKEN_TYPE_CLOSE_BRACE) {
      next_token();
      break;
    }

    // Ignore comments
    if (token->type == TOKEN_TYPE_COMMENT) {
      next_token();
      continue;
    }

    auto function_definition = parse_function_definition();
    if (function_definition == nullptr) {
      set_error(current_token(), "Expected function definition");
      return nullptr;
    }
    op->add_child(function_definition);
  }

  return op;
}

std::shared_ptr<OperationTypeDefinition> Parser::parse_type_definition() {
  if (!current_token()->has_text("type"))
    return nullptr;
  next_token();

  auto name = current_token(); // FIXME: Check valid name
  if (name->type != TOKEN_TYPE_WORD) {
    set_error(name, "Expected type name");
    return nullptr;
  }
  next_token();

  if (current_token()->type != TOKEN_TYPE_OPEN_BRACE) {
    set_error(current_token(), "Missing type open brace");
    return nullptr;
  }
  next_token();

  auto op = std::make_shared<OperationTypeDefinition>(name);
  push_stack(op);

  while (current_token() != nullptr) {
    auto token = current_token();

    // Stop when sequence ends
    if (token->type == TOKEN_TYPE_CLOSE_BRACE) {
      next_token();
      break;
    }

    // Ignore comments
    if (token->type == TOKEN_TYPE_COMMENT) {
      next_token();
      continue;
    }

    auto variable_definition = parse_variable_definition();
    if (variable_definition == nullptr) {
      set_error(current_token(), "Expected variable or function definition");
      return nullptr;
    }
    op->add_child(variable_definition);
  }

  return op;
}

std::shared_ptr<OperationVariableDefinition>
Parser::parse_variable_definition() {
  auto start_offset = offset;

  auto data_type = parse_data_type();
  if (data_type == nullptr) {
    offset = start_offset;
    return nullptr;
  }

  auto name = current_token();
  if (name->type != TOKEN_TYPE_WORD) {
    offset = start_offset;
    return nullptr;
  }
  next_token();

  // This is actually a function definition
  if (current_token()->type == TOKEN_TYPE_OPEN_PAREN) {
    offset = start_offset;
    return nullptr;
  }

  std::shared_ptr<Operation> value;
  if (current_token()->type == TOKEN_TYPE_ASSIGN) {
    next_token();

    value = parse_expression();
    if (value == nullptr) {
      offset = start_offset;
      return nullptr;
    }
  }

  return std::make_shared<OperationVariableDefinition>(data_type, name, value);
}

std::shared_ptr<OperationFunctionDefinition>
Parser::parse_function_definition() {
  auto start_offset = offset;

  auto data_type = parse_data_type();
  if (data_type == nullptr) {
    offset = start_offset;
    return nullptr;
  }

  auto name = current_token(); // FIXME: Check valid name
  if (name->type != TOKEN_TYPE_WORD) {
    offset = start_offset;
    return nullptr;
  }
  next_token();

  if (current_token()->type != TOKEN_TYPE_OPEN_PAREN) {
    set_error(current_token(), "Missing open parenthesis");
    return nullptr;
  }
  next_token();

  std::vector<std::shared_ptr<OperationVariableDefinition>> parameters;
  while (current_token()->type != TOKEN_TYPE_EOF) {
    auto t = current_token();
    if (t->type == TOKEN_TYPE_CLOSE_PAREN) {
      next_token();
      break;
    }

    if (parameters.size() > 0) {
      if (t->type != TOKEN_TYPE_COMMA) {
        set_error(current_token(), "Missing comma");
        return nullptr;
      }
      next_token();
      t = current_token();
    }

    auto param_data_type = parse_data_type();
    if (param_data_type == nullptr) {
      set_error(current_token(), "Parameter not a data type");
      return nullptr;
    }

    auto name = current_token();
    if (name->type != TOKEN_TYPE_WORD) {
      set_error(current_token(), "Not a parameter name");
      return nullptr;
    }
    next_token();

    parameters.push_back(std::make_shared<OperationVariableDefinition>(
        param_data_type, name, nullptr));
  }

  if (current_token()->type == TOKEN_TYPE_EOF) {
    set_error(current_token(), "Unclosed paren");
    return nullptr;
  }

  if (current_token()->type != TOKEN_TYPE_OPEN_BRACE) {
    set_error(current_token(), "Missing function open brace");
    return nullptr;
  }
  next_token();

  auto op = std::make_shared<OperationFunctionDefinition>(data_type, name,
                                                          parameters);
  push_stack(op);

  if (!parse_sequence())
    return nullptr;

  if (current_token()->type != TOKEN_TYPE_CLOSE_BRACE) {
    set_error(current_token(), "Missing function close brace");
    return nullptr;
  }
  next_token();

  pop_stack();

  return op;
}

std::shared_ptr<Operation> Parser::parse_expression_or_assignment() {
  auto start_offset = offset;

  auto target = parse_expression();
  if (target == nullptr)
    return nullptr;

  if (current_token()->type != TOKEN_TYPE_ASSIGN)
    return target;
  next_token();

  auto value = parse_expression();
  if (value == nullptr) {
    offset = start_offset;
    return nullptr;
  }

  return std::make_shared<OperationAssignment>(target, value);
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
  auto parent = stack.back()->operation;

  while (true) {
    // Stop when sequence ends
    if (current_token()->type == TOKEN_TYPE_EOF ||
        current_token()->type == TOKEN_TYPE_CLOSE_BRACE)
      return true;

    // Ignore comments
    if (current_token()->type == TOKEN_TYPE_COMMENT) {
      next_token();
      continue;
    }

    std::shared_ptr<Operation> op = parse_if();
    if (op == nullptr)
      op = parse_else(parent);
    if (op == nullptr)
      op = parse_while();
    if (op == nullptr)
      op = parse_return();
    if (op == nullptr)
      op = parse_assert();
    if (op == nullptr)
      op = parse_primitive_definition();
    if (op == nullptr)
      op = parse_type_definition();
    if (op == nullptr)
      op = parse_variable_definition();
    if (op == nullptr)
      op = parse_function_definition();
    if (op == nullptr)
      op = parse_expression_or_assignment();

    if (op == nullptr) {
      set_error(current_token(), "Unexpected token");
      return false;
    }

    parent->add_child(op);
  }
}

bool Parser::resolve_operation(std::shared_ptr<Operation> operation) {
  auto op_module = std::dynamic_pointer_cast<OperationModule>(operation);
  if (op_module != nullptr)
    return resolve_module(op_module);

  auto op_variable_definition =
      std::dynamic_pointer_cast<OperationVariableDefinition>(operation);
  if (op_variable_definition != nullptr)
    return resolve_variable_definition(op_variable_definition);

  auto op_symbol = std::dynamic_pointer_cast<OperationSymbol>(operation);
  if (op_symbol != nullptr)
    return resolve_symbol(op_symbol);

  auto op_assignment =
      std::dynamic_pointer_cast<OperationAssignment>(operation);
  if (op_assignment != nullptr)
    return resolve_assignment(op_assignment);

  auto op_if = std::dynamic_pointer_cast<OperationIf>(operation);
  if (op_if != nullptr)
    return resolve_if(op_if);

  auto op_else = std::dynamic_pointer_cast<OperationElse>(operation);
  if (op_else != nullptr)
    return resolve_else(op_else);

  auto op_while = std::dynamic_pointer_cast<OperationWhile>(operation);
  if (op_while != nullptr)
    return resolve_while(op_while);

  auto op_function_definition =
      std::dynamic_pointer_cast<OperationFunctionDefinition>(operation);
  if (op_function_definition != nullptr)
    return resolve_function_definition(op_function_definition);

  auto op_type_definition =
      std::dynamic_pointer_cast<OperationTypeDefinition>(operation);
  if (op_type_definition != nullptr)
    return resolve_type_definition(op_type_definition);

  auto op_call = std::dynamic_pointer_cast<OperationCall>(operation);
  if (op_call != nullptr)
    return resolve_call(op_call);

  auto op_return = std::dynamic_pointer_cast<OperationReturn>(operation);
  if (op_return != nullptr)
    return resolve_return(op_return);

  auto op_assert = std::dynamic_pointer_cast<OperationAssert>(operation);
  if (op_assert != nullptr)
    return resolve_assert(op_assert);

  auto op_member = std::dynamic_pointer_cast<OperationMember>(operation);
  if (op_member != nullptr)
    return resolve_member(op_member);

  auto op_binary = std::dynamic_pointer_cast<OperationBinary>(operation);
  if (op_binary != nullptr)
    return resolve_binary(op_binary);

  auto op_convert = std::dynamic_pointer_cast<OperationConvert>(operation);
  if (op_convert != nullptr)
    return resolve_convert(op_convert);

  return true;
}

bool Parser::resolve_sequence(std::vector<std::shared_ptr<Operation>> &body) {
  for (auto i = body.begin(); i != body.end(); i++) {
    if (!resolve_operation(*i))
      return false;
  }
  pop_stack();

  return true;
}

bool Parser::resolve_module(std::shared_ptr<OperationModule> &operation) {
  push_stack(operation);
  return resolve_sequence(operation->body);
}

bool Parser::resolve_variable_definition(
    std::shared_ptr<OperationVariableDefinition> &operation) {

  if (!resolve_data_type(operation->data_type))
    return false;

  if (operation->value != nullptr) {
    if (!resolve_operation(operation->value))
      return false;

    auto conversion =
        convert_to_data_type(operation->value, operation->get_data_type());
    if (conversion == nullptr) {
      set_error(operation->name, "Variable is of type " +
                                     operation->get_data_type() +
                                     ", but value is of type " +
                                     operation->value->get_data_type());
      return false;
    }
    operation->value = conversion;
  }

  add_stack_variable(operation);

  return true;
}

bool Parser::resolve_assignment(
    std::shared_ptr<OperationAssignment> &operation) {
  return resolve_operation(operation->target) &&
         resolve_operation(operation->value);
}

bool Parser::resolve_if(std::shared_ptr<OperationIf> &operation) {
  if (!resolve_operation(operation->condition))
    return false;
  push_stack(operation);
  return resolve_sequence(operation->body);
}

bool Parser::resolve_else(std::shared_ptr<OperationElse> &operation) {
  push_stack(operation);
  return resolve_sequence(operation->body);
}

bool Parser::resolve_while(std::shared_ptr<OperationWhile> &operation) {
  if (!resolve_operation(operation->condition))
    return false;
  push_stack(operation);
  return resolve_sequence(operation->body);
}

bool Parser::resolve_data_type(std::shared_ptr<OperationDataType> &operation) {
  auto data_type = operation->name->get_text();
  auto type_definition = find_type(data_type);
  if (type_definition == nullptr) {
    set_error(operation->name, "Unknown data type");
    return false;
  }

  operation->type_definition = type_definition;

  return true;
}

bool Parser::resolve_symbol(std::shared_ptr<OperationSymbol> &operation) {
  // TEMP: Hard coded function
  if (operation->name->has_text("print"))
    return true;

  std::shared_ptr<Operation> definition = find_variable(operation->name);
  if (definition == nullptr)
    definition = find_function(operation->name);
  if (definition == nullptr) {
    set_error(operation->name, "Not a variable or function");
    return false;
  }

  operation->definition = definition;

  return true;
}

bool Parser::resolve_call(std::shared_ptr<OperationCall> &operation) {
  if (!resolve_operation(operation->value))
    return false;

  return resolve_sequence(operation->parameters);
}

bool Parser::resolve_function_definition(
    std::shared_ptr<OperationFunctionDefinition> &operation) {
  if (!resolve_data_type(operation->data_type))
    return false;

  push_stack(operation);
  for (auto i = operation->parameters.begin(); i != operation->parameters.end();
       i++)
    add_stack_variable(*i);
  return resolve_sequence(operation->body);
}

bool Parser::resolve_type_definition(
    std::shared_ptr<OperationTypeDefinition> &operation) {
  push_stack(operation);
  return resolve_sequence(operation->body);
}

bool Parser::resolve_return(std::shared_ptr<OperationReturn> &operation) {
  return resolve_operation(operation->value);
}

bool Parser::resolve_assert(std::shared_ptr<OperationAssert> &operation) {
  return resolve_operation(operation->expression);
}

bool Parser::resolve_member(std::shared_ptr<OperationMember> &operation) {
  if (!resolve_operation(operation->value))
    return false;

  auto data_type = operation->value->get_data_type();

  auto definition = find_type(data_type);
  if (definition == nullptr) { // FIXME: Should always resolve
    set_error(operation->member,
              "Can't access members of data type " + data_type);
    return false;
  }
  operation->type_definition = definition;

  auto member_name = operation->get_member_name();

  auto primitive_definition =
      std::dynamic_pointer_cast<OperationPrimitiveDefinition>(definition);
  if (primitive_definition != nullptr) {
    if (primitive_definition->find_member(member_name) == nullptr) {
      set_error(operation->member, "Primitive type " + data_type +
                                       " doesn't have a member named " +
                                       member_name);
      return false;
    }
  }

  auto type_definition =
      std::dynamic_pointer_cast<OperationTypeDefinition>(definition);
  if (type_definition != nullptr) {
    if (type_definition->find_member(member_name) == nullptr) {
      set_error(operation->member, "Data type " + data_type +
                                       " doesn't have a member named " +
                                       member_name);
      return false;
    }
  }

  return true;
}

bool Parser::resolve_binary(std::shared_ptr<OperationBinary> &operation) {
  if (!resolve_operation(operation->a) || !resolve_operation(operation->b))
    return false;

  auto a_type = operation->a->get_data_type();
  auto b_type = operation->b->get_data_type();
  if (a_type != b_type) {
    auto converted_a = convert_to_data_type(operation->a, b_type);
    auto converted_b = convert_to_data_type(operation->b, a_type);
    if (converted_a != nullptr)
      operation->a = converted_a;
    else if (converted_b != nullptr)
      operation->b = converted_b;
    else {
      set_error(operation->op,
                "Can't combine " + a_type + " and " + b_type + " types");
      return false;
    }
  }

  return true;
}

bool Parser::resolve_convert(std::shared_ptr<OperationConvert> &operation) {
  return resolve_operation(operation->op);
}

static std::shared_ptr<OperationModule>
parse_module(std::shared_ptr<OperationModule> core_module, const char *data,
             size_t data_length) {
  Parser parser(data, data_length);

  parser.core_module = core_module;
  parser.tokens = elf_lex(data, data_length);

  auto module = std::make_shared<OperationModule>();
  parser.push_stack(module);

  if (!parser.parse_sequence()) {
    parser.print_error();
    return nullptr;
  }

  if (!parser.resolve_operation(module)) {
    parser.print_error();
    return nullptr;
  }

  if (parser.current_token()->type != TOKEN_TYPE_EOF) {
    printf("Expected end of input\n");
    return nullptr;
  }

  return module;
}

std::shared_ptr<OperationModule> elf_parse(const char *data,
                                           size_t data_length) {
  std::string core_module_source =
      "primitive bool {}\n"
      "primitive uint8 {}\n"
      "primitive int8 {}\n"
      "primitive uint16 {}\n"
      "primitive int16 {}\n"
      "primitive uint32 {}\n"
      "primitive int32 {}\n"
      "primitive uint64 {}\n"
      "primitive int64 {}\n"
      "primitive utf8 {}\n"; // FIXME: Doesn't need to be primitive?

  auto core_module = parse_module(nullptr, core_module_source.c_str(),
                                  core_module_source.size());
  if (core_module == nullptr)
    return nullptr;

  return parse_module(core_module, data, data_length);
}
