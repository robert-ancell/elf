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

OperationModule::~OperationModule() {
  for (auto i = body.begin(); i != body.end(); i++)
    (*i)->unref();
}

bool OperationFunctionCall::is_constant() {
  // FIXME: Check if parameters are constant
  return function->is_constant();
}

OperationFunctionCall::~OperationFunctionCall() {
  name->unref();
  for (auto i = parameters.begin(); i != parameters.end(); i++)
    (*i)->unref();
}

bool OperationVariableValue::is_constant() {
  // FIXME: Have to follow chain of variable assignment
  return false;
}

std::string OperationMemberValue::get_data_type(const char *data) {
  // FIXME
  return nullptr;
}

bool OperationMemberValue::is_constant() {
  // FIXME:
  return false;
}

OperationMemberValue::~OperationMemberValue() {
  object->unref();
  member->unref();
  for (auto i = parameters.begin(); i != parameters.end(); i++)
    (*i)->unref();
}

bool OperationFunctionDefinition::is_constant() {
  // FIXME: Should scan function for return value
  return false;
}

OperationFunctionDefinition::~OperationFunctionDefinition() {
  data_type->unref();
  name->unref();
  for (auto i = parameters.begin(); i != parameters.end(); i++)
    (*i)->unref();
  for (auto i = body.begin(); i != body.end(); i++)
    (*i)->unref();
}

std::string OperationNumberConstant::get_data_type(const char *data) {
  return "uint8"; // FIXME: Find minimum size
}

std::string OperationBinary::get_data_type(const char *data) {
  // FIXME: Need to combine data type
  return a->get_data_type(data);
}

Operation *Operation::get_last_child() {
  size_t n_children = get_n_children();
  if (n_children > 0)
    return get_child(n_children - 1);
  else
    return nullptr;
}

OperationIf::~OperationIf() {
  keyword->unref();
  condition->unref();
  for (auto i = body.begin(); i != body.end(); i++)
    (*i)->unref();
}

OperationElse::~OperationElse() {
  keyword->unref();
  for (auto i = body.begin(); i != body.end(); i++)
    (*i)->unref();
}

OperationWhile::~OperationWhile() {
  condition->unref();
  for (auto i = body.begin(); i != body.end(); i++)
    (*i)->unref();
}

void operation_cleanup(Operation **operation) {
  if (*operation == nullptr)
    return;
  (*operation)->unref();
  *operation = nullptr;
}
