/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-runner.h"

#include <assert.h>
#include <memory>
#include <stdio.h>

struct DataValue {
  virtual ~DataValue() {}
  virtual void print() = 0;
};

struct DataValueNone : DataValue {
  void print() { printf("none\n"); }
};

struct DataValueBool : DataValue {
  bool value;

  DataValueBool(bool value) : value(value) {}
  ~DataValueBool() {}
  void print() { printf(value ? "true\n" : "false\n"); }
};

struct DataValueUint8 : DataValue {
  uint8_t value;

  DataValueUint8(uint8_t value) : value(value) {}
  void print() { printf("%d\n", value); }
};

struct DataValueUint16 : DataValue {
  uint16_t value;

  DataValueUint16(uint16_t value) : value(value) {}
  void print() { printf("%d\n", value); }
};

struct DataValueUint32 : DataValue {
  uint32_t value;

  DataValueUint32(uint32_t value) : value(value) {}
  void print() { printf("%d\n", value); }
};

struct DataValueUint64 : DataValue {
  uint64_t value;

  DataValueUint64(uint64_t value) : value(value) {}
  void print() { printf("%lu\n", value); }
};

struct DataValueUtf8 : DataValue {
  std::string value;

  DataValueUtf8(std::string value) : value(value) {}
  void print() { printf("%s\n", value.c_str()); }
};

struct Variable {
  std::string name;
  std::shared_ptr<DataValue> value;

  Variable(std::string name, std::shared_ptr<DataValue> value)
      : name(name), value(value) {}
};

struct ProgramState {
  const char *data;

  std::shared_ptr<DataValueNone> none_value;

  std::vector<Variable *> variables;

  std::shared_ptr<DataValue> return_value;

  OperationAssert *failed_assertion;

  ProgramState(const char *data)
      : data(data), return_value(nullptr), failed_assertion(nullptr) {}

  ~ProgramState() {
    for (auto i = variables.begin(); i != variables.end(); i++)
      delete *i;
  }

  void run_sequence(std::vector<Operation *> &body);
  std::shared_ptr<DataValue> run_module(OperationModule *module);
  std::shared_ptr<DataValue>
  run_function(OperationFunctionDefinition *function);
  std::shared_ptr<DataValue> make_default_value(Token *data_type);
  void add_variable(std::string name, std::shared_ptr<DataValue> value);
  std::shared_ptr<DataValue>
  run_variable_definition(OperationVariableDefinition *operation);
  std::shared_ptr<DataValue>
  run_variable_assignment(OperationVariableAssignment *operation);
  std::shared_ptr<DataValue> run_if(OperationIf *operation);
  std::shared_ptr<DataValue> run_while(OperationWhile *operation);
  std::shared_ptr<DataValue>
  run_function_call(OperationFunctionCall *operation);
  std::shared_ptr<DataValue> run_return(OperationReturn *operation);
  std::shared_ptr<DataValue> run_assert(OperationAssert *operation);
  std::shared_ptr<DataValue>
  run_boolean_constant(OperationBooleanConstant *operation);
  std::shared_ptr<DataValue>
  run_number_constant(OperationNumberConstant *operation);
  std::shared_ptr<DataValue>
  run_text_constant(OperationTextConstant *operation);
  std::shared_ptr<DataValue>
  run_variable_value(OperationVariableValue *operation);
  std::shared_ptr<DataValue> run_member_value(OperationMemberValue *operation);
  std::shared_ptr<DataValue>
  run_binary_boolean(OperationBinary *operation,
                     std::shared_ptr<DataValueBool> a,
                     std::shared_ptr<DataValueBool> b);
  std::shared_ptr<DataValue>
  run_binary_integer(OperationBinary *operation,
                     std::shared_ptr<DataValueUint8> a,
                     std::shared_ptr<DataValueUint8> b);
  std::shared_ptr<DataValue> run_binary_text(OperationBinary *operation,
                                             std::shared_ptr<DataValueUtf8> a,
                                             std::shared_ptr<DataValueUtf8> b);
  std::shared_ptr<DataValue> run_binary(OperationBinary *operation);
  std::shared_ptr<DataValue> run_operation(Operation *operation);
};

void ProgramState::run_sequence(std::vector<Operation *> &body) {
  for (auto i = body.begin();
       i != body.end() && failed_assertion == NULL && return_value == NULL;
       i++) {
    std::shared_ptr<DataValue> value = run_operation(*i);
  }
}

