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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

typedef enum {
  DATA_TYPE_NONE,
  DATA_TYPE_BOOL,
  DATA_TYPE_UINT8,
  DATA_TYPE_INT8,
  DATA_TYPE_UINT16,
  DATA_TYPE_INT16,
  DATA_TYPE_UINT32,
  DATA_TYPE_INT32,
  DATA_TYPE_UINT64,
  DATA_TYPE_INT64,
  DATA_TYPE_UTF8,
} DataType;

struct DataValue {
  int ref_count;
  DataType type;
  uint8_t *data;
  size_t data_length;

  DataValue(DataType type)
      : ref_count(1), type(type), data(nullptr), data_length(0) {}
  ~DataValue() { free(data); }

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

struct Variable {
  std::string name;
  DataValue *value;

  Variable(std::string name, DataValue *value)
      : name(name), value(value->ref()) {}

  ~Variable() { value->unref(); }
};

typedef struct {
  const char *data;

  DataValue *none_value;

  Variable **variables;
  size_t variables_length;

  DataValue *return_value;

  OperationAssert *failed_assertion;
} ProgramState;

static DataValue *run_operation(ProgramState *state, Operation *operation);

static DataValue *data_value_new(DataType type) {
  DataValue *value = new DataValue(type);

  switch (type) {
  case DATA_TYPE_NONE:
    value->data_length = 0;
  case DATA_TYPE_BOOL:
  case DATA_TYPE_UINT8:
  case DATA_TYPE_INT8:
    value->data_length = 1;
    break;
  case DATA_TYPE_UINT16:
  case DATA_TYPE_INT16:
    value->data_length = 2;
    break;
  case DATA_TYPE_UINT32:
  case DATA_TYPE_INT32:
    value->data_length = 4;
    break;
  case DATA_TYPE_UINT64:
  case DATA_TYPE_INT64:
    value->data_length = 8;
    break;
  case DATA_TYPE_UTF8:
    value->data_length = 1;
    break;
  }
  value->data = static_cast<uint8_t *>(malloc(value->data_length));
  memset(value->data, 0, value->data_length);

  return value;
}

static DataValue *data_value_new_bool(bool bool_value) {
  DataValue *value = data_value_new(DATA_TYPE_BOOL);
  value->data[0] = bool_value ? 0xFF : 0x00;
  return value;
}

static DataValue *data_value_new_uint8(uint8_t int_value) {
  DataValue *value = data_value_new(DATA_TYPE_UINT8);
  value->data[0] = int_value;
  return value;
}

static DataValue *data_value_new_uint16(uint16_t int_value) {
  DataValue *value = data_value_new(DATA_TYPE_UINT16);
  value->data[0] = (int_value >> 8) & 0xFF;
  value->data[1] = (int_value >> 0) & 0xFF;
  return value;
}

static DataValue *data_value_new_uint32(uint32_t int_value) {
  DataValue *value = data_value_new(DATA_TYPE_UINT32);
  value->data[0] = (int_value >> 24) & 0xFF;
  value->data[1] = (int_value >> 16) & 0xFF;
  value->data[2] = (int_value >> 8) & 0xFF;
  value->data[3] = (int_value >> 0) & 0xFF;
  return value;
}

static DataValue *data_value_new_uint64(uint64_t int_value) {
  DataValue *value = data_value_new(DATA_TYPE_UINT64);
  value->data[0] = (int_value >> 56) & 0xFF;
  value->data[1] = (int_value >> 48) & 0xFF;
  value->data[2] = (int_value >> 40) & 0xFF;
  value->data[3] = (int_value >> 32) & 0xFF;
  value->data[4] = (int_value >> 24) & 0xFF;
  value->data[5] = (int_value >> 16) & 0xFF;
  value->data[6] = (int_value >> 8) & 0xFF;
  value->data[7] = (int_value >> 0) & 0xFF;
  return value;
}

static DataValue *data_value_new_utf8_sized(size_t length) {
  DataValue *value = new DataValue(DATA_TYPE_UTF8);

  value->data_length = length;
  value->data =
      static_cast<uint8_t *>(malloc(sizeof(char) * value->data_length));
  value->data[0] = '\0';

  return value;
}

static DataValue *data_value_new_utf8(std::string string_value) {
  DataValue *value = data_value_new_utf8_sized(string_value.size() + 1);
  for (size_t i = 0; i < value->data_length; i++)
    value->data[i] = string_value[i];
  value->data[value->data_length] = '\0';

  return value;
}

static DataValue *data_value_new_utf8_join(DataValue *a, DataValue *b) {
  DataValue *value = data_value_new_utf8_sized((a->data_length - 1) +
                                               (b->data_length - 1) + 1);
  value->data =
      static_cast<uint8_t *>(malloc(sizeof(char) * value->data_length));
  for (size_t i = 0; i < a->data_length; i++)
    value->data[i] = a->data[i];
  for (size_t i = 0; i < b->data_length; i++)
    value->data[a->data_length - 1 + i] = b->data[i];
  value->data[value->data_length] = '\0';

  return value;
}

static bool data_value_is_integer(DataValue *value) {
  return value->type == DATA_TYPE_UINT8 || value->type == DATA_TYPE_INT8 ||
         value->type == DATA_TYPE_UINT16 || value->type == DATA_TYPE_INT16 ||
         value->type == DATA_TYPE_UINT32 || value->type == DATA_TYPE_INT32 ||
         value->type == DATA_TYPE_UINT64 || value->type == DATA_TYPE_INT64;
}

static bool data_value_equal(DataValue *a, DataValue *b) {
  if (a->type != b->type)
    return false;

  if (a->data_length != b->data_length)
    return false;

  for (size_t i = 0; i < a->data_length; i++)
    if (a->data[i] != b->data[i])
      return false;

  return true;
}

static void run_sequence(ProgramState *state, Operation **body,
                         size_t body_length) {
  for (size_t i = 0; i < body_length && state->failed_assertion == NULL &&
                     state->return_value == NULL;
       i++) {
    DataValue *value = run_operation(state, body[i]);
    value->unref();
  }
}

static DataValue *run_module(ProgramState *state, OperationModule *module) {
  run_sequence(state, module->body, module->body_length);

  return state->none_value->ref();
}

static DataValue *run_function(ProgramState *state,
                               OperationFunctionDefinition *function) {
  run_sequence(state, function->body, function->body_length);

  DataValue *return_value = state->return_value;
  state->return_value = nullptr;

  if (return_value == nullptr)
    return state->none_value->ref();

  return return_value;
}

static DataValue *make_default_value(ProgramState *state, Token *data_type) {
  auto type_name = data_type->get_text(state->data);

  DataValue *result = NULL;
  if (type_name == "bool")
    result = data_value_new_bool(false);
  else if (type_name == "uint8")
    result = data_value_new_uint8(0);
  else if (type_name == "uint16")
    result = data_value_new_uint16(0);
  else if (type_name == "uint32")
    result = data_value_new_uint32(0);
  else if (type_name == "uint64")
    result = data_value_new_uint64(0);
  else if (type_name == "utf8")
    result = data_value_new_utf8("");

  if (result == NULL)
    printf("default value (%s)\n", type_name.c_str());

  return result;
}

static void add_variable(ProgramState *state, std::string name,
                         DataValue *value) {
  state->variables_length++;
  state->variables = static_cast<Variable **>(
      realloc(state->variables, sizeof(Variable *) * state->variables_length));
  state->variables[state->variables_length - 1] = new Variable(name, value);
}

static DataValue *
run_variable_definition(ProgramState *state,
                        OperationVariableDefinition *operation) {
  auto variable_name = operation->name->get_text(state->data);

  DataValue *value = NULL;
  if (operation->value != NULL)
    value = run_operation(state, operation->value);
  else
    value = make_default_value(state, operation->data_type);

  if (value != NULL)
    add_variable(state, variable_name, value);

  value->unref();

  return state->none_value->ref();
}

static DataValue *
run_variable_assignment(ProgramState *state,
                        OperationVariableAssignment *operation) {
  auto variable_name = operation->name->get_text(state->data);

  DataValue *value = run_operation(state, operation->value);

  for (size_t i = 0; state->variables[i] != NULL; i++) {
    Variable *variable = state->variables[i];
    if (variable->name == variable_name) {
      variable->value->unref();
      variable->value = value->ref();
    }
  }

  return state->none_value->ref();
}

static DataValue *run_if(ProgramState *state, OperationIf *operation) {
  DataValue *value = run_operation(state, operation->condition);
  if (value->type != DATA_TYPE_BOOL) {
    value->unref();
    return state->none_value->ref();
  }
  bool condition = value->data[0] != 0;
  value->unref();

  Operation **body = NULL;
  size_t body_length = 0;
  if (condition) {
    body = operation->body;
    body_length = operation->body_length;
  } else if (operation->else_operation != NULL) {
    body = operation->else_operation->body;
    body_length = operation->else_operation->body_length;
  }

  run_sequence(state, body, body_length);

  return state->none_value->ref();
}

static DataValue *run_while(ProgramState *state, OperationWhile *operation) {
  while (true) {
    DataValue *value = run_operation(state, operation->condition);
    if (value->type != DATA_TYPE_BOOL) {
      value->unref();
      return state->none_value->ref();
    }
    bool condition = value->data[0] != 0;
    value->unref();

    if (!condition)
      return state->none_value->ref();

    run_sequence(state, operation->body, operation->body_length);
  }
}

static DataValue *run_function_call(ProgramState *state,
                                    OperationFunctionCall *operation) {
  if (operation->function != NULL) {
    // FIXME: Use a stack, these variables shouldn't remain after the call
    for (int i = 0; operation->parameters[i] != NULL; i++) {
      DataValue *value = run_operation(state, operation->parameters[i]);
      OperationVariableDefinition *parameter_definition =
          (OperationVariableDefinition *)operation->function->parameters[i];
      auto variable_name = parameter_definition->name->get_text(state->data);
      add_variable(state, variable_name, value);
    }

    return run_function(state, operation->function);
  }

  std::string function_name = operation->name->get_text(state->data);

  if (function_name == "print") {
    DataValue *value = run_operation(state, operation->parameters[0]);

    switch (value->type) {
    case DATA_TYPE_BOOL:
      if (value->data[0] == 0)
        printf("false");
      else
        printf("true");
      break;
    case DATA_TYPE_UINT8:
      printf("%d", value->data[0]);
      break;
    case DATA_TYPE_UINT16:
      printf("%d", value->data[0] << 8 | value->data[1]);
      break;
    case DATA_TYPE_UINT32:
      printf("%d", value->data[3] << 24 | value->data[1] << 16 |
                       value->data[1] << 8 | value->data[0]);
      break;
    case DATA_TYPE_UINT64: {
      uint64_t v =
          (uint64_t)value->data[7] << 56 | (uint64_t)value->data[6] << 48 |
          (uint64_t)value->data[5] << 40 | (uint64_t)value->data[4] << 32 |
          (uint64_t)value->data[3] << 24 | (uint64_t)value->data[1] << 16 |
          (uint64_t)value->data[1] << 8 | (uint64_t)value->data[0];
      printf("%lu", v);
      break;
    }
    case DATA_TYPE_UTF8:
      printf("%.*s", (int)value->data_length, value->data);
      break;
    default:
      for (size_t i = 0; i < value->data_length; i++)
        printf("%02X", value->data[i]);
      break;
    }
    printf("\n");

    value->unref();
  }

  return state->none_value->ref();
}

static DataValue *run_return(ProgramState *state, OperationReturn *operation) {
  DataValue *value = run_operation(state, operation->value);
  state->return_value = value->ref();
  return value;
}

static DataValue *run_assert(ProgramState *state, OperationAssert *operation) {
  DataValue *value = run_operation(state, operation->expression);
  assert(value->type == DATA_TYPE_BOOL);
  if (value->data[0] == 0)
    state->failed_assertion = operation;
  return value;
}

static DataValue *run_boolean_constant(ProgramState *state,
                                       OperationBooleanConstant *operation) {
  bool value = operation->value->parse_boolean_constant(state->data);
  return data_value_new_bool(value);
}

static DataValue *run_number_constant(ProgramState *state,
                                      OperationNumberConstant *operation) {
  uint64_t value = operation->value->parse_number_constant(state->data);
  // FIXME: Catch overflow (numbers > 64 bit not supported)

  if (value <= UINT8_MAX)
    return data_value_new_uint8(value);
  else if (value <= UINT16_MAX)
    return data_value_new_uint16(value);
  else if (value <= UINT32_MAX)
    return data_value_new_uint32(value);
  else
    return data_value_new_uint64(value);
}

static DataValue *run_text_constant(ProgramState *state,
                                    OperationTextConstant *operation) {
  auto value = operation->value->parse_text_constant(state->data);
  DataValue *result = data_value_new_utf8(value);
  return result;
}

static DataValue *run_variable_value(ProgramState *state,
                                     OperationVariableValue *operation) {
  auto variable_name = operation->name->get_text(state->data);

  for (size_t i = 0; i < state->variables_length; i++) {
    Variable *variable = state->variables[i];
    if (variable->name == variable_name)
      return variable->value->ref();
  }

  printf("variable value %s\n", variable_name.c_str());
  return state->none_value->ref();
}

static DataValue *run_member_value(ProgramState *state,
                                   OperationMemberValue *operation) {
  DataValue *object = run_operation(state, operation->object);

  DataValue *result = NULL;
  if (object->type == DATA_TYPE_UTF8) {
    if (operation->member->has_text(state->data, ".length"))
      result =
          data_value_new_uint8(object->data_length - 1); // FIXME: Total hack
    else if (operation->member->has_text(state->data, ".upper"))
      result = data_value_new_utf8("FOO"); // FIXME: Total hack
    else if (operation->member->has_text(state->data, ".lower"))
      result = data_value_new_utf8("foo"); // FIXME: Total hack
  }

  object->unref();

  return result;
}

static DataValue *run_binary_boolean(ProgramState *state,
                                     OperationBinary *operation, DataValue *a,
                                     DataValue *b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return data_value_new_bool(a->data[0] == b->data[0]);
  case TOKEN_TYPE_NOT_EQUAL:
    return data_value_new_bool(a->data[0] != b->data[0]);
  default:
    return state->none_value->ref();
  }
}

