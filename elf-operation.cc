/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-operation.h"

#include <string.h>

#include "utils.h"

static Operation *operation_new(OperationType type, size_t size) {
  Operation *o = static_cast<Operation *>(malloc(size));
  memset(o, 0, size);
  o->type = type;
  o->ref_count = 1;

  return o;
}

Operation *make_module(void) {
  OperationModule *o = (OperationModule *)operation_new(
      OPERATION_TYPE_MODULE, sizeof(OperationModule));
  return (Operation *)o;
}

Operation *make_variable_definition(Token *data_type, Token *name,
                                    Operation *value) {
  OperationVariableDefinition *o = (OperationVariableDefinition *)operation_new(
      OPERATION_TYPE_VARIABLE_DEFINITION, sizeof(OperationVariableDefinition));
  o->data_type = token_ref(data_type);
  o->name = token_ref(name);
  o->value = operation_ref(value);
  return (Operation *)o;
}

Operation *make_variable_assignment(Token *name, Operation *value,
                                    OperationVariableDefinition *variable) {
  OperationVariableAssignment *o = (OperationVariableAssignment *)operation_new(
      OPERATION_TYPE_VARIABLE_ASSIGNMENT, sizeof(OperationVariableAssignment));
  o->name = token_ref(name);
  o->value = operation_ref(value);
  o->variable = variable;
  return (Operation *)o;
}

Operation *make_if(Operation *condition) {
  OperationIf *o =
      (OperationIf *)operation_new(OPERATION_TYPE_IF, sizeof(OperationIf));
  o->condition = operation_ref(condition);
  return (Operation *)o;
}

Operation *make_else(void) {
  OperationElse *o = (OperationElse *)operation_new(OPERATION_TYPE_ELSE,
                                                    sizeof(OperationElse));
  return (Operation *)o;
}

Operation *make_while(Operation *condition) {
  OperationWhile *o = (OperationWhile *)operation_new(OPERATION_TYPE_WHILE,
                                                      sizeof(OperationWhile));
  o->condition = operation_ref(condition);
  return (Operation *)o;
}

Operation *make_function_definition(Token *data_type, Token *name,
                                    OperationVariableDefinition **parameters) {
  OperationFunctionDefinition *o = (OperationFunctionDefinition *)operation_new(
      OPERATION_TYPE_FUNCTION_DEFINITION, sizeof(OperationFunctionDefinition));
  o->data_type = token_ref(data_type);
  o->name = token_ref(name);
  o->parameters = parameters;
  return (Operation *)o;
}

Operation *make_function_call(Token *name, Operation **parameters,
                              OperationFunctionDefinition *function) {
  OperationFunctionCall *o = (OperationFunctionCall *)operation_new(
      OPERATION_TYPE_FUNCTION_CALL, sizeof(OperationFunctionCall));
  o->name = token_ref(name);
  o->parameters = parameters;
  o->function = function;
  return (Operation *)o;
}

Operation *make_return(Operation *value,
                       OperationFunctionDefinition *function) {
  OperationReturn *o = (OperationReturn *)operation_new(
      OPERATION_TYPE_RETURN, sizeof(OperationReturn));
  o->value = value;
  o->function = function;
  return (Operation *)o;
}

Operation *make_assert(Token *name, Operation *expression) {
  OperationAssert *o = (OperationAssert *)operation_new(
      OPERATION_TYPE_ASSERT, sizeof(OperationAssert));
  o->name = token_ref(name);
  o->expression = operation_ref(expression);
  return (Operation *)o;
}

Operation *make_boolean_constant(Token *value) {
  OperationBooleanConstant *o = (OperationBooleanConstant *)operation_new(
      OPERATION_TYPE_BOOLEAN_CONSTANT, sizeof(OperationBooleanConstant));
  o->value = token_ref(value);
  return (Operation *)o;
}

Operation *make_number_constant(Token *value) {
  OperationNumberConstant *o = (OperationNumberConstant *)operation_new(
      OPERATION_TYPE_NUMBER_CONSTANT, sizeof(OperationNumberConstant));
  o->value = token_ref(value);
  return (Operation *)o;
}

