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
#include <stdio.h>

struct DataValue {
  int ref_count;

  DataValue() : ref_count(1) {}
  virtual ~DataValue() {}
  virtual void print() = 0;

  DataValue *ref() {
    ref_count++;
    return this;
  }

  void unref() {
    ref_count--;
    if (ref_count == 0)
      delete this;
  }
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
  DataValue *value;

  Variable(std::string name, DataValue *value)
      : name(name), value(value->ref()) {}

  ~Variable() { value->unref(); }
};

struct ProgramState {
  const char *data;

  DataValueNone none_value;

  std::vector<Variable *> variables;

  DataValue *return_value;

  OperationAssert *failed_assertion;

  ProgramState(const char *data)
      : data(data), return_value(nullptr), failed_assertion(nullptr) {}

  ~ProgramState() {
    for (auto i = variables.begin(); i != variables.end(); i++)
      delete *i;
  }

  void run_sequence(std::vector<Operation *> &body);
  DataValue *run_module(OperationModule *module);
  DataValue *run_function(OperationFunctionDefinition *function);
  DataValue *make_default_value(Token *data_type);
  void add_variable(std::string name, DataValue *value);
  DataValue *run_variable_definition(OperationVariableDefinition *operation);
  DataValue *run_variable_assignment(OperationVariableAssignment *operation);
  DataValue *run_if(OperationIf *operation);
  DataValue *run_while(OperationWhile *operation);
  DataValue *run_function_call(OperationFunctionCall *operation);
  DataValue *run_return(OperationReturn *operation);
  DataValue *run_assert(OperationAssert *operation);
  DataValue *run_boolean_constant(OperationBooleanConstant *operation);
  DataValue *run_number_constant(OperationNumberConstant *operation);
  DataValue *run_text_constant(OperationTextConstant *operation);
  DataValue *run_variable_value(OperationVariableValue *operation);
  DataValue *run_member_value(OperationMemberValue *operation);
  DataValue *run_binary_boolean(OperationBinary *operation, DataValueBool *a,
                                DataValueBool *b);
  DataValue *run_binary_integer(OperationBinary *operation, DataValueUint8 *a,
                                DataValueUint8 *b);
  DataValue *run_binary_text(OperationBinary *operation, DataValueUtf8 *a,
                             DataValueUtf8 *b);
  DataValue *run_binary(OperationBinary *operation);
  DataValue *run_operation(Operation *operation);
};

void ProgramState::run_sequence(std::vector<Operation *> &body) {
  for (auto i = body.begin();
       i != body.end() && failed_assertion == NULL && return_value == NULL;
       i++) {
    DataValue *value = run_operation(*i);
    value->unref();
  }
}

DataValue *ProgramState::run_module(OperationModule *module) {
  run_sequence(module->body);

  return none_value.ref();
}

DataValue *ProgramState::run_function(OperationFunctionDefinition *function) {
  run_sequence(function->body);

  auto result = return_value;
  return_value = nullptr;

  if (result == nullptr)
    return none_value.ref();

  return result;
}

DataValue *ProgramState::make_default_value(Token *data_type) {
  auto type_name = data_type->get_text(data);

  DataValue *result = NULL;
  if (type_name == "bool")
    result = new DataValueBool(false);
  else if (type_name == "uint8")
    result = new DataValueUint8(0);
  else if (type_name == "uint16")
    result = new DataValueUint16(0);
  else if (type_name == "uint32")
    result = new DataValueUint32(0);
  else if (type_name == "uint64")
    result = new DataValueUint64(0);
  else if (type_name == "utf8")
    result = new DataValueUtf8("");

  if (result == NULL)
    printf("default value (%s)\n", type_name.c_str());

  return result;
}

void ProgramState::add_variable(std::string name, DataValue *value) {
  variables.push_back(new Variable(name, value));
}

DataValue *
ProgramState::run_variable_definition(OperationVariableDefinition *operation) {
  auto variable_name = operation->name->get_text(data);

  DataValue *value = NULL;
  if (operation->value != NULL)
    value = run_operation(operation->value);
  else
    value = make_default_value(operation->data_type);

  if (value != NULL)
    add_variable(variable_name, value);

  value->unref();

  return none_value.ref();
}