static DataValue *run_binary_integer(ProgramState *state,
                                     OperationBinary *operation, DataValue *a,
                                     DataValue *b) {
  if (a->type != DATA_TYPE_UINT8 || b->type != DATA_TYPE_UINT8)
    return state->none_value->ref();

  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return data_value_new_bool(a->data[0] == b->data[0]);
  case TOKEN_TYPE_NOT_EQUAL:
    return data_value_new_bool(a->data[0] != b->data[0]);
  case TOKEN_TYPE_GREATER:
    return data_value_new_bool(a->data[0] > b->data[0]);
  case TOKEN_TYPE_GREATER_EQUAL:
    return data_value_new_bool(a->data[0] >= b->data[0]);
  case TOKEN_TYPE_LESS:
    return data_value_new_bool(a->data[0] < b->data[0]);
  case TOKEN_TYPE_LESS_EQUAL:
    return data_value_new_bool(a->data[0] <= b->data[0]);
  case TOKEN_TYPE_ADD:
    return data_value_new_uint8(a->data[0] + b->data[0]);
  case TOKEN_TYPE_SUBTRACT:
    return data_value_new_uint8(a->data[0] - b->data[0]);
  case TOKEN_TYPE_MULTIPLY:
    return data_value_new_uint8(a->data[0] * b->data[0]);
  case TOKEN_TYPE_DIVIDE:
    return data_value_new_uint8(a->data[0] / b->data[0]);
  default:
    return state->none_value->ref();
  }
}