Operation *make_text_constant(Token *value) {
  OperationTextConstant *o = (OperationTextConstant *)operation_new(
      OPERATION_TYPE_TEXT_CONSTANT, sizeof(OperationTextConstant));
  o->value = token_ref(value);
  return (Operation *)o;
}

Operation *make_variable_value(Token *name,
                               OperationVariableDefinition *variable) {
  OperationVariableValue *o = (OperationVariableValue *)operation_new(
      OPERATION_TYPE_VARIABLE_VALUE, sizeof(OperationVariableValue));
  o->name = token_ref(name);
  o->variable = variable;
  return (Operation *)o;
}

Operation *make_member_value(Operation *object, Token *member,
                             Operation **parameters) {
  OperationMemberValue *o = (OperationMemberValue *)operation_new(
      OPERATION_TYPE_MEMBER_VALUE, sizeof(OperationMemberValue));
  o->object = operation_ref(object);
  o->member = token_ref(member);
  o->parameters = parameters;
  return (Operation *)o;
}

Operation *make_binary(Token *op, Operation *a, Operation *b) {
  OperationBinary *o = (OperationBinary *)operation_new(
      OPERATION_TYPE_BINARY, sizeof(OperationBinary));
  o->op = token_ref(op);
  o->a = operation_ref(a);
  o->b = operation_ref(b);
  return (Operation *)o;
}

bool operation_is_constant(Operation *operation) {
  switch (operation->type) {
  case OPERATION_TYPE_VARIABLE_DEFINITION: {
    OperationVariableDefinition *op = (OperationVariableDefinition *)operation;
    return op->value == NULL || operation_is_constant(op->value);
  }
  case OPERATION_TYPE_VARIABLE_ASSIGNMENT: {
    OperationVariableAssignment *op = (OperationVariableAssignment *)operation;
    return operation_is_constant(op->value);
  }
  case OPERATION_TYPE_FUNCTION_CALL: {
    OperationFunctionCall *op = (OperationFunctionCall *)operation;
    // FIXME: Check if parameters are constant
    return operation_is_constant((Operation *)op->function);
  }
  case OPERATION_TYPE_RETURN: {
    OperationReturn *op = (OperationReturn *)operation;
    return op->value == NULL || operation_is_constant(op->value);
  }
  case OPERATION_TYPE_ASSERT: {
    OperationReturn *op = (OperationReturn *)operation;
    return op->value == NULL || operation_is_constant(op->value);
  }
  case OPERATION_TYPE_BOOLEAN_CONSTANT:
  case OPERATION_TYPE_NUMBER_CONSTANT:
  case OPERATION_TYPE_TEXT_CONSTANT:
    return true;
  case OPERATION_TYPE_VARIABLE_VALUE: {
    // FIXME: Have to follow chain of variable assignment
    return false;
  }
  case OPERATION_TYPE_MEMBER_VALUE: {
    // FIXME:
    return false;
  }
  case OPERATION_TYPE_BINARY: {
    OperationBinary *op = (OperationBinary *)operation;
    return operation_is_constant(op->a) && operation_is_constant(op->b);
  }
  case OPERATION_TYPE_FUNCTION_DEFINITION: {
    // FIXME: Should scan function for return value
    return false;
  }
  case OPERATION_TYPE_MODULE:
  case OPERATION_TYPE_IF:
  case OPERATION_TYPE_ELSE:
  case OPERATION_TYPE_WHILE:
    return false;
  }

  return false;
}

