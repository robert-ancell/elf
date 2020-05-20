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
  for (size_t i = 0; i < body_length; i++)
    body[i]->unref();
  free(body);
}

bool OperationFunctionCall::is_constant() {
  // FIXME: Check if parameters are constant
  return function->is_constant();
}

OperationFunctionCall::~OperationFunctionCall() {
  name->unref();
  for (int i = 0; parameters[i] != nullptr; i++)
    parameters[i]->unref();
  free(parameters);
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
  for (int i = 0; parameters[i] != nullptr; i++)
    parameters[i]->unref();
  free(parameters);
}

bool OperationFunctionDefinition::is_constant() {
  // FIXME: Should scan function for return value
  return false;
}

void OperationFunctionDefinition::add_child(Operation *child) {
  body_length++;
  body = static_cast<Operation **>(
      realloc(body, sizeof(Operation *) * body_length));
  body[body_length - 1] = child->ref();
}

OperationFunctionDefinition::~OperationFunctionDefinition() {
  data_type->unref();
  name->unref();
  for (int i = 0; parameters[i] != nullptr; i++)
    parameters[i]->unref();
  free(parameters);
  for (size_t i = 0; i < body_length; i++)
    body[i]->unref();
  free(body);
}

std::string OperationNumberConstant::get_data_type(const char *data) {
  return "uint8"; // FIXME: Find minimum size
}

std::string OperationBinary::get_data_type(const char *data) {
  // FIXME: Need to combine data type
  return a->get_data_type(data);
}

void OperationModule::add_child(Operation *child) {
  body_length++;
  body = static_cast<Operation **>(
      realloc(body, sizeof(Operation *) * body_length));
  body[body_length - 1] = child->ref();
}

void OperationIf::add_child(Operation *child) {
  body_length++;
  body = static_cast<Operation **>(
      realloc(body, sizeof(Operation *) * body_length));
  body[body_length - 1] = child->ref();
}

void OperationElse::add_child(Operation *child) {
  body_length++;
  body = static_cast<Operation **>(
      realloc(body, sizeof(Operation *) * body_length));
  body[body_length - 1] = child->ref();
}

void OperationWhile::add_child(Operation *child) {
  body_length++;
  body = static_cast<Operation **>(
      realloc(body, sizeof(Operation *) * body_length));
  body[body_length - 1] = child->ref();
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
  for (size_t i = 0; i < body_length; i++)
    body[i]->unref();
  free(body);
}

OperationElse::~OperationElse() {
  keyword->unref();
  for (size_t i = 0; i < body_length; i++)
    body[i]->unref();
  free(body);
}

OperationWhile::~OperationWhile() {
  condition->unref();
  for (size_t i = 0; i < body_length; i++)
    body[i]->unref();
  free(body);
}

void operation_cleanup(Operation **operation) {
  if (*operation == nullptr)
    return;
  (*operation)->unref();
  *operation = nullptr;
}
