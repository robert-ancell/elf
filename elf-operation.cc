/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-operation.h"

bool OperationModule::is_constant() { return true; };

std::string OperationModule::to_string() { return "MODULE"; }

std::string OperationPrimitiveDefinition::get_data_type() {
  return name->get_text();
}

std::string OperationPrimitiveDefinition::to_string() {
  return "PRIMITIVE_DEFINITION";
}

std::shared_ptr<Operation>
OperationPrimitiveDefinition::find_member(const std::string &name) {
  for (auto i = children.begin(); i != children.end(); i++) {
    auto function_definition =
        std::dynamic_pointer_cast<OperationFunctionDefinition>(*i);
    if (function_definition != nullptr &&
        function_definition->name->has_text(name))
      return function_definition;
  }
  return nullptr;
}

std::string OperationTypeDefinition::get_data_type() {
  return name->get_text();
}

std::string OperationTypeDefinition::to_string() { return "TYPE_DEFINITION"; }

std::shared_ptr<Operation>
OperationTypeDefinition::find_member(const std::string &name) {
  for (auto i = children.begin(); i != children.end(); i++) {
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

std::string OperationDataType::get_data_type() {
  if (is_array)
    return name->get_text() + "[]";
  else
    return name->get_text();
}

std::string OperationDataType::to_string() { return "DATA_TYPE"; }

bool OperationVariableDefinition::is_constant() {
  return value == nullptr || value->is_constant();
}

std::string OperationVariableDefinition::get_data_type() {
  return data_type->get_data_type();
}

std::string OperationVariableDefinition::to_string() {
  return "VARIABLE_DEFINITION";
}

std::string OperationSymbol::get_data_type() {
  return definition != nullptr ? definition->get_data_type() : nullptr;
}

std::string OperationSymbol::to_string() { return "SYMBOL"; }

bool OperationAssignment::is_constant() {
  return target->is_constant() && value->is_constant();
}

std::string OperationAssignment::get_data_type() {
  return target->get_data_type();
}

std::string OperationAssignment::to_string() { return "ASSIGNMENT"; }

std::string OperationIf::to_string() { return "IF"; }

std::string OperationElse::to_string() { return "ELSE"; }

std::string OperationWhile::to_string() { return "WHILE"; }

bool OperationFunctionDefinition::is_constant() {
  // FIXME: Should scan function for return value
  return false;
}

std::string OperationFunctionDefinition::get_data_type() {
  return data_type->get_data_type();
}

std::string OperationFunctionDefinition::to_string() {
  return "FUNCTION_DEFINITION";
}

bool OperationCall::is_constant() {
  // FIXME: Check if parameters are constant
  return value->is_constant();
}

std::string OperationCall::get_data_type() { return value->get_data_type(); }

std::string OperationCall::to_string() { return "CALL"; }

bool OperationReturn::is_constant() {
  return value == nullptr || value->is_constant();
}

std::string OperationReturn::get_data_type() {
  return function->get_data_type();
}

std::string OperationReturn::to_string() {
  return "RETURN(" + value->to_string() + ")";
}

bool OperationAssert::is_constant() {
  return expression == nullptr || expression->is_constant();
}

std::string OperationAssert::to_string() {
  return "ASSERT(" + expression->to_string() + ")";
}

bool OperationTrue::is_constant() { return true; }

std::string OperationTrue::get_data_type() { return "bool"; }

std::string OperationTrue::to_string() { return "TRUE"; }

bool OperationFalse::is_constant() { return true; }

std::string OperationFalse::get_data_type() { return "bool"; }

std::string OperationFalse::to_string() { return "FALSE"; }

bool OperationNumberConstant::is_constant() { return true; }

std::string OperationNumberConstant::get_data_type() { return data_type; }

std::string OperationNumberConstant::to_string() {
  return "NUMBER_CONSTANT(" + (sign_token != nullptr)
             ? "-"
             : "" + std::to_string(magnitude) + ")";
}

bool OperationTextConstant::is_constant() { return true; }

std::string OperationTextConstant::get_data_type() { return "utf8"; }

std::string OperationTextConstant::to_string() {
  return "TEXT_CONSTANT(" + value + ")";
}

bool OperationArrayConstant::is_constant() {
  for (auto i = values.begin(); i != values.end(); i++) {
    auto value = *i;
    if (!value->is_constant())
      return false;
  }

  return true;
}

std::string OperationArrayConstant::get_data_type() {
  if (values.size() == 0)
    return "[]";
  else
    return values[0]->get_data_type() + "[]";
}

std::string OperationArrayConstant::to_string() { return "ARRAY_CONSTANT"; }

bool OperationMember::is_constant() {
  // FIXME:
  return false;
}

std::string OperationMember::get_data_type() {
  return member_definition->get_data_type();
}

std::string OperationMember::to_string() {
  return "MEMBER(" + member->to_string() + ")";
}

std::string OperationMember::get_member_name() {
  return std::string(member->data + member->offset + 1, member->length - 1);
}

bool OperationUnary::is_constant() { return value->is_constant(); }

std::string OperationUnary::get_data_type() {
  // FIXME: Type depends on operation
  return value->get_data_type();
}

std::string OperationUnary::to_string() { return "UNARY"; }

bool OperationBinary::is_constant() {
  return a->is_constant() && b->is_constant();
}

std::string OperationBinary::get_data_type() {
  // FIXME: Need to combine data type
  return a->get_data_type();
}

std::string OperationBinary::to_string() { return "BINARY"; }

bool OperationConvert::is_constant() { return op->is_constant(); }

std::string OperationConvert::get_data_type() { return data_type; }

std::string OperationConvert::to_string() { return "CONVERT"; }
