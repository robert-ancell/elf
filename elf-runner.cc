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
  virtual std::shared_ptr<DataValue> convert_to(const std::string &data_type);
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
  std::shared_ptr<DataValue> convert_to(const std::string &data_type);
  void print() { printf("%u\n", value); }
};

struct DataValueInt8 : DataValue {
  int8_t value;

  DataValueInt8(int8_t value) : value(value) {}
  std::shared_ptr<DataValue> convert_to(const std::string &data_type);
  void print() { printf("%d\n", value); }
};

struct DataValueUint16 : DataValue {
  uint16_t value;

  DataValueUint16(uint16_t value) : value(value) {}
  std::shared_ptr<DataValue> convert_to(const std::string &data_type);
  void print() { printf("%u\n", value); }
};

struct DataValueInt16 : DataValue {
  int16_t value;

  DataValueInt16(int16_t value) : value(value) {}
  std::shared_ptr<DataValue> convert_to(const std::string &data_type);
  void print() { printf("%d\n", value); }
};

struct DataValueUint32 : DataValue {
  uint32_t value;

  DataValueUint32(uint32_t value) : value(value) {}
  std::shared_ptr<DataValue> convert_to(const std::string &data_type);
  void print() { printf("%u\n", value); }
};

struct DataValueInt32 : DataValue {
  int32_t value;

  DataValueInt32(int32_t value) : value(value) {}
  std::shared_ptr<DataValue> convert_to(const std::string &data_type);
  void print() { printf("%d\n", value); }
};

struct DataValueUint64 : DataValue {
  uint64_t value;

  DataValueUint64(uint64_t value) : value(value) {}
  void print() { printf("%lu\n", value); }
};

struct DataValueInt64 : DataValue {
  int64_t value;

  DataValueInt64(int64_t value) : value(value) {}
  void print() { printf("%li\n", value); }
};

struct DataValueUtf8 : DataValue {
  std::string value;

  DataValueUtf8(std::string value) : value(value) {}
  void print() { printf("%s\n", value.c_str()); }
};

struct DataValueObject : DataValue {
  std::vector<std::shared_ptr<DataValue>> values;

  DataValueObject() {}
  void print() { printf("[FIXME]\n"); }
};

struct DataValueFunction : DataValue {
  std::shared_ptr<OperationFunctionDefinition> function;

  DataValueFunction(std::shared_ptr<OperationFunctionDefinition> &function)
      : function(function) {}
  void print() { printf("<method>\n"); }
};

struct DataValuePrintFunction : DataValue {
  DataValuePrintFunction() {}
  void print() { printf("<print>\n"); }
};

static std::shared_ptr<DataValue>
make_unsigned_integer_value(const std::string &data_type, uint64_t value) {
  if (data_type == "uint8")
    return std::make_shared<DataValueUint8>(value);
  else if (data_type == "uint16")
    return std::make_shared<DataValueUint16>(value);
  else if (data_type == "uint32")
    return std::make_shared<DataValueUint32>(value);
  else if (data_type == "uint64")
    return std::make_shared<DataValueUint64>(value);
  else
    return std::make_shared<DataValueNone>();
}

static std::shared_ptr<DataValue>
make_signed_integer_value(const std::string &data_type, int64_t value) {
  if (data_type == "int8")
    return std::make_shared<DataValueInt8>(value);
  else if (data_type == "int16")
    return std::make_shared<DataValueInt16>(value);
  else if (data_type == "int32")
    return std::make_shared<DataValueInt32>(value);
  else if (data_type == "int64")
    return std::make_shared<DataValueInt64>(value);
  else
    return std::make_shared<DataValueNone>();
}

