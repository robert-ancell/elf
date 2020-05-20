/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdbool.h>
#include <string>

#include "elf-token.h"

struct Operation {
  int ref_count;

  Operation() : ref_count(1) {}
  virtual ~Operation() {}
  virtual bool is_constant() { return false; }
  virtual std::string get_data_type(const char *data) { return nullptr; }
  virtual void add_child(Operation *child) {}
  virtual size_t get_n_children() { return 0; }
  virtual Operation *get_child(size_t index) { return nullptr; }
  Operation *get_last_child();
  virtual std::string to_string() = 0;
  Operation *ref() {
    ref_count++;
    return this;
  }
  void unref() {
    ref_count--;
    if (ref_count == 0)
      delete this;
  }
};

struct OperationModule : Operation {
  Operation **body;
  size_t body_length;

  OperationModule() : body(nullptr), body_length(0) {}
  ~OperationModule();
  bool is_constant() { return true; };
  void add_child(Operation *child);
  size_t get_n_children() { return body_length; }
  Operation *get_child(size_t index) { return body[index]; }
  std::string to_string() { return "MODULE"; }
};

struct OperationVariableDefinition : Operation {
  Token *data_type;
  Token *name;
  Operation *value;

  OperationVariableDefinition(Token *data_type, Token *name, Operation *value)
      : data_type(data_type->ref()), name(name->ref()),
        value(value ? value->ref() : nullptr) {}
  ~OperationVariableDefinition() { value->unref(); }
  bool is_constant() { return value == nullptr || value->is_constant(); }
  std::string get_data_type(const char *data) {
    return data_type->get_text(data);
  }
  std::string to_string() { return "VARIABLE_DEFINITION"; }
};

struct OperationVariableAssignment : Operation {
  Token *name;
  Operation *value;
  OperationVariableDefinition *variable;

  OperationVariableAssignment(Token *name, Operation *value,
                              OperationVariableDefinition *variable)
      : name(name->ref()), value(value->ref()), variable(variable) {}
  ~OperationVariableAssignment() {
    name->unref();
    value->unref();
  }
  bool is_constant() { return value->is_constant(); }
  std::string get_data_type(const char *data) {
    return variable->get_data_type(data);
  }
  std::string to_string() { return "VARIABLE_ASSIGNMENT"; }
};

struct OperationElse;

struct OperationIf : Operation {
  Token *keyword;
  Operation *condition;
  Operation **body;
  size_t body_length;
  OperationElse *else_operation;

  OperationIf(Token *keyword, Operation *condition)
      : keyword(keyword->ref()), condition(condition->ref()), body(nullptr),
        body_length(0), else_operation(nullptr) {}
  ~OperationIf();
  void add_child(Operation *child);
  size_t get_n_children() { return body_length; }
  Operation *get_child(size_t index) { return body[index]; }
  std::string to_string() { return "IF"; }
};

struct OperationElse : Operation {
  Token *keyword;
  Operation **body;
  size_t body_length;

  OperationElse(Token *keyword)
      : keyword(keyword->ref()), body(nullptr), body_length(0){};
  ~OperationElse();
  void add_child(Operation *child);
  size_t get_n_children() { return body_length; }
  Operation *get_child(size_t index) { return body[index]; }
  std::string to_string() { return "ELSE"; }
};

struct OperationWhile : Operation {
  Operation *condition;
  Operation **body;
  size_t body_length;

  OperationWhile(Operation *condition)
      : condition(condition->ref()), body(nullptr), body_length(0) {}
  ~OperationWhile();
  void add_child(Operation *child);
  size_t get_n_children() { return body_length; }
  Operation *get_child(size_t index) { return body[index]; }
  std::string to_string() { return "WHILE"; }
};

struct OperationFunctionDefinition : Operation {
  OperationFunctionDefinition *parent;
  Token *data_type;
  Token *name;
  OperationVariableDefinition **parameters;
  Operation **body;
  size_t body_length;

  OperationFunctionDefinition(Token *data_type, Token *name,
                              OperationVariableDefinition **parameters)
      : data_type(data_type->ref()), name(name->ref()), parameters(parameters),
        body(nullptr), body_length(0) {}
  ~OperationFunctionDefinition();
  bool is_constant();
  std::string get_data_type(const char *data) {
    return data_type->get_text(data);
  }
  void add_child(Operation *child);
  size_t get_n_children() { return body_length; }
  Operation *get_child(size_t index) { return body[index]; }
  std::string to_string() { return "FUNCTION_DEFINITION"; }
};