std::shared_ptr<DataValue> ProgramState::run_module(OperationModule *module) {
  run_sequence(module->body);

  return none_value;
}

std::shared_ptr<DataValue>
ProgramState::run_function(OperationFunctionDefinition *function) {
  run_sequence(function->body);

  auto result = return_value;
  return_value = nullptr;

  if (result == nullptr)
    return none_value;

  return result;
}

std::shared_ptr<DataValue> ProgramState::make_default_value(Token *data_type) {
  auto type_name = data_type->get_text(data);

  if (type_name == "bool")
    return std::make_shared<DataValueBool>(false);
  else if (type_name == "uint8")
    return std::make_shared<DataValueUint8>(0);
  else if (type_name == "uint16")
    return std::make_shared<DataValueUint16>(0);
  else if (type_name == "uint32")
    return std::make_shared<DataValueUint32>(0);
  else if (type_name == "uint64")
    return std::make_shared<DataValueUint64>(0);
  else if (type_name == "utf8")
    return std::make_shared<DataValueUtf8>("");
  else
    return none_value;
}

void ProgramState::add_variable(std::string name,
                                std::shared_ptr<DataValue> value) {
  variables.push_back(new Variable(name, value));
}

std::shared_ptr<DataValue>
ProgramState::run_variable_definition(OperationVariableDefinition *operation) {
  auto variable_name = operation->name->get_text(data);

  if (operation->value != nullptr) {
    auto value = run_operation(operation->value);
    add_variable(variable_name, value);
  } else {
    auto value = make_default_value(operation->data_type);
    add_variable(variable_name, value);
  }

  return none_value;
}

std::shared_ptr<DataValue>
ProgramState::run_variable_assignment(OperationVariableAssignment *operation) {
  auto variable_name = operation->name->get_text(data);

  auto value = run_operation(operation->value);

  for (size_t i = 0; variables[i] != NULL; i++) {
    Variable *variable = variables[i];

    if (variable->name == variable_name) {
      variable->value = value;
      break;
    }
  }

  return none_value;
}

std::shared_ptr<DataValue> ProgramState::run_if(OperationIf *operation) {
  auto value = run_operation(operation->condition);
  auto bool_value = std::dynamic_pointer_cast<DataValueBool>(value);
  if (bool_value == nullptr)
    return none_value;
  bool condition = bool_value->value;

  if (condition) {
    run_sequence(operation->body);
  } else if (operation->else_operation != NULL) {
    run_sequence(operation->else_operation->body);
  }

  return none_value;
}

std::shared_ptr<DataValue> ProgramState::run_while(OperationWhile *operation) {
  while (true) {
    auto value = run_operation(operation->condition);
    auto bool_value = std::dynamic_pointer_cast<DataValueBool>(value);
    if (bool_value == nullptr)
      return none_value;

    if (!bool_value->value)
      return none_value;

    run_sequence(operation->body);
  }
}

std::shared_ptr<DataValue>
ProgramState::run_function_call(OperationFunctionCall *operation) {
  if (operation->function != NULL) {
    // FIXME: Use a stack, these variables shouldn't remain after the call
    for (auto i = operation->parameters.begin();
         i != operation->parameters.end(); i++) {
      auto value = run_operation(*i);
      OperationVariableDefinition *parameter_definition =
          (OperationVariableDefinition *)operation->function
              ->parameters[i - operation->parameters.begin()];
      auto variable_name = parameter_definition->name->get_text(data);
      add_variable(variable_name, value);
    }

    return run_function(operation->function);
  }

  std::string function_name = operation->name->get_text(data);

  if (function_name == "print") {
    auto value = run_operation(operation->parameters[0]);
    value->print();
  }

  return none_value;
}

std::shared_ptr<DataValue>
ProgramState::run_return(OperationReturn *operation) {
  auto value = run_operation(operation->value);
  return_value = value;
  return value;
}

std::shared_ptr<DataValue>
ProgramState::run_assert(OperationAssert *operation) {
  auto value = run_operation(operation->expression);
  auto bool_value = std::dynamic_pointer_cast<DataValueBool>(value);
  if (bool_value == nullptr || !bool_value->value)
    failed_assertion = operation;
  return value;
}

std::shared_ptr<DataValue>
ProgramState::run_boolean_constant(OperationBooleanConstant *operation) {
  bool value = operation->value->parse_boolean_constant(data);
  return std::make_shared<DataValueBool>(value);
}