static std::shared_ptr<DataValue>
make_default_value(const std::string &data_type) {
  if (data_type == "bool")
    return std::make_shared<DataValueBool>(false);
  else if (data_type == "uint8" || data_type == "uint16" ||
           data_type == "uint32" || data_type == "uint64")
    return make_unsigned_integer_value(data_type, 0);
  else if (data_type == "int8" || data_type == "int16" ||
           data_type == "int32" || data_type == "int64")
    return make_signed_integer_value(data_type, 0);
  else if (data_type == "utf8")
    return std::make_shared<DataValueUtf8>("");
  else
    return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue> DataValue::convert_to(const std::string &data_type) {
  return std::make_shared<DataValueNone>();
};

std::shared_ptr<DataValue>
DataValueUint8::convert_to(const std::string &data_type) {
  if (data_type == "uint16" || data_type == "uint32" || data_type == "uint64")
    return make_unsigned_integer_value(data_type, value);
  else if (data_type == "int16" || data_type == "int32" || data_type == "int64")
    return make_signed_integer_value(data_type, value);
  else
    return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
DataValueInt8::convert_to(const std::string &data_type) {
  if (data_type == "int16" || data_type == "int32" || data_type == "int64")
    return make_signed_integer_value(data_type, value);
  else
    return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
DataValueUint16::convert_to(const std::string &data_type) {
  if (data_type == "uint32" || data_type == "uint64")
    return make_unsigned_integer_value(data_type, value);
  else if (data_type == "int32" || data_type == "int64")
    return make_signed_integer_value(data_type, value);
  else
    return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
DataValueInt16::convert_to(const std::string &data_type) {
  if (data_type == "int32" || data_type == "int64")
    return make_signed_integer_value(data_type, value);
  else
    return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
DataValueUint32::convert_to(const std::string &data_type) {
  if (data_type == "uint64")
    return make_unsigned_integer_value(data_type, value);
  else if (data_type == "int64")
    return make_signed_integer_value(data_type, value);
  else
    return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
DataValueInt32::convert_to(const std::string &data_type) {
  if (data_type == "int64")
    return make_signed_integer_value(data_type, value);
  else
    return std::make_shared<DataValueNone>();
}

struct Variable {
  std::string name;
  std::shared_ptr<DataValue> value;

  Variable(std::string name, std::shared_ptr<DataValue> value)
      : name(name), value(value) {}
};

struct ProgramState {
  const char *data;

  std::vector<Variable *> variables;

  std::shared_ptr<DataValue> return_value;

  std::shared_ptr<OperationAssert> failed_assertion;

  ProgramState(const char *data)
      : data(data), return_value(nullptr), failed_assertion(nullptr) {}

  ~ProgramState() {
    for (auto i = variables.begin(); i != variables.end(); i++)
      delete *i;
  }

  void run_sequence(std::vector<std::shared_ptr<Operation>> &body);
  std::shared_ptr<DataValue>
  run_module(std::shared_ptr<OperationModule> &module);
  std::shared_ptr<DataValue>
  run_function(std::shared_ptr<OperationFunctionDefinition> &function);
  void add_variable(std::string name, std::shared_ptr<DataValue> value);
  std::shared_ptr<DataValue> run_variable_definition(
      std::shared_ptr<OperationVariableDefinition> &operation);
  std::shared_ptr<DataValue>
  run_assignment(std::shared_ptr<OperationAssignment> &operation);
  std::shared_ptr<DataValue> run_if(std::shared_ptr<OperationIf> &operation);
  std::shared_ptr<DataValue>
  run_while(std::shared_ptr<OperationWhile> &operation);
  std::shared_ptr<DataValue>
  run_symbol(std::shared_ptr<OperationSymbol> &operation);
  std::shared_ptr<DataValue>
  run_call(std::shared_ptr<OperationCall> &operation);
  std::shared_ptr<DataValue>
  run_return(std::shared_ptr<OperationReturn> &operation);
  std::shared_ptr<DataValue>
  run_assert(std::shared_ptr<OperationAssert> &operation);
  std::shared_ptr<DataValue>
  run_true(std::shared_ptr<OperationTrue> &operation);
  std::shared_ptr<DataValue>
  run_false(std::shared_ptr<OperationFalse> &operation);
  std::shared_ptr<DataValue>
  run_number_constant(std::shared_ptr<OperationNumberConstant> &operation);
  std::shared_ptr<DataValue>
  run_text_constant(std::shared_ptr<OperationTextConstant> &operation);
  std::shared_ptr<DataValue>
  run_member(std::shared_ptr<OperationMember> &operation);
  std::shared_ptr<DataValue>
  run_binary_boolean(std::shared_ptr<OperationBinary> &operation,
                     std::shared_ptr<DataValueBool> &a,
                     std::shared_ptr<DataValueBool> &b);
  std::shared_ptr<DataValue>
  run_binary_integer(std::shared_ptr<OperationBinary> &operation,
                     std::shared_ptr<DataValueUint8> &a,
                     std::shared_ptr<DataValueUint8> &b);
  std::shared_ptr<DataValue>
  run_binary_text(std::shared_ptr<OperationBinary> &operation,
                  std::shared_ptr<DataValueUtf8> &a,
                  std::shared_ptr<DataValueUtf8> &b);
  std::shared_ptr<DataValue>
  run_binary(std::shared_ptr<OperationBinary> &operation);
  std::shared_ptr<DataValue>
  run_convert(std::shared_ptr<OperationConvert> &operation);
  std::shared_ptr<DataValue>
  run_operation(std::shared_ptr<Operation> &operation);
};

void ProgramState::run_sequence(std::vector<std::shared_ptr<Operation>> &body) {
  for (auto i = body.begin();
       i != body.end() && failed_assertion == NULL && return_value == NULL;
       i++) {
    run_operation(*i);
  }
}

std::shared_ptr<DataValue>
ProgramState::run_module(std::shared_ptr<OperationModule> &module) {
  run_sequence(module->children);
  return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue> ProgramState::run_function(
    std::shared_ptr<OperationFunctionDefinition> &function) {
  run_sequence(function->children);

  auto result = return_value;
  return_value = nullptr;

  if (result == nullptr)
    return std::make_shared<DataValueNone>();

  return result;
}

void ProgramState::add_variable(std::string name,
                                std::shared_ptr<DataValue> value) {
  variables.push_back(new Variable(name, value));
}

std::shared_ptr<DataValue> ProgramState::run_variable_definition(
    std::shared_ptr<OperationVariableDefinition> &operation) {
  auto variable_name = operation->name->get_text();

  auto type_definition = std::dynamic_pointer_cast<OperationTypeDefinition>(
      operation->data_type->type_definition);
  if (type_definition != nullptr) {
    auto value = std::make_shared<DataValueObject>();
    for (auto i = type_definition->children.begin();
         i != type_definition->children.end(); i++) {
      auto variable_definition =
          std::dynamic_pointer_cast<OperationVariableDefinition>(*i);
      if (variable_definition == nullptr)
        continue;

      auto v = make_default_value(variable_definition->get_data_type());
      value->values.push_back(v);
    }

    add_variable(variable_name, value);
  } else if (operation->value != nullptr) {
    auto value = run_operation(operation->value);
    add_variable(variable_name, value);
  } else {
    auto value = make_default_value(operation->data_type->get_data_type());
    add_variable(variable_name, value);
  }

  return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
ProgramState::run_assignment(std::shared_ptr<OperationAssignment> &operation) {
  auto target_value = run_operation(operation->target);
  auto value = run_operation(operation->value);

  auto target_value_bool =
      std::dynamic_pointer_cast<DataValueBool>(target_value);
  auto value_bool = std::dynamic_pointer_cast<DataValueBool>(value);
  if (target_value_bool != nullptr && value_bool != nullptr)
    target_value_bool->value = value_bool->value;
  auto target_value_uint8 =
      std::dynamic_pointer_cast<DataValueUint8>(target_value);
  auto value_uint8 = std::dynamic_pointer_cast<DataValueUint8>(value);
  if (target_value_uint8 != nullptr && value_uint8 != nullptr)
    target_value_uint8->value = value_uint8->value;
  auto target_value_utf8 =
      std::dynamic_pointer_cast<DataValueUtf8>(target_value);
  auto value_utf8 = std::dynamic_pointer_cast<DataValueUtf8>(value);
  if (target_value_utf8 != nullptr && value_utf8 != nullptr)
    target_value_utf8->value = value_utf8->value;

  return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
ProgramState::run_if(std::shared_ptr<OperationIf> &operation) {
  auto value = run_operation(operation->condition);
  auto bool_value = std::dynamic_pointer_cast<DataValueBool>(value);
  if (bool_value == nullptr)
    return std::make_shared<DataValueNone>();
  bool condition = bool_value->value;

  if (condition) {
    run_sequence(operation->children);
  } else if (operation->else_operation != NULL) {
    run_sequence(operation->else_operation->children);
  }

  return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
ProgramState::run_while(std::shared_ptr<OperationWhile> &operation) {
  while (true) {
    auto value = run_operation(operation->condition);
    auto bool_value = std::dynamic_pointer_cast<DataValueBool>(value);
    if (bool_value == nullptr)
      return std::make_shared<DataValueNone>();

    if (!bool_value->value)
      return std::make_shared<DataValueNone>();

    run_sequence(operation->children);
  }
}

std::shared_ptr<DataValue>
ProgramState::run_symbol(std::shared_ptr<OperationSymbol> &operation) {
  auto name = operation->name->get_text();

  if (name == "print")
    return std::make_shared<DataValuePrintFunction>();

  auto function_definition =
      std::dynamic_pointer_cast<OperationFunctionDefinition>(
          operation->definition);
  if (function_definition != nullptr)
    return std::make_shared<DataValueFunction>(function_definition);

  for (auto i = variables.begin(); i != variables.end(); i++) {
    auto variable = *i;
    if (variable->name == name)
      return variable->value;
  }

  return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
ProgramState::run_call(std::shared_ptr<OperationCall> &operation) {
  auto value = run_operation(operation->value);

  std::vector<std::shared_ptr<DataValue>> parameter_values;
  for (auto i = operation->parameters.begin(); i != operation->parameters.end();
       i++)
    parameter_values.push_back(run_operation(*i));

  if (std::dynamic_pointer_cast<DataValuePrintFunction>(value) != nullptr) {
    parameter_values[0]->print();
    return std::make_shared<DataValueNone>();
  }

  auto function_value = std::dynamic_pointer_cast<DataValueFunction>(value);
  if (function_value == nullptr)
    return std::make_shared<DataValueNone>();

  // FIXME: Use a stack, these variables shouldn't remain after the call
  for (auto i = parameter_values.begin(); i != parameter_values.end(); i++) {
    auto parameter_definition =
        function_value->function->parameters[i - parameter_values.begin()];
    auto variable_name = parameter_definition->name->get_text();
    add_variable(variable_name, *i);
  }

  return run_function(function_value->function);
}

std::shared_ptr<DataValue>
ProgramState::run_return(std::shared_ptr<OperationReturn> &operation) {
  auto value = run_operation(operation->value);
  return_value = value;
  return value;
}

std::shared_ptr<DataValue>
ProgramState::run_assert(std::shared_ptr<OperationAssert> &operation) {
  auto value = run_operation(operation->expression);
  auto bool_value = std::dynamic_pointer_cast<DataValueBool>(value);
  if (bool_value == nullptr || !bool_value->value)
    failed_assertion = operation;
  return value;
}

std::shared_ptr<DataValue>
ProgramState::run_true(std::shared_ptr<OperationTrue> &operation) {
  return std::make_shared<DataValueBool>(true);
}

std::shared_ptr<DataValue>
ProgramState::run_false(std::shared_ptr<OperationFalse> &operation) {
  return std::make_shared<DataValueBool>(false);
}

std::shared_ptr<DataValue> ProgramState::run_number_constant(
    std::shared_ptr<OperationNumberConstant> &operation) {
  // FIXME: Catch overflow (numbers > 64 bit not supported)

  int64_t sign = operation->sign_token != nullptr ? -1 : 1;

  if (operation->data_type == "uint8")
    return std::make_shared<DataValueUint8>(operation->magnitude);
  else if (operation->data_type == "int8")
    return std::make_shared<DataValueInt8>(sign * operation->magnitude);
  else if (operation->data_type == "uint16")
    return std::make_shared<DataValueUint16>(operation->magnitude);
  else if (operation->data_type == "int16")
    return std::make_shared<DataValueInt16>(sign * operation->magnitude);
  else if (operation->data_type == "uint32")
    return std::make_shared<DataValueUint32>(operation->magnitude);
  else if (operation->data_type == "int32")
    return std::make_shared<DataValueInt32>(sign * operation->magnitude);
  else if (operation->data_type == "uint64")
    return std::make_shared<DataValueUint64>(operation->magnitude);
  else if (operation->data_type == "int64")
    return std::make_shared<DataValueInt64>(sign * operation->magnitude);
  else
    return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue> ProgramState::run_text_constant(
    std::shared_ptr<OperationTextConstant> &operation) {
  return std::make_shared<DataValueUtf8>(operation->value);
}

std::shared_ptr<DataValue>
ProgramState::run_member(std::shared_ptr<OperationMember> &operation) {
  auto value = run_operation(operation->value);

  auto object_value = std::dynamic_pointer_cast<DataValueObject>(value);
  if (object_value == nullptr)
    return std::make_shared<DataValueNone>();

  auto primitive_definition =
      std::dynamic_pointer_cast<OperationPrimitiveDefinition>(
          operation->type_definition);
  if (primitive_definition != nullptr) {
    return std::make_shared<DataValueNone>();
  }

  auto type_definition = std::dynamic_pointer_cast<OperationTypeDefinition>(
      operation->type_definition);
  if (type_definition != nullptr) {
    auto member_name = operation->get_member_name();
    size_t index = 0;
    for (auto i = type_definition->children.begin();
         i != type_definition->children.end(); i++) {
      auto vd = std::dynamic_pointer_cast<OperationVariableDefinition>(*i);
      if (vd == nullptr)
        continue;

      if (vd->name->has_text(member_name))
        return object_value->values[index];

      index++;
      if (index >= object_value->values.size())
        break;
    }
  }

  return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
ProgramState::run_binary_boolean(std::shared_ptr<OperationBinary> &operation,
                                 std::shared_ptr<DataValueBool> &a,
                                 std::shared_ptr<DataValueBool> &b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_WORD:
    if (operation->op->has_text("and"))
      return std::make_shared<DataValueBool>(a->value && b->value);
    else if (operation->op->has_text("or"))
      return std::make_shared<DataValueBool>(a->value || b->value);
    else if (operation->op->has_text("xor"))
      return std::make_shared<DataValueBool>(a->value ^ b->value);
    else
      return std::make_shared<DataValueNone>();
  case TOKEN_TYPE_EQUAL:
    return std::make_shared<DataValueBool>(a->value == b->value);
  case TOKEN_TYPE_NOT_EQUAL:
    return std::make_shared<DataValueBool>(a->value != b->value);
  default:
    return std::make_shared<DataValueNone>();
  }
}

std::shared_ptr<DataValue>
ProgramState::run_binary_integer(std::shared_ptr<OperationBinary> &operation,
                                 std::shared_ptr<DataValueUint8> &a,
                                 std::shared_ptr<DataValueUint8> &b) {
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
    return std::make_shared<DataValueNone>();
  }
}

std::shared_ptr<DataValue>
ProgramState::run_binary_text(std::shared_ptr<OperationBinary> &operation,
                              std::shared_ptr<DataValueUtf8> &a,
                              std::shared_ptr<DataValueUtf8> &b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return std::make_shared<DataValueBool>(a->value == b->value);
  case TOKEN_TYPE_NOT_EQUAL:
    return std::make_shared<DataValueBool>(a->value != b->value);
  case TOKEN_TYPE_ADD:
    return std::make_shared<DataValueUtf8>(a->value + b->value);
  default:
    return std::make_shared<DataValueNone>();
  }
}

std::shared_ptr<DataValue>
ProgramState::run_binary(std::shared_ptr<OperationBinary> &operation) {
  auto a = run_operation(operation->a);
  auto b = run_operation(operation->b);

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

  return std::make_shared<DataValueNone>();
}

std::shared_ptr<DataValue>
ProgramState::run_convert(std::shared_ptr<OperationConvert> &operation) {
  auto value = run_operation(operation->op);
  return value->convert_to(operation->data_type);
}

std::shared_ptr<DataValue>
ProgramState::run_operation(std::shared_ptr<Operation> &operation) {
  auto op_module = std::dynamic_pointer_cast<OperationModule>(operation);
  if (op_module != nullptr)
    return run_module(op_module);

  auto op_variable_definition =
      std::dynamic_pointer_cast<OperationVariableDefinition>(operation);
  if (op_variable_definition != nullptr)
    return run_variable_definition(op_variable_definition);

  auto op_assignment =
      std::dynamic_pointer_cast<OperationAssignment>(operation);
  if (op_assignment != nullptr)
    return run_assignment(op_assignment);

  auto op_if = std::dynamic_pointer_cast<OperationIf>(operation);
  if (op_if != nullptr)
    return run_if(op_if);

  auto op_else = std::dynamic_pointer_cast<OperationElse>(operation);
  if (op_else != nullptr)
    return std::make_shared<DataValueNone>(); // Resolved in IF

  auto op_while = std::dynamic_pointer_cast<OperationWhile>(operation);
  if (op_while != nullptr)
    return run_while(op_while);

  auto op_function_definition =
      std::dynamic_pointer_cast<OperationFunctionDefinition>(operation);
  if (op_function_definition != nullptr)
    return std::make_shared<DataValueNone>(); // Resolved at compile time

  auto op_type_definition =
      std::dynamic_pointer_cast<OperationTypeDefinition>(operation);
  if (op_type_definition != nullptr)
    return std::make_shared<DataValueNone>(); // Resolved at compile time

  auto op_symbol = std::dynamic_pointer_cast<OperationSymbol>(operation);
  if (op_symbol != nullptr)
    return run_symbol(op_symbol);

  auto op_call = std::dynamic_pointer_cast<OperationCall>(operation);
  if (op_call != nullptr)
    return run_call(op_call);

  auto op_return = std::dynamic_pointer_cast<OperationReturn>(operation);
  if (op_return != nullptr)
    return run_return(op_return);

  auto op_assert = std::dynamic_pointer_cast<OperationAssert>(operation);
  if (op_assert != nullptr)
    return run_assert(op_assert);

  auto op_true = std::dynamic_pointer_cast<OperationTrue>(operation);
  if (op_true != nullptr)
    return run_true(op_true);

  auto op_false = std::dynamic_pointer_cast<OperationFalse>(operation);
  if (op_false != nullptr)
    return run_false(op_false);

  auto op_number_constant =
      std::dynamic_pointer_cast<OperationNumberConstant>(operation);
  if (op_number_constant != nullptr)
    return run_number_constant(op_number_constant);

  auto op_text_constant =
      std::dynamic_pointer_cast<OperationTextConstant>(operation);
  if (op_text_constant != nullptr)
    return run_text_constant(op_text_constant);

  auto op_member = std::dynamic_pointer_cast<OperationMember>(operation);
  if (op_member != nullptr)
    return run_member(op_member);

  auto op_binary = std::dynamic_pointer_cast<OperationBinary>(operation);
  if (op_binary != nullptr)
    return run_binary(op_binary);

  auto op_convert = std::dynamic_pointer_cast<OperationConvert>(operation);
  if (op_convert != nullptr)
    return run_convert(op_convert);

  return std::make_shared<DataValueNone>();
}

void elf_run(const char *data, std::shared_ptr<OperationModule> module) {
  ProgramState state(data);

  state.run_module(module);
}