char *operation_get_data_type(Operation *operation, const char *data) {
  switch (operation->type) {
  case OPERATION_TYPE_VARIABLE_DEFINITION: {
    OperationVariableDefinition *op = (OperationVariableDefinition *)operation;
    return token_get_text(op->data_type, data);
  }
  case OPERATION_TYPE_VARIABLE_ASSIGNMENT: {
    OperationVariableAssignment *op = (OperationVariableAssignment *)operation;
    return operation_get_data_type((Operation *)op->variable, data);
  }
  case OPERATION_TYPE_FUNCTION_CALL: {
    OperationFunctionCall *op = (OperationFunctionCall *)operation;
    return operation_get_data_type((Operation *)op->function, data);
  }
  case OPERATION_TYPE_RETURN: {
    OperationReturn *op = (OperationReturn *)operation;
    return operation_get_data_type((Operation *)op->function, data);
  }
  case OPERATION_TYPE_ASSERT: {
    OperationReturn *op = (OperationReturn *)operation;
    return operation_get_data_type((Operation *)op->function, data);
  }
  case OPERATION_TYPE_BOOLEAN_CONSTANT:
    return str_new("bool");
  case OPERATION_TYPE_NUMBER_CONSTANT:
    return str_new("uint8"); // FIXME: Find minimum size
  case OPERATION_TYPE_TEXT_CONSTANT:
    return str_new("utf8");
  case OPERATION_TYPE_VARIABLE_VALUE: {
    OperationVariableValue *op = (OperationVariableValue *)operation;
    return operation_get_data_type((Operation *)op->variable, data);
  }
  case OPERATION_TYPE_MEMBER_VALUE: {
    // FIXME:
    return NULL;
  }
  case OPERATION_TYPE_BINARY: {
    OperationBinary *op = (OperationBinary *)operation;
    // FIXME: Need to combine data type
    return operation_get_data_type(op->a, data);
  }
  case OPERATION_TYPE_FUNCTION_DEFINITION: {
    OperationFunctionDefinition *op = (OperationFunctionDefinition *)operation;
    return token_get_text(op->data_type, data);
  }
  case OPERATION_TYPE_MODULE:
  case OPERATION_TYPE_IF:
  case OPERATION_TYPE_ELSE:
  case OPERATION_TYPE_WHILE:
    return NULL;
  }

  return NULL;
}

void operation_add_child(Operation *operation, Operation *child) {
  if (operation->type == OPERATION_TYPE_MODULE) {
    OperationModule *o = (OperationModule *)operation;
    o->body_length++;
    o->body = static_cast<Operation **>(
        realloc(o->body, sizeof(Operation *) * o->body_length));
    o->body[o->body_length - 1] = operation_ref(child);
  } else if (operation->type == OPERATION_TYPE_FUNCTION_DEFINITION) {
    OperationFunctionDefinition *o = (OperationFunctionDefinition *)operation;
    o->body_length++;
    o->body = static_cast<Operation **>(
        realloc(o->body, sizeof(Operation *) * o->body_length));
    o->body[o->body_length - 1] = operation_ref(child);
  } else if (operation->type == OPERATION_TYPE_IF) {
    OperationIf *o = (OperationIf *)operation;
    o->body_length++;
    o->body = static_cast<Operation **>(
        realloc(o->body, sizeof(Operation *) * o->body_length));
    o->body[o->body_length - 1] = operation_ref(child);
  } else if (operation->type == OPERATION_TYPE_ELSE) {
    OperationElse *o = (OperationElse *)operation;
    o->body_length++;
    o->body = static_cast<Operation **>(
        realloc(o->body, sizeof(Operation *) * o->body_length));
    o->body[o->body_length - 1] = operation_ref(child);
  } else if (operation->type == OPERATION_TYPE_WHILE) {
    OperationWhile *o = (OperationWhile *)operation;
    o->body_length++;
    o->body = static_cast<Operation **>(
        realloc(o->body, sizeof(Operation *) * o->body_length));
    o->body[o->body_length - 1] = operation_ref(child);
  }
}

size_t operation_get_n_children(Operation *operation) {
  if (operation->type == OPERATION_TYPE_MODULE) {
    OperationModule *o = (OperationModule *)operation;
    return o->body_length;
  } else if (operation->type == OPERATION_TYPE_FUNCTION_DEFINITION) {
    OperationFunctionDefinition *o = (OperationFunctionDefinition *)operation;
    return o->body_length;
  } else if (operation->type == OPERATION_TYPE_IF) {
    OperationIf *o = (OperationIf *)operation;
    return o->body_length;
  } else if (operation->type == OPERATION_TYPE_ELSE) {
    OperationElse *o = (OperationElse *)operation;
    return o->body_length;
  } else if (operation->type == OPERATION_TYPE_WHILE) {
    OperationWhile *o = (OperationWhile *)operation;
    return o->body_length;
  }

  return 0;
}