static DataValue *run_binary_text(ProgramState *state,
                                  OperationBinary *operation, DataValue *a,
                                  DataValue *b) {
  switch (operation->op->type) {
  case TOKEN_TYPE_EQUAL:
    return data_value_new_bool(data_value_equal(a, b));
  case TOKEN_TYPE_NOT_EQUAL:
    return data_value_new_bool(!data_value_equal(a, b));
  case TOKEN_TYPE_ADD:
    return data_value_new_utf8_join(a, b);
  default:
    return state->none_value->ref();
  }
}

static DataValue *run_binary(ProgramState *state, OperationBinary *operation) {
  DataValue *a = run_operation(state, operation->a);
  DataValue *b = run_operation(state, operation->b);

  // FIXME: Support string multiply "*" * 5 == "*****"
  DataValue *result;
  if (a->type == DATA_TYPE_BOOL && b->type == DATA_TYPE_BOOL)
    result = run_binary_boolean(state, operation, a, b);
  else if (data_value_is_integer(a) && data_value_is_integer(b))
    result = run_binary_integer(state, operation, a, b);
  else if (a->type == DATA_TYPE_UTF8 && b->type == DATA_TYPE_UTF8)
    result = run_binary_text(state, operation, a, b);

  a->unref();
  b->unref();

  return result;
}