struct OperationFunctionCall : Operation {
  Token *name;
  Operation **parameters;
  OperationFunctionDefinition *function;

  OperationFunctionCall(Token *name, Operation **parameters,
                        OperationFunctionDefinition *function)
      : name(name->ref()), parameters(parameters), function(function) {}
  ~OperationFunctionCall();
  bool is_constant();
  std::string get_data_type(const char *data) {
    return function->get_data_type(data);
  }
  std::string to_string() { return "FUNCTION_CALL"; }
};

struct OperationReturn : Operation {
  Operation *value;
  OperationFunctionDefinition *function;

  OperationReturn(Operation *value, OperationFunctionDefinition *function)
      : value(value->ref()), function(function) {}
  ~OperationReturn() { value->unref(); }
  bool is_constant() { return value == nullptr || value->is_constant(); }
  std::string get_data_type(const char *data) {
    return function->get_data_type(data);
  }
  std::string to_string() { return "RETURN(" + value->to_string() + ")"; }
};

struct OperationAssert : Operation {
  Token *name;
  Operation *expression;

  OperationAssert(Token *name, Operation *expression)
      : name(name->ref()), expression(expression->ref()) {}
  ~OperationAssert() {
    name->unref();
    expression->unref();
  }
  bool is_constant() {
    return expression == nullptr || expression->is_constant();
  }
  std::string to_string() { return "ASSERT(" + expression->to_string() + ")"; }
};

struct OperationBooleanConstant : Operation {
  Token *value;

  OperationBooleanConstant(Token *value) : value(value->ref()) {}
  ~OperationBooleanConstant() { value->unref(); }
  bool is_constant() { return true; }
  std::string get_data_type(const char *data) { return "bool"; }
  std::string to_string() {
    return "BOOLEAN_CONSTANT(" + value->to_string() + ")";
  }
};

struct OperationNumberConstant : Operation {
  Token *value;

  OperationNumberConstant(Token *value) : value(value->ref()) {}
  ~OperationNumberConstant() { value->unref(); }
  bool is_constant() { return true; }
  std::string get_data_type(const char *data);
  std::string to_string() {
    return "NUMBER_CONSTANT(" + value->to_string() + ")";
  }
};

struct OperationTextConstant : Operation {
  Token *value;

  OperationTextConstant(Token *value) : value(value->ref()) {}
  ~OperationTextConstant() { value->unref(); }
  bool is_constant() { return true; }
  std::string get_data_type(const char *data) { return "utf8"; }
  std::string to_string() {
    return "TEXT_CONSTANT(" + value->to_string() + ")";
  }
};

struct OperationVariableValue : Operation {
  Token *name;
  OperationVariableDefinition *variable;

  OperationVariableValue(Token *name, OperationVariableDefinition *variable)
      : name(name->ref()), variable(variable) {}
  ~OperationVariableValue() { name->unref(); }
  bool is_constant();
  std::string get_data_type(const char *data) {
    return variable->get_data_type(data);
  }
  std::string to_string() { return "VARIABLE_VALUE"; }
};

struct OperationMemberValue : Operation {
  Operation *object;
  Token *member;
  Operation **parameters;

  OperationMemberValue(Operation *object, Token *member, Operation **parameters)
      : object(object->ref()), member(member->ref()), parameters(parameters) {}
  ~OperationMemberValue();
  bool is_constant();
  std::string get_data_type(const char *data);
  std::string to_string() {
    return "MEMBER_VALUE(" + member->to_string() + ")";
  }
};

struct OperationBinary : Operation {
  Token *op;
  Operation *a;
  Operation *b;

  OperationBinary(Token *op, Operation *a, Operation *b)
      : op(op->ref()), a(a->ref()), b(b->ref()) {}
  ~OperationBinary() {
    op->unref();
    a->unref();
    b->unref();
  }
  bool is_constant() { return a->is_constant() && b->is_constant(); }
  std::string get_data_type(const char *data);
  std::string to_string() { return "BINARY"; }
};

void operation_cleanup(Operation **operation);

#define autofree_operation                                                     \
  __attribute__((cleanup(operation_cleanup))) Operation *