std::shared_ptr<DataValue>
ProgramState::run_number_constant(OperationNumberConstant *operation) {
  uint64_t value = operation->value->parse_number_constant(data);
  // FIXME: Catch overflow (numbers > 64 bit not supported)

  if (value <= UINT8_MAX)
    return std::make_shared<DataValueUint8>(value);
  else if (value <= UINT16_MAX)
    return std::make_shared<DataValueUint16>(value);
  else if (value <= UINT32_MAX)
    return std::make_shared<DataValueUint32>(value);
  else
    return std::make_shared<DataValueUint64>(value);
}

std::shared_ptr<DataValue>
ProgramState::run_text_constant(OperationTextConstant *operation) {
  auto value = operation->value->parse_text_constant(data);
  return std::make_shared<DataValueUtf8>(value);
}

std::shared_ptr<DataValue>
ProgramState::run_variable_value(OperationVariableValue *operation) {
  auto variable_name = operation->name->get_text(data);

  for (auto i = variables.begin(); i != variables.end(); i++) {
    auto variable = *i;
    if (variable->name == variable_name)
      return variable->value;
  }

  printf("variable value %s\n", variable_name.c_str());
  return none_value;
}

std::shared_ptr<DataValue>
ProgramState::run_member_value(OperationMemberValue *operation) {
  std::shared_ptr<DataValue> object = run_operation(operation->object);

  std::shared_ptr<DataValue> result = NULL;
  auto utf8_value = std::dynamic_pointer_cast<DataValueUtf8>(object);
  if (utf8_value != nullptr) {
    if (operation->member->has_text(data, ".length"))
      return std::make_shared<DataValueUint8>(utf8_value->value.size());
    else if (operation->member->has_text(data, ".upper"))
      return std::make_shared<DataValueUtf8>("FOO"); // FIXME: Total hack
    else if (operation->member->has_text(data, ".lower"))
      return std::make_shared<DataValueUtf8>("foo"); // FIXME: Total hack
  }

  return none_value;
}

std::shared_ptr<DataValue>
ProgramState::run_binary_boolean(OperationBinary *operation,
                                 std::shared_ptr<DataValueBool> a,
                                 std::shared_ptr<DataValueBool> b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return std::make_shared<DataValueBool>(a->value == b->value);
  case TOKEN_TYPE_NOT_EQUAL:
    return std::make_shared<DataValueBool>(a->value != b->value);
  default:
    return none_value;
  }
}

std::shared_ptr<DataValue>
ProgramState::run_binary_integer(OperationBinary *operation,
                                 std::shared_ptr<DataValueUint8> a,
                                 std::shared_ptr<DataValueUint8> b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return std::make_shared<DataValueBool>(a->value == b->value);
  case TOKEN_TYPE_NOT_EQUAL:
    return std::make_shared<DataValueBool>(a->value != b->value);
  case TOKEN_TYPE_GREATER:
    return std::make_shared<DataValueBool>(a->value > b->value);
  case TOKEN_TYPE_GREATER_EQUAL:
    return std::make_shared<DataValueBool>(a->value >= b->value);
  case TOKEN_TYPE_LESS:
    return std::make_shared<DataValueBool>(a->value < b->value);
  case TOKEN_TYPE_LESS_EQUAL:
    return std::make_shared<DataValueBool>(a->value <= b->value);
  case TOKEN_TYPE_ADD:
    return std::make_shared<DataValueUint8>(a->value + b->value);
  case TOKEN_TYPE_SUBTRACT:
    return std::make_shared<DataValueUint8>(a->value - b->value);
  case TOKEN_TYPE_MULTIPLY:
    return std::make_shared<DataValueUint8>(a->value * b->value);
  case TOKEN_TYPE_DIVIDE:
    return std::make_shared<DataValueUint8>(a->value / b->value);
  default:
    return none_value;
  }
}

std::shared_ptr<DataValue>
ProgramState::run_binary_text(OperationBinary *operation,
                              std::shared_ptr<DataValueUtf8> a,
                              std::shared_ptr<DataValueUtf8> b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return std::make_shared<DataValueBool>(a->value == b->value);
  case TOKEN_TYPE_NOT_EQUAL:
    return std::make_shared<DataValueBool>(a->value != b->value);
  case TOKEN_TYPE_ADD:
    return std::make_shared<DataValueUtf8>(a->value + b->value);
  default:
    return none_value;
  }
}

