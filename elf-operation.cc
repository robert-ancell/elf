/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-operation.h"

bool OperationFunctionCall::is_constant() {
  // FIXME: Check if parameters are constant
  return function->is_constant();
}

bool OperationVariableValue::is_constant() {
  // FIXME: Have to follow chain of variable assignment
  return false;
}

std::string OperationMemberValue::get_data_type() {
  // FIXME
  return nullptr;
}

bool OperationMemberValue::is_constant() {
  // FIXME:
  return false;
}

bool OperationFunctionDefinition::is_constant() {
  // FIXME: Should scan function for return value
  return false;
}

std::string OperationUnary::get_data_type() {
  // FIXME: Type depends on operation
  return value->get_data_type();
}

std::string OperationBinary::get_data_type() {
  // FIXME: Need to combine data type
  return a->get_data_type();
}

std::shared_ptr<Operation> Operation::get_last_child() {
  size_t n_children = get_n_children();
  if (n_children > 0)
    return get_child(n_children - 1);
  else
    return nullptr;
}
