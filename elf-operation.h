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
  virtual std::string get_data_type(const char *data) { return nullptr; }
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

struct OperationVariableDefinition : Operation {
  std::shared_ptr<Token> data_type;
  std::shared_ptr<Token> name;
  std::shared_ptr<Operation> value;

  OperationVariableDefinition(std::shared_ptr<Token> data_type,
                              std::shared_ptr<Token> name,
                              std::shared_ptr<Operation> value)
      : data_type(data_type), name(name), value(value ? value : nullptr) {}
  bool is_constant() { return value == nullptr || value->is_constant(); }
  std::string get_data_type(const char *data) {
    return data_type->get_text(data);
  }
  std::string to_string() { return "VARIABLE_DEFINITION"; }
};

struct OperationVariableAssignment : Operation {
  std::shared_ptr<Token> name;
  std::shared_ptr<Operation> value;
  std::shared_ptr<OperationVariableDefinition> variable;

  OperationVariableAssignment(
      std::shared_ptr<Token> name, std::shared_ptr<Operation> value,
      std::shared_ptr<OperationVariableDefinition> variable)
      : name(name), value(value), variable(variable) {}
  bool is_constant() { return value->is_constant(); }
  std::string get_data_type(const char *data) {
    return variable->get_data_type(data);
  }
  std::string to_string() { return "VARIABLE_ASSIGNMENT"; }
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
  std::shared_ptr<Token> data_type;
  std::shared_ptr<Token> name;
  std::vector<std::shared_ptr<OperationVariableDefinition>> parameters;
  std::vector<std::shared_ptr<Operation>> body;

  OperationFunctionDefinition(
      std::shared_ptr<Token> data_type, std::shared_ptr<Token> name,
      std::vector<std::shared_ptr<OperationVariableDefinition>> parameters)
      : data_type(data_type), name(name), parameters(parameters) {}
  bool is_constant();
  std::string get_data_type(const char *data) {
    return data_type->get_text(data);
  }
  void add_child(std::shared_ptr<Operation> child) { body.push_back(child); }
  size_t get_n_children() { return body.size(); }
  std::shared_ptr<Operation> get_child(size_t index) { return body[index]; }
  std::string to_string() { return "FUNCTION_DEFINITION"; }
};

struct OperationFunctionCall : Operation {
  std::shared_ptr<Token> name;
  std::vector<std::shared_ptr<Operation>> parameters;
  std::shared_ptr<OperationFunctionDefinition> function;

  OperationFunctionCall(std::shared_ptr<Token> name,
                        std::vector<std::shared_ptr<Operation>> parameters,
                        std::shared_ptr<OperationFunctionDefinition> function)
      : name(name), parameters(parameters), function(function) {}
  bool is_constant();
  std::string get_data_type(const char *data) {
    return function->get_data_type(data);
  }
  std::string to_string() { return "FUNCTION_CALL"; }
};

struct OperationReturn : Operation {
  std::shared_ptr<Operation> value;
  std::shared_ptr<OperationFunctionDefinition> function;

  OperationReturn(std::shared_ptr<Operation> value,
                  std::shared_ptr<OperationFunctionDefinition> function)
      : value(value), function(function) {}
  bool is_constant() { return value == nullptr || value->is_constant(); }
  std::string get_data_type(const char *data) {
    return function->get_data_type(data);
  }
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

struct OperationBooleanConstant : Operation {
  std::shared_ptr<Token> value;

  OperationBooleanConstant(std::shared_ptr<Token> value) : value(value) {}
  bool is_constant() { return true; }
  std::string get_data_type(const char *data) { return "bool"; }
  std::string to_string() {
    return "BOOLEAN_CONSTANT(" + value->to_string() + ")";
  }
};

struct OperationNumberConstant : Operation {
  std::shared_ptr<Token> value;

  OperationNumberConstant(std::shared_ptr<Token> value) : value(value) {}
  bool is_constant() { return true; }
  std::string get_data_type(const char *data);
  std::string to_string() {
    return "NUMBER_CONSTANT(" + value->to_string() + ")";
  }
};

struct OperationTextConstant : Operation {
  std::shared_ptr<Token> value;

  OperationTextConstant(std::shared_ptr<Token> value) : value(value) {}
  bool is_constant() { return true; }
  std::string get_data_type(const char *data) { return "utf8"; }
  std::string to_string() {
    return "TEXT_CONSTANT(" + value->to_string() + ")";
  }
};

struct OperationVariableValue : Operation {
  std::shared_ptr<Token> name;
  std::shared_ptr<OperationVariableDefinition> variable;

  OperationVariableValue(std::shared_ptr<Token> name,
                         std::shared_ptr<OperationVariableDefinition> variable)
      : name(name), variable(variable) {}
  bool is_constant();
  std::string get_data_type(const char *data) {
    return variable->get_data_type(data);
  }
  std::string to_string() { return "VARIABLE_VALUE"; }
};

struct OperationMemberValue : Operation {
  std::shared_ptr<Operation> object;
  std::shared_ptr<Token> member;
  std::vector<std::shared_ptr<Operation>> parameters;

  OperationMemberValue(std::shared_ptr<Operation> object,
                       std::shared_ptr<Token> member,
                       std::vector<std::shared_ptr<Operation>> parameters)
      : object(object), member(member), parameters(parameters) {}
  bool is_constant();
  std::string get_data_type(const char *data);
  std::string to_string() {
    return "MEMBER_VALUE(" + member->to_string() + ")";
  }
};

struct OperationBinary : Operation {
  std::shared_ptr<Token> op;
  std::shared_ptr<Operation> a;
  std::shared_ptr<Operation> b;

  OperationBinary(std::shared_ptr<Token> op, std::shared_ptr<Operation> a,
                  std::shared_ptr<Operation> b)
      : op(op), a(a), b(b) {}
  bool is_constant() { return a->is_constant() && b->is_constant(); }
  std::string get_data_type(const char *data);
  std::string to_string() { return "BINARY"; }
};
