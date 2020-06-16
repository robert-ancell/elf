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
  std::vector<std::shared_ptr<Operation>> children;

  virtual ~Operation() {}
  virtual bool is_constant() { return false; }
  virtual std::string get_data_type() { return nullptr; }
  virtual std::string to_string() = 0;
};

struct OperationModule : Operation {
  bool is_constant();
  std::string to_string();
};

struct OperationPrimitiveDefinition : Operation {
  std::shared_ptr<Token> name;

  OperationPrimitiveDefinition(std::shared_ptr<Token> &name) : name(name) {}
  std::string get_data_type();
  std::string to_string();
  std::shared_ptr<Operation> find_member(const std::string &name);
};

struct OperationTypeDefinition : Operation {
  std::shared_ptr<Token> name;

  OperationTypeDefinition(std::shared_ptr<Token> &name) : name(name) {}
  std::string get_data_type();
  std::string to_string();
  std::shared_ptr<Operation> find_member(const std::string &name);
};

struct OperationDataType : Operation {
  std::shared_ptr<Token> name;
  bool is_array;
  std::shared_ptr<Operation> type_definition;

  OperationDataType(std::shared_ptr<Token> name, bool is_array)
      : name(name), is_array(is_array) {}
  std::string get_data_type();
  std::string to_string();
};

struct OperationVariableDefinition : Operation {
  std::shared_ptr<OperationDataType> data_type;
  std::shared_ptr<Token> name;
  std::shared_ptr<Operation> value;

  OperationVariableDefinition(std::shared_ptr<OperationDataType> data_type,
                              std::shared_ptr<Token> name,
                              std::shared_ptr<Operation> value)
      : data_type(data_type), name(name), value(value) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationSymbol : Operation {
  std::shared_ptr<Token> name;
  std::shared_ptr<Operation> definition;

  OperationSymbol(std::shared_ptr<Token> name) : name(name) {}
  std::string get_data_type();
  std::string to_string();
};

struct OperationAssignment : Operation {
  std::shared_ptr<Operation> target;
  std::shared_ptr<Operation> value;

  OperationAssignment(std::shared_ptr<Operation> target,
                      std::shared_ptr<Operation> &value)
      : target(target), value(value) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationElse;

struct OperationIf : Operation {
  std::shared_ptr<Token> keyword;
  std::shared_ptr<Operation> condition;
  std::shared_ptr<OperationElse> else_operation;

  OperationIf(std::shared_ptr<Token> keyword,
              std::shared_ptr<Operation> condition)
      : keyword(keyword), condition(condition), else_operation(nullptr) {}
  std::string to_string();
};

struct OperationElse : Operation {
  std::shared_ptr<Token> keyword;

  OperationElse(std::shared_ptr<Token> keyword) : keyword(keyword){};
  std::string to_string();
};

struct OperationWhile : Operation {
  std::shared_ptr<Operation> condition;

  OperationWhile(std::shared_ptr<Operation> condition) : condition(condition) {}
  std::string to_string();
};

struct OperationFunctionDefinition : Operation {
  std::shared_ptr<OperationFunctionDefinition> parent;
  std::shared_ptr<OperationDataType> data_type;
  std::shared_ptr<Token> name;
  std::vector<std::shared_ptr<OperationVariableDefinition>> parameters;

  OperationFunctionDefinition(
      std::shared_ptr<OperationDataType> data_type, std::shared_ptr<Token> name,
      std::vector<std::shared_ptr<OperationVariableDefinition>> parameters)
      : data_type(data_type), name(name), parameters(parameters) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationCall : Operation {
  std::shared_ptr<Operation> value;
  std::vector<std::shared_ptr<Operation>> parameters;

  OperationCall(std::shared_ptr<Operation> &value,
                std::vector<std::shared_ptr<Operation>> &parameters)
      : value(value), parameters(parameters) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationReturn : Operation {
  std::shared_ptr<Operation> value;
  std::shared_ptr<OperationFunctionDefinition> function;

  OperationReturn(std::shared_ptr<Operation> value,
                  std::shared_ptr<OperationFunctionDefinition> function)
      : value(value), function(function) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationAssert : Operation {
  std::shared_ptr<Token> name;
  std::shared_ptr<Operation> expression;

  OperationAssert(std::shared_ptr<Token> name,
                  std::shared_ptr<Operation> expression)
      : name(name), expression(expression) {}
  bool is_constant();
  std::string to_string();
};

struct OperationTrue : Operation {
  std::shared_ptr<Token> token;

  OperationTrue(std::shared_ptr<Token> &token) : token(token) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationFalse : Operation {
  std::shared_ptr<Token> token;

  OperationFalse(std::shared_ptr<Token> &token) : token(token) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
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
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationTextConstant : Operation {
  std::shared_ptr<Token> token;
  std::string value;

  OperationTextConstant(std::shared_ptr<Token> &token, const std::string &value)
      : token(token), value(value) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationArrayConstant : Operation {
  std::vector<std::shared_ptr<Operation>> values;

  OperationArrayConstant(std::vector<std::shared_ptr<Operation>> &values)
      : values(values) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
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
  std::string to_string();
  std::string get_member_name();
};

struct OperationUnary : Operation {
  std::shared_ptr<Token> op;
  std::shared_ptr<Operation> value;

  OperationUnary(std::shared_ptr<Token> op, std::shared_ptr<Operation> value)
      : op(op), value(value) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationBinary : Operation {
  std::shared_ptr<Token> op;
  std::shared_ptr<Operation> a;
  std::shared_ptr<Operation> b;

  OperationBinary(std::shared_ptr<Token> op, std::shared_ptr<Operation> a,
                  std::shared_ptr<Operation> b)
      : op(op), a(a), b(b) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};

struct OperationConvert : Operation {
  std::shared_ptr<Operation> op;
  std::string data_type;

  OperationConvert(std::shared_ptr<Operation> &op, const std::string &data_type)
      : op(op), data_type(data_type) {}
  bool is_constant();
  std::string get_data_type();
  std::string to_string();
};
