/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "elf-token.h"

struct Operation {
  virtual ~Operation() {}
  virtual bool is_constant() { return false; }
  virtual std::string get_data_type() { return nullptr; }
  virtual void add_child(std::shared_ptr<Operation> child) {}
  virtual size_t get_n_children() { return 0; }
  virtual std::shared_ptr<Operation> get_child(size_t index) { return nullptr; }
  std::shared_ptr<Operation> get_last_child();
  virtual std::string to_string() = 0;
};

struct OperationModule : Operation {
  std::vector<std::shared_ptr<Operation>> body;

  bool is_constant() { return true; };
  void add_child(std::shared_ptr<Operation> child) { body.push_back(child); }
  size_t get_n_children() { return body.size(); }
  std::shared_ptr<Operation> get_child(size_t index) { return body[index]; }
  std::string to_string() { return "MODULE"; }
};

struct OperationPrimitiveDefinition : Operation {
  std::shared_ptr<Token> name;
  std::vector<std::shared_ptr<Operation>> body;

  OperationPrimitiveDefinition(std::shared_ptr<Token> &name) : name(name) {}
  std::string get_data_type() { return name->get_text(); }
  void add_child(std::shared_ptr<Operation> child) { body.push_back(child); }
  size_t get_n_children() { return body.size(); }
  std::shared_ptr<Operation> get_child(size_t index) { return body[index]; }
  std::string to_string() { return "PRIMITIVE_DEFINITION"; }

  std::shared_ptr<Operation> find_member(const std::string &name);
};

struct OperationTypeDefinition : Operation {
  std::shared_ptr<Token> name;
  std::vector<std::shared_ptr<Operation>> body;

  OperationTypeDefinition(std::shared_ptr<Token> &name) : name(name) {}
  std::string get_data_type() { return name->get_text(); }
  void add_child(std::shared_ptr<Operation> child) { body.push_back(child); }
  size_t get_n_children() { return body.size(); }
  std::shared_ptr<Operation> get_child(size_t index) { return body[index]; }
  std::string to_string() { return "TYPE_DEFINITION"; }

  std::shared_ptr<Operation> find_member(const std::string &name);
};

struct OperationDataType : Operation {
  std::shared_ptr<Token> name;
  std::shared_ptr<Operation> type_definition;

  OperationDataType(std::shared_ptr<Token> name) : name(name) {}
  std::string get_data_type() { return name->get_text(); }
  std::string to_string() { return "DATA_TYPE"; }
};

struct OperationVariableDefinition : Operation {
  std::shared_ptr<OperationDataType> data_type;
  std::shared_ptr<Token> name;
  std::shared_ptr<Operation> value;

  OperationVariableDefinition(std::shared_ptr<OperationDataType> data_type,
                              std::shared_ptr<Token> name,
                              std::shared_ptr<Operation> value)
      : data_type(data_type), name(name), value(value) {}
  bool is_constant() { return value == nullptr || value->is_constant(); }
  std::string get_data_type() { return data_type->get_data_type(); }
  std::string to_string() { return "VARIABLE_DEFINITION"; }
};

struct OperationSymbol : Operation {
  std::shared_ptr<Token> name;
  std::shared_ptr<Operation> definition;

  OperationSymbol(std::shared_ptr<Token> name) : name(name) {}
  std::string get_data_type() {
    return definition != nullptr ? definition->get_data_type() : nullptr;
  }
  std::string to_string() { return "SYMBOL"; }
};

struct OperationAssignment : Operation {
  std::shared_ptr<Operation> target;
  std::shared_ptr<Operation> value;

  OperationAssignment(std::shared_ptr<Operation> target,
                      std::shared_ptr<Operation> &value)
      : target(target), value(value) {}
  bool is_constant() { return target->is_constant() && value->is_constant(); }
  std::string get_data_type() { return target->get_data_type(); }
  std::string to_string() { return "ASSIGNMENT"; }
};

