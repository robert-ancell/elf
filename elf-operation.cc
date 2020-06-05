/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-operation.h"

std::shared_ptr<Operation>
OperationPrimitiveDefinition::find_member(const std::string &name) {
  for (auto i = body.begin(); i != body.end(); i++) {
    auto function_definition =
        std::dynamic_pointer_cast<OperationFunctionDefinition>(*i);
    if (function_definition != nullptr &&
        function_definition->name->has_text(name))
      return function_definition;
  }
  return nullptr;
}

std::shared_ptr<Operation>
OperationTypeDefinition::find_member(const std::string &name) {
  for (auto i = body.begin(); i != body.end(); i++) {
    auto function_definition =
        std::dynamic_pointer_cast<OperationFunctionDefinition>(*i);
    if (function_definition != nullptr &&
        function_definition->name->has_text(name))
      return function_definition;
    auto variable_definition =
        std::dynamic_pointer_cast<OperationVariableDefinition>(*i);
    if (variable_definition != nullptr &&
        variable_definition->name->has_text(name))
      return variable_definition;
  }
  return nullptr;
}

bool OperationCall::is_constant() {
  // FIXME: Check if parameters are constant
  return value->is_constant();
}

std::string OperationMember::get_data_type() {
  // FIXME
  return nullptr;
}

bool OperationMember::is_constant() {
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