static DataValue *run_operation(ProgramState *state, Operation *operation) {
  OperationModule *op_module = dynamic_cast<OperationModule *>(operation);
  if (op_module != nullptr)
    return run_module(state, op_module);

  OperationVariableDefinition *op_variable_definition =
      dynamic_cast<OperationVariableDefinition *>(operation);
  if (op_variable_definition != nullptr)
    return run_variable_definition(state, op_variable_definition);

  OperationVariableAssignment *op_variable_assignment =
      dynamic_cast<OperationVariableAssignment *>(operation);
  if (op_variable_assignment != nullptr)
    return run_variable_assignment(state, op_variable_assignment);

  OperationIf *op_if = dynamic_cast<OperationIf *>(operation);
  if (op_if != nullptr)
    return run_if(state, op_if);

  OperationElse *op_else = dynamic_cast<OperationElse *>(operation);
  if (op_else != nullptr)
    return state->none_value->ref(); // Resolved in IF

  OperationWhile *op_while = dynamic_cast<OperationWhile *>(operation);
  if (op_while != nullptr)
    return run_while(state, op_while);

  OperationFunctionDefinition *op_function_definition =
      dynamic_cast<OperationFunctionDefinition *>(operation);
  if (op_function_definition != nullptr)
    return state->none_value->ref(); // Resolved at compile time

  OperationFunctionCall *op_function_call =
      dynamic_cast<OperationFunctionCall *>(operation);
  if (op_function_call != nullptr)
    return run_function_call(state, op_function_call);

  OperationReturn *op_return = dynamic_cast<OperationReturn *>(operation);
  if (op_return != nullptr)
    return run_return(state, op_return);

  OperationAssert *op_assert = dynamic_cast<OperationAssert *>(operation);
  if (op_assert != nullptr)
    return run_assert(state, op_assert);

  OperationBooleanConstant *op_boolean_constant =
      dynamic_cast<OperationBooleanConstant *>(operation);
  if (op_boolean_constant != nullptr)
    return run_boolean_constant(state, op_boolean_constant);

  OperationNumberConstant *op_number_constant =
      dynamic_cast<OperationNumberConstant *>(operation);
  if (op_number_constant != nullptr)
    return run_number_constant(state, op_number_constant);

  OperationTextConstant *op_text_constant =
      dynamic_cast<OperationTextConstant *>(operation);
  if (op_text_constant != nullptr)
    return run_text_constant(state, op_text_constant);

  OperationVariableValue *op_variable_value =
      dynamic_cast<OperationVariableValue *>(operation);
  if (op_variable_value != nullptr)
    return run_variable_value(state, op_variable_value);

  OperationMemberValue *op_member_value =
      dynamic_cast<OperationMemberValue *>(operation);
  if (op_member_value != nullptr)
    return run_member_value(state, op_member_value);

  OperationBinary *op_binary = dynamic_cast<OperationBinary *>(operation);
  if (op_binary != nullptr)
    return run_binary(state, op_binary);

  return state->none_value->ref();
}

void elf_run(const char *data, OperationModule *module) {
  ProgramState *state = new ProgramState;
  memset(state, 0, sizeof(ProgramState));
  state->data = data;
  state->none_value = data_value_new(DATA_TYPE_NONE);

  DataValue *result = run_module(state, module);
  result->unref();

  state->none_value->unref();
  for (size_t i = 0; i < state->variables_length; i++)
    delete state->variables[i];
  free(state->variables);
  delete state;
}