struct OperationElse;

struct OperationIf : Operation {
  std::shared_ptr<Token> keyword;
  std::shared_ptr<Operation> condition;
  std::vector<std::shared_ptr<Operation>> body;
  std::shared_ptr<OperationElse> else_operation;

  OperationIf(std::shared_ptr<Token> keyword,
              std::shared_ptr<Operation> condition)
      : keyword(keyword), condition(condition), else_operation(nullptr) {}
  void add_child(std::shared_ptr<Operation> child) { body.push_back(child); }
  size_t get_n_children() { return body.size(); }
  std::shared_ptr<Operation> get_child(size_t index) { return body[index]; }
  std::string to_string() { return "IF"; }
};

struct OperationElse : Operation {
  std::shared_ptr<Token> keyword;
  std::vector<std::shared_ptr<Operation>> body;

  OperationElse(std::shared_ptr<Token> keyword) : keyword(keyword){};
  void add_child(std::shared_ptr<Operation> child) { body.push_back(child); }
  size_t get_n_children() { return body.size(); }
  std::shared_ptr<Operation> get_child(size_t index) { return body[index]; }
  std::string to_string() { return "ELSE"; }
};

struct OperationWhile : Operation {
  std::shared_ptr<Operation> condition;
  std::vector<std::shared_ptr<Operation>> body;

  OperationWhile(std::shared_ptr<Operation> condition) : condition(condition) {}
  void add_child(std::shared_ptr<Operation> child) { body.push_back(child); }
  size_t get_n_children() { return body.size(); }
  std::shared_ptr<Operation> get_child(size_t index) { return body[index]; }
  std::string to_string() { return "WHILE"; }
};

struct OperationFunctionDefinition : Operation {
  std::shared_ptr<OperationFunctionDefinition> parent;
  std::shared_ptr<OperationDataType> data_type;
  std::shared_ptr<Token> name;
  std::vector<std::shared_ptr<OperationVariableDefinition>> parameters;
  std::vector<std::shared_ptr<Operation>> body;

  OperationFunctionDefinition(
      std::shared_ptr<OperationDataType> data_type, std::shared_ptr<Token> name,
      std::vector<std::shared_ptr<OperationVariableDefinition>> parameters)
      : data_type(data_type), name(name), parameters(parameters) {}
  bool is_constant();
  std::string get_data_type() { return data_type->get_data_type(); }
  void add_child(std::shared_ptr<Operation> child) { body.push_back(child); }
  size_t get_n_children() { return body.size(); }
  std::shared_ptr<Operation> get_child(size_t index) { return body[index]; }
  std::string to_string() { return "FUNCTION_DEFINITION"; }
};

struct OperationCall : Operation {
  std::shared_ptr<Operation> value;
  std::vector<std::shared_ptr<Operation>> parameters;

  OperationCall(std::shared_ptr<Operation> &value,
                std::vector<std::shared_ptr<Operation>> &parameters)
      : value(value), parameters(parameters) {}
  bool is_constant();
  std::string get_data_type() { return value->get_data_type(); }
  std::string to_string() { return "CALL"; }
};

struct OperationReturn : Operation {
  std::shared_ptr<Operation> value;
  std::shared_ptr<OperationFunctionDefinition> function;

  OperationReturn(std::shared_ptr<Operation> value,
                  std::shared_ptr<OperationFunctionDefinition> function)
      : value(value), function(function) {}
  bool is_constant() { return value == nullptr || value->is_constant(); }
  std::string get_data_type() { return function->get_data_type(); }
  std::string to_string() { return "RETURN(" + value->to_string() + ")"; }
};

struct OperationAssert : Operation {
  std::shared_ptr<Token> name;
  std::shared_ptr<Operation> expression;