Operation *operation_get_child(Operation *operation, size_t index) {
  if (operation->type == OPERATION_TYPE_MODULE) {
    OperationModule *o = (OperationModule *)operation;
    return o->body[index];
  } else if (operation->type == OPERATION_TYPE_FUNCTION_DEFINITION) {
    OperationFunctionDefinition *o = (OperationFunctionDefinition *)operation;
    return o->body[index];
  } else if (operation->type == OPERATION_TYPE_IF) {
    OperationIf *o = (OperationIf *)operation;
    return o->body[index];
  } else if (operation->type == OPERATION_TYPE_ELSE) {
    OperationElse *o = (OperationElse *)operation;
    return o->body[index];
  } else if (operation->type == OPERATION_TYPE_WHILE) {
    OperationWhile *o = (OperationWhile *)operation;
    return o->body[index];
  }

  return NULL;
}

Operation *operation_get_last_child(Operation *operation) {
  size_t n_children = operation_get_n_children(operation);
  if (n_children > 0)
    return operation_get_child(operation, n_children - 1);
  else
    return NULL;
}

char *operation_to_string(Operation *operation) {
  switch (operation->type) {
  case OPERATION_TYPE_MODULE:
    return str_printf("MODULE");
  case OPERATION_TYPE_VARIABLE_DEFINITION:
    return str_printf("VARIABLE_DEFINITION");
  case OPERATION_TYPE_VARIABLE_ASSIGNMENT:
    return str_printf("VARIABLE_ASSIGNMENT");
  case OPERATION_TYPE_IF:
    return str_printf("IF");
  case OPERATION_TYPE_ELSE:
    return str_printf("ELSE");
  case OPERATION_TYPE_WHILE:
    return str_printf("WHILE");
  case OPERATION_TYPE_FUNCTION_DEFINITION:
    return str_printf("FUNCTION_DEFINITION");
  case OPERATION_TYPE_FUNCTION_CALL:
    return str_printf("FUNCTION_CALL");
  case OPERATION_TYPE_RETURN: {
    OperationReturn *op = (OperationReturn *)operation;
    autofree_str value_string = operation_to_string(op->value);
    return str_printf("RETURN(%s)", value_string);
  }
  case OPERATION_TYPE_ASSERT: {
    OperationAssert *op = (OperationAssert *)operation;
    autofree_str expression_string = operation_to_string(op->expression);
    return str_printf("ASSERT(%s)", expression_string);
  }
  case OPERATION_TYPE_BOOLEAN_CONSTANT: {
    OperationNumberConstant *op = (OperationNumberConstant *)operation;
    autofree_str value_string = token_to_string(op->value);
    return str_printf("BOOLEAN_CONSTANT(%s)", value_string);
  }
  case OPERATION_TYPE_NUMBER_CONSTANT: {
    OperationNumberConstant *op = (OperationNumberConstant *)operation;
    autofree_str value_string = token_to_string(op->value);
    return str_printf("NUMBER_CONSTANT(%s)", value_string);
  }
  case OPERATION_TYPE_TEXT_CONSTANT: {
    OperationTextConstant *op = (OperationTextConstant *)operation;
    autofree_str value_string = token_to_string(op->value);
    return str_printf("TEXT_CONSTANT(%s)", value_string);
  }
  case OPERATION_TYPE_VARIABLE_VALUE:
    return str_printf("VARIABLE_VALUE");
  case OPERATION_TYPE_MEMBER_VALUE: {
    OperationMemberValue *op = (OperationMemberValue *)operation;
    char *member_string = token_to_string(op->member);
    return str_printf("MEMBER_VALUE(%s)", member_string);
  }
  case OPERATION_TYPE_BINARY:
    return str_printf("BINARY");
  }

  return str_printf("UNKNOWN(%d)", operation->type);
}

Operation *operation_ref(Operation *operation) {
  if (operation == NULL)
    return NULL;

  operation->ref_count++;
  return operation;
}