std::shared_ptr<DataValue>
ProgramState::run_binary(OperationBinary *operation) {
  std::shared_ptr<DataValue> a = run_operation(operation->a);
  std::shared_ptr<DataValue> b = run_operation(operation->b);

  // FIXME: Support string multiply "*" * 5 == "*****"
  // FIXME: Have compile tell us the data types in advance
  auto bool_a = std::dynamic_pointer_cast<DataValueBool>(a);
  auto bool_b = std::dynamic_pointer_cast<DataValueBool>(b);
  if (bool_a != nullptr && bool_b != nullptr)
    return run_binary_boolean(operation, bool_a, bool_b);
  auto uint8_a = std::dynamic_pointer_cast<DataValueUint8>(a);
  auto uint8_b = std::dynamic_pointer_cast<DataValueUint8>(b);
  if (uint8_a != nullptr && uint8_b != nullptr)
    return run_binary_integer(operation, uint8_a, uint8_b);
  auto utf8_a = std::dynamic_pointer_cast<DataValueUtf8>(a);
  auto utf8_b = std::dynamic_pointer_cast<DataValueUtf8>(b);
  if (utf8_a != nullptr && utf8_b != nullptr)
    return run_binary_text(operation, utf8_a, utf8_b);

  return none_value;
}

std::shared_ptr<DataValue> ProgramState::run_operation(Operation *operation) {
  OperationModule *op_module = dynamic_cast<OperationModule *>(operation);
  if (op_module != nullptr)
    return run_module(op_module);

  OperationVariableDefinition *op_variable_definition =
      dynamic_cast<OperationVariableDefinition *>(operation);
  if (op_variable_definition != nullptr)
    return run_variable_definition(op_variable_definition);

  OperationVariableAssignment *op_variable_assignment =
      dynamic_cast<OperationVariableAssignment *>(operation);
  if (op_variable_assignment != nullptr)
    return run_variable_assignment(op_variable_assignment);

  OperationIf *op_if = dynamic_cast<OperationIf *>(operation);
  if (op_if != nullptr)
    return run_if(op_if);

  OperationElse *op_else = dynamic_cast<OperationElse *>(operation);
  if (op_else != nullptr)
    return none_value; // Resolved in IF

  OperationWhile *op_while = dynamic_cast<OperationWhile *>(operation);
  if (op_while != nullptr)
    return run_while(op_while);

  OperationFunctionDefinition *op_function_definition =
      dynamic_cast<OperationFunctionDefinition *>(operation);
  if (op_function_definition != nullptr)
    return none_value; // Resolved at compile time

  OperationFunctionCall *op_function_call =
      dynamic_cast<OperationFunctionCall *>(operation);
  if (op_function_call != nullptr)
    return run_function_call(op_function_call);

  OperationReturn *op_return = dynamic_cast<OperationReturn *>(operation);
  if (op_return != nullptr)
    return run_return(op_return);

  OperationAssert *op_assert = dynamic_cast<OperationAssert *>(operation);
  if (op_assert != nullptr)
    return run_assert(op_assert);

  OperationBooleanConstant *op_boolean_constant =
      dynamic_cast<OperationBooleanConstant *>(operation);
  if (op_boolean_constant != nullptr)
    return run_boolean_constant(op_boolean_constant);

  OperationNumberConstant *op_number_constant =
      dynamic_cast<OperationNumberConstant *>(operation);
  if (op_number_constant != nullptr)
    return run_number_constant(op_number_constant);

  OperationTextConstant *op_text_constant =
      dynamic_cast<OperationTextConstant *>(operation);
  if (op_text_constant != nullptr)
    return run_text_constant(op_text_constant);

  OperationVariableValue *op_variable_value =
      dynamic_cast<OperationVariableValue *>(operation);
  if (op_variable_value != nullptr)
    return run_variable_value(op_variable_value);

  OperationMemberValue *op_member_value =
      dynamic_cast<OperationMemberValue *>(operation);
  if (op_member_value != nullptr)
    return run_member_value(op_member_value);

  OperationBinary *op_binary = dynamic_cast<OperationBinary *>(operation);
  if (op_binary != nullptr)
    return run_binary(op_binary);

  return none_value;
}

void elf_run(const char *data, OperationModule *module) {
  ProgramState state(data);

  state.run_module(module);
}