  OperationAssert(std::shared_ptr<Token> name,
                  std::shared_ptr<Operation> expression)
      : name(name), expression(expression) {}
  bool is_constant() {
    return expression == nullptr || expression->is_constant();
  }
  std::string to_string() { return "ASSERT(" + expression->to_string() + ")"; }
};

struct OperationTrue : Operation {
  std::shared_ptr<Token> token;

  OperationTrue(std::shared_ptr<Token> &token) : token(token) {}
  bool is_constant() { return true; }
  std::string get_data_type() { return "bool"; }
  std::string to_string() { return "TRUE"; }
};

struct OperationFalse : Operation {
  std::shared_ptr<Token> token;

  OperationFalse(std::shared_ptr<Token> &token) : token(token) {}
  bool is_constant() { return true; }
  std::string get_data_type() { return "bool"; }
  std::string to_string() { return "FALSE"; }
};

struct OperationNumberConstant : Operation {
  std::string data_type;
  std::shared_ptr<Token> sign_token;
  std::shared_ptr<Token> magnitude_token;
  uint64_t magnitude;

  OperationNumberConstant(const std::string &data_type,
                          std::shared_ptr<Token> &magnitude_token,
                          uint64_t magnitude)
      : data_type(data_type), sign_token(nullptr),
        magnitude_token(magnitude_token), magnitude(magnitude) {}
  OperationNumberConstant(const std::string &data_type,
                          std::shared_ptr<Token> &sign_token,
                          std::shared_ptr<Token> &magnitude_token,
                          uint64_t magnitude)
      : data_type(data_type), sign_token(sign_token),
        magnitude_token(magnitude_token), magnitude(magnitude) {}
  bool is_constant() { return true; }
  std::string get_data_type() { return data_type; }
  std::string to_string() {
    return "NUMBER_CONSTANT(" + (sign_token != nullptr)
               ? "-"
               : "" + std::to_string(magnitude) + ")";
  }
};

struct OperationTextConstant : Operation {
  std::shared_ptr<Token> token;
  std::string value;

  OperationTextConstant(std::shared_ptr<Token> &token, const std::string &value)
      : token(token), value(value) {}
  bool is_constant() { return true; }
  std::string get_data_type() { return "utf8"; }
  std::string to_string() { return "TEXT_CONSTANT(" + value + ")"; }
};

struct OperationMember : Operation {
  std::shared_ptr<Operation> value;
  std::shared_ptr<Token> member;
  std::shared_ptr<Operation> type_definition;

  OperationMember(std::shared_ptr<Operation> value,
                  std::shared_ptr<Token> member)
      : value(value), member(member) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string() { return "MEMBER(" + member->to_string() + ")"; }
  std::string get_member_name() {
    return std::string(member->data + member->offset + 1, member->length - 1);
  }
};

struct OperationUnary : Operation {
  std::shared_ptr<Token> op;
  std::shared_ptr<Operation> value;

  OperationUnary(std::shared_ptr<Token> op, std::shared_ptr<Operation> value)
      : op(op), value(value) {}
  bool is_constant() { return value->is_constant(); }
  std::string get_data_type();
  std::string to_string() { return "UNARY"; }
};

struct OperationBinary : Operation {
  std::shared_ptr<Token> op;
  std::shared_ptr<Operation> a;
  std::shared_ptr<Operation> b;

  OperationBinary(std::shared_ptr<Token> op, std::shared_ptr<Operation> a,
                  std::shared_ptr<Operation> b)
      : op(op), a(a), b(b) {}
  bool is_constant() { return a->is_constant() && b->is_constant(); }
  std::string get_data_type();
  std::string to_string() { return "BINARY"; }
};

struct OperationConvert : Operation {
  std::shared_ptr<Operation> op;
  std::string data_type;

  OperationConvert(std::shared_ptr<Operation> &op, const std::string &data_type)
      : op(op), data_type(data_type) {}
  bool is_constant() { return op->is_constant(); }
  std::string get_data_type() { return data_type; }
  std::string to_string() { return "CONVERT"; }
};