DataValue *
ProgramState::run_variable_assignment(OperationVariableAssignment *operation) {
  auto variable_name = operation->name->get_text(data);

  DataValue *value = run_operation(operation->value);

  for (size_t i = 0; variables[i] != NULL; i++) {
    Variable *variable = variables[i];
    if (variable->name == variable_name) {
      variable->value->unref();
      variable->value = value->ref();
    }
  }

  return none_value.ref();
}

DataValue *ProgramState::run_if(OperationIf *operation) {
  DataValue *value = run_operation(operation->condition);
  auto bool_value = dynamic_cast<DataValueBool *>(value);
  if (bool_value == nullptr) {
    value->unref();
    return none_value.ref();
  }
  bool condition = bool_value->value;
  value->unref();

  if (condition) {
    run_sequence(operation->body);
  } else if (operation->else_operation != NULL) {
    run_sequence(operation->else_operation->body);
  }

  return none_value.ref();
}

DataValue *ProgramState::run_while(OperationWhile *operation) {
  while (true) {
    DataValue *value = run_operation(operation->condition);
    auto bool_value = dynamic_cast<DataValueBool *>(value);
    if (bool_value == nullptr) {
      value->unref();
      return none_value.ref();
    }
    bool condition = bool_value->value;
    value->unref();

    if (!condition)
      return none_value.ref();

    run_sequence(operation->body);
  }
}

DataValue *ProgramState::run_function_call(OperationFunctionCall *operation) {
  if (operation->function != NULL) {
    // FIXME: Use a stack, these variables shouldn't remain after the call
    for (auto i = operation->parameters.begin();
         i != operation->parameters.end(); i++) {
      DataValue *value = run_operation(*i);
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
    DataValue *value = run_operation(operation->parameters[0]);
    value->print();
    value->unref();
  }

  return none_value.ref();
}

DataValue *ProgramState::run_return(OperationReturn *operation) {
  DataValue *value = run_operation(operation->value);
  return_value = value->ref();
  return value;
}

DataValue *ProgramState::run_assert(OperationAssert *operation) {
  DataValue *value = run_operation(operation->expression);
  auto bool_value = dynamic_cast<DataValueBool *>(value);
  if (bool_value == nullptr || !bool_value->value)
    failed_assertion = operation;
  return value;
}

DataValue *
ProgramState::run_boolean_constant(OperationBooleanConstant *operation) {
  bool value = operation->value->parse_boolean_constant(data);
  return new DataValueBool(value);
}

DataValue *
ProgramState::run_number_constant(OperationNumberConstant *operation) {
  uint64_t value = operation->value->parse_number_constant(data);
  // FIXME: Catch overflow (numbers > 64 bit not supported)

  if (value <= UINT8_MAX)
    return new DataValueUint8(value);
  else if (value <= UINT16_MAX)
    return new DataValueUint16(value);
  else if (value <= UINT32_MAX)
    return new DataValueUint32(value);
  else
    return new DataValueUint64(value);
}

DataValue *ProgramState::run_text_constant(OperationTextConstant *operation) {
  auto value = operation->value->parse_text_constant(data);
  DataValue *result = new DataValueUtf8(value);
  return result;
}

DataValue *ProgramState::run_variable_value(OperationVariableValue *operation) {
  auto variable_name = operation->name->get_text(data);

  for (auto i = variables.begin(); i != variables.end(); i++) {
    auto variable = *i;
    if (variable->name == variable_name)
      return variable->value->ref();
  }

  printf("variable value %s\n", variable_name.c_str());
  return none_value.ref();
}

DataValue *ProgramState::run_member_value(OperationMemberValue *operation) {
  DataValue *object = run_operation(operation->object);

  DataValue *result = NULL;
  auto utf8_value = dynamic_cast<DataValueUtf8 *>(object);
  if (utf8_value != nullptr) {
    if (operation->member->has_text(data, ".length"))
      result = new DataValueUint8(utf8_value->value.size());
    else if (operation->member->has_text(data, ".upper"))
      result = new DataValueUtf8("FOO"); // FIXME: Total hack
    else if (operation->member->has_text(data, ".lower"))
      result = new DataValueUtf8("foo"); // FIXME: Total hack
  }

  object->unref();

  return result;
}

DataValue *ProgramState::run_binary_boolean(OperationBinary *operation,
                                            DataValueBool *a,
                                            DataValueBool *b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return new DataValueBool(a->value == b->value);
  case TOKEN_TYPE_NOT_EQUAL:
    return new DataValueBool(a->value != b->value);
  default:
    return none_value.ref();
  }
}