void operation_unref(Operation *operation) {
  if (operation == NULL)
    return;

  operation->ref_count--;
  if (operation->ref_count != 0)
    return;

  switch (operation->type) {
  case OPERATION_TYPE_MODULE: {
    OperationModule *op = (OperationModule *)operation;
    for (size_t i = 0; i < op->body_length; i++)
      operation_unref(op->body[i]);
    free(op->body);
    break;
  }
  case OPERATION_TYPE_VARIABLE_DEFINITION: {
    OperationVariableDefinition *op = (OperationVariableDefinition *)operation;
    operation_unref(op->value);
    break;
  }
  case OPERATION_TYPE_VARIABLE_ASSIGNMENT: {
    OperationVariableAssignment *op = (OperationVariableAssignment *)operation;
    token_unref(op->name);
    operation_unref(op->value);
    break;
  }
  case OPERATION_TYPE_IF: {
    OperationIf *op = (OperationIf *)operation;
    operation_unref(op->condition);
    for (size_t i = 0; i < op->body_length; i++)
      operation_unref(op->body[i]);
    free(op->body);
    break;
  }
  case OPERATION_TYPE_ELSE: {
    OperationElse *op = (OperationElse *)operation;
    for (size_t i = 0; i < op->body_length; i++)
      operation_unref(op->body[i]);
    free(op->body);
    break;
  }
  case OPERATION_TYPE_WHILE: {
    OperationWhile *op = (OperationWhile *)operation;
    operation_unref(op->condition);
    for (size_t i = 0; i < op->body_length; i++)
      operation_unref(op->body[i]);
    free(op->body);
    break;
  }
  case OPERATION_TYPE_FUNCTION_DEFINITION: {
    OperationFunctionDefinition *op = (OperationFunctionDefinition *)operation;
    token_unref(op->data_type);
    token_unref(op->name);
    for (int i = 0; op->parameters[i] != NULL; i++)
      operation_unref((Operation *)op->parameters[i]);
    free(op->parameters);
    for (size_t i = 0; i < op->body_length; i++)
      operation_unref(op->body[i]);
    free(op->body);
    break;
  }
  case OPERATION_TYPE_FUNCTION_CALL: {
    OperationFunctionCall *op = (OperationFunctionCall *)operation;
    token_unref(op->name);
    for (int i = 0; op->parameters[i] != NULL; i++)
      operation_unref(op->parameters[i]);
    free(op->parameters);
    break;
  }
  case OPERATION_TYPE_RETURN: {
    OperationReturn *op = (OperationReturn *)operation;
    token_unref(op->name);
    operation_unref(op->value);
    break;
  }
  case OPERATION_TYPE_ASSERT: {
    OperationAssert *op = (OperationAssert *)operation;
    token_unref(op->name);
    operation_unref(op->expression);
    break;
  }
  case OPERATION_TYPE_BOOLEAN_CONSTANT: {
    OperationBooleanConstant *op = (OperationBooleanConstant *)operation;
    token_unref(op->value);
    break;
  }
  case OPERATION_TYPE_NUMBER_CONSTANT: {
    OperationNumberConstant *op = (OperationNumberConstant *)operation;
    token_unref(op->value);
    break;
  }
  case OPERATION_TYPE_TEXT_CONSTANT: {
    OperationTextConstant *op = (OperationTextConstant *)operation;
    token_unref(op->value);
    break;
  }
  case OPERATION_TYPE_VARIABLE_VALUE: {
    OperationVariableValue *op = (OperationVariableValue *)operation;
    token_unref(op->name);
    break;
  }
  case OPERATION_TYPE_MEMBER_VALUE: {
    OperationMemberValue *op = (OperationMemberValue *)operation;
    operation_unref(op->object);
    token_unref(op->member);
    for (int i = 0; op->parameters[i] != NULL; i++)
      operation_unref(op->parameters[i]);
    free(op->parameters);
    break;
  }
  case OPERATION_TYPE_BINARY: {
    OperationBinary *op = (OperationBinary *)operation;
    token_unref(op->op);
    operation_unref(op->a);
    operation_unref(op->b);
    break;
  }
  }

  free(operation);
}

void operation_cleanup(Operation **operation) {
  if (*operation == NULL)
    return;
  operation_unref(*operation);
  *operation = NULL;
}