DataValue *ProgramState::run_binary_integer(OperationBinary *operation,
                                            DataValueUint8 *a,
                                            DataValueUint8 *b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return new DataValueBool(a->value == b->value);
  case TOKEN_TYPE_NOT_EQUAL:
    return new DataValueBool(a->value != b->value);
  case TOKEN_TYPE_GREATER:
    return new DataValueBool(a->value > b->value);
  case TOKEN_TYPE_GREATER_EQUAL:
    return new DataValueBool(a->value >= b->value);
  case TOKEN_TYPE_LESS:
    return new DataValueBool(a->value < b->value);
  case TOKEN_TYPE_LESS_EQUAL:
    return new DataValueBool(a->value <= b->value);
  case TOKEN_TYPE_ADD:
    return new DataValueUint8(a->value + b->value);
  case TOKEN_TYPE_SUBTRACT:
    return new DataValueUint8(a->value - b->value);
  case TOKEN_TYPE_MULTIPLY:
    return new DataValueUint8(a->value * b->value);
  case TOKEN_TYPE_DIVIDE:
    return new DataValueUint8(a->value / b->value);
  default:
    return none_value.ref();
  }
}

DataValue *ProgramState::run_binary_text(OperationBinary *operation,
                                         DataValueUtf8 *a, DataValueUtf8 *b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return new DataValueBool(a->value == b->value);
  case TOKEN_TYPE_NOT_EQUAL:
    return new DataValueBool(a->value != b->value);
  case TOKEN_TYPE_ADD:
    return new DataValueUtf8(a->value + b->value);
  default:
    return none_value.ref();
  }
}

DataValue *ProgramState::run_binary(OperationBinary *operation) {
  DataValue *a = run_operation(operation->a);
  DataValue *b = run_operation(operation->b);

  // FIXME: Support string multiply "*" * 5 == "*****"
  // FIXME: Have compile tell us the data types in advance
  DataValue *result;
  auto bool_a = dynamic_cast<DataValueBool *>(a);
  auto bool_b = dynamic_cast<DataValueBool *>(b);
  if (bool_a != nullptr && bool_b != nullptr)
    result = run_binary_boolean(operation, bool_a, bool_b);
  auto uint8_a = dynamic_cast<DataValueUint8 *>(a);
  auto uint8_b = dynamic_cast<DataValueUint8 *>(b);
  if (uint8_a != nullptr && uint8_b != nullptr)
    result = run_binary_integer(operation, uint8_a, uint8_b);
  auto utf8_a = dynamic_cast<DataValueUtf8 *>(a);
  auto utf8_b = dynamic_cast<DataValueUtf8 *>(b);
  if (utf8_a != nullptr && utf8_b != nullptr)
    result = run_binary_text(operation, utf8_a, utf8_b);

  a->unref();
  b->unref();

  return result;
}

DataValue *ProgramState::run_operation(Operation *operation) {
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
    return none_value.ref(); // Resolved in IF

  OperationWhile *op_while = dynamic_cast<OperationWhile *>(operation);
  if (op_while != nullptr)
    return run_while(op_while);

  OperationFunctionDefinition *op_function_definition =
      dynamic_cast<OperationFunctionDefinition *>(operation);
  if (op_function_definition != nullptr)
    return none_value.ref(); // Resolved at compile time

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

  return none_value.ref();
}

void elf_run(const char *data, OperationModule *module) {
  ProgramState state(data);

  auto result = state.run_module(module);
  result->unref();
}
