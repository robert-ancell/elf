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

Operation *
make_variable_definition (Token *data_type, Token *name, Operation *value)
{
    OperationVariableDefinition *o = malloc (sizeof (OperationVariableDefinition));
    memset (o, 0, sizeof (OperationVariableDefinition));
    o->type = OPERATION_TYPE_VARIABLE_DEFINITION;
    o->data_type = data_type;
    o->name = name;
    o->value = value;
    return (Operation *) o;
}

Operation *
make_variable_assignment (Token *name, Operation *value, OperationVariableDefinition *variable)
{
    OperationVariableAssignment *o = malloc (sizeof (OperationVariableAssignment));
    memset (o, 0, sizeof (OperationVariableAssignment));
    o->type = OPERATION_TYPE_VARIABLE_ASSIGNMENT;
    o->name = name;
    o->value = value;
    o->variable = variable;
    return (Operation *) o;
}

Operation *
make_if (Operation *condition)
{
    OperationIf *o = malloc (sizeof (OperationIf));
    memset (o, 0, sizeof (OperationIf));
    o->type = OPERATION_TYPE_IF;
    o->condition = condition;
    return (Operation *) o;
}

Operation *
make_else (void)
{
    OperationElse *o = malloc (sizeof (OperationElse));
    memset (o, 0, sizeof (OperationElse));
    o->type = OPERATION_TYPE_ELSE;
    return (Operation *) o;
}

Operation *
make_while (Operation *condition)
{
    OperationWhile *o = malloc (sizeof (OperationWhile));
    memset (o, 0, sizeof (OperationWhile));
    o->type = OPERATION_TYPE_WHILE;
    o->condition = condition;
    return (Operation *) o;
}

Operation *
make_function_definition (Token *data_type, Token *name, Operation **parameters)
{
    OperationFunctionDefinition *o = malloc (sizeof (OperationFunctionDefinition));
    memset (o, 0, sizeof (OperationFunctionDefinition));
    o->type = OPERATION_TYPE_FUNCTION_DEFINITION;
    o->data_type = data_type;
    o->name = name;
    o->parameters = parameters;
    return (Operation *) o;
}

Operation *
make_function_call (Token *name, Operation **parameters, OperationFunctionDefinition *function)
{
    OperationFunctionCall *o = malloc (sizeof (OperationFunctionCall));
    memset (o, 0, sizeof (OperationFunctionCall));
    o->type = OPERATION_TYPE_FUNCTION_CALL;
    o->name = name;
    o->parameters = parameters;
    o->function = function;
    return (Operation *) o;
}

Operation *
make_return (Operation *value, OperationFunctionDefinition *function)
{
    OperationReturn *o = malloc (sizeof (OperationReturn));
    memset (o, 0, sizeof (OperationReturn));
    o->type = OPERATION_TYPE_RETURN;
    o->value = value;
    o->function = function;
    return (Operation *) o;
}

Operation *
make_boolean_constant (Token *value)
{
    OperationBooleanConstant *o = malloc (sizeof (OperationBooleanConstant));
    memset (o, 0, sizeof (OperationBooleanConstant));
    o->type = OPERATION_TYPE_BOOLEAN_CONSTANT;
    o->value = value;
    return (Operation *) o;
}

Operation *
make_number_constant (Token *value)
{
    OperationNumberConstant *o = malloc (sizeof (OperationNumberConstant));
    memset (o, 0, sizeof (OperationNumberConstant));
    o->type = OPERATION_TYPE_NUMBER_CONSTANT;
    o->value = value;
    return (Operation *) o;
}

Operation *
make_text_constant (Token *value)
{
    OperationTextConstant *o = malloc (sizeof (OperationTextConstant));
    memset (o, 0, sizeof (OperationTextConstant));
    o->type = OPERATION_TYPE_TEXT_CONSTANT;
    o->value = value;
    return (Operation *) o;
}

Operation *
make_variable_value (Token *name, OperationVariableDefinition *variable)
{
    OperationVariableValue *o = malloc (sizeof (OperationVariableValue));
    memset (o, 0, sizeof (OperationVariableValue));
    o->type = OPERATION_TYPE_VARIABLE_VALUE;
    o->name = name;
    o->variable = variable;
    return (Operation *) o;
}

Operation *
make_member_value (Operation *object, Token *member)
{
    OperationMemberValue *o = malloc (sizeof (OperationMemberValue));
    memset (o, 0, sizeof (OperationMemberValue));
    o->type = OPERATION_TYPE_MEMBER_VALUE;
    o->object = object;
    o->member = member;
    return (Operation *) o;
}

Operation *
make_binary (Token *operator, Operation *a, Operation *b)
{
    OperationBinary *o = malloc (sizeof (OperationBinary));
    memset (o, 0, sizeof (OperationBinary));
    o->type = OPERATION_TYPE_BINARY;
    o->operator = operator;
    o->a = a;
    o->b = b;
    return (Operation *) o;
}

bool
operation_is_constant (Operation *operation)
{
    switch (operation->type) {
    case OPERATION_TYPE_VARIABLE_DEFINITION: {
        OperationVariableDefinition *op = (OperationVariableDefinition *) operation;
        return op->value == NULL || operation_is_constant (op->value);
    }
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT: {
        OperationVariableAssignment *op = (OperationVariableAssignment *) operation;
        return operation_is_constant (op->value);
    }
    case OPERATION_TYPE_FUNCTION_CALL: {
        OperationFunctionCall *op = (OperationFunctionCall *) operation;
        // FIXME: Check if parameters are constant
        return operation_is_constant ((Operation *) op->function);
    }
    case OPERATION_TYPE_RETURN: {
        OperationReturn *op = (OperationReturn *) operation;
        return op->value == NULL || operation_is_constant (op->value);
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
        OperationBinary *op = (OperationBinary *) operation;
        return operation_is_constant (op->a) && operation_is_constant (op->b);
    }
    case OPERATION_TYPE_FUNCTION_DEFINITION: {
        // FIXME: Should scan function for return value
        return false;
    }
    case OPERATION_TYPE_IF:
    case OPERATION_TYPE_ELSE:
    case OPERATION_TYPE_WHILE:
        return false;
    }

    return false;
}

char *
operation_get_data_type (Operation *operation, const char *data)
{
    switch (operation->type) {
    case OPERATION_TYPE_VARIABLE_DEFINITION: {
        OperationVariableDefinition *op = (OperationVariableDefinition *) operation;
        return token_get_text (op->data_type, data);
    }
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT: {
        OperationVariableAssignment *op = (OperationVariableAssignment *) operation;
        return operation_get_data_type ((Operation *) op->variable, data);
    }
    case OPERATION_TYPE_FUNCTION_CALL: {
        OperationFunctionCall *op = (OperationFunctionCall *) operation;
        return operation_get_data_type ((Operation *) op->function, data);
    }
    case OPERATION_TYPE_RETURN: {
        OperationReturn *op = (OperationReturn *) operation;
        return operation_get_data_type ((Operation *) op->function, data);
    }
    case OPERATION_TYPE_BOOLEAN_CONSTANT:
        return str_new ("bool");
    case OPERATION_TYPE_NUMBER_CONSTANT:
        return str_new ("uint8"); // FIXME: Find minimum size
    case OPERATION_TYPE_TEXT_CONSTANT:
        return str_new ("utf8");
    case OPERATION_TYPE_VARIABLE_VALUE: {
        OperationVariableValue *op = (OperationVariableValue *) operation;
        return operation_get_data_type ((Operation *) op->variable, data);
    }
    case OPERATION_TYPE_MEMBER_VALUE: {
        // FIXME:
        return NULL;
    }
    case OPERATION_TYPE_BINARY: {
        OperationBinary *op = (OperationBinary *) operation;
        // FIXME: Need to combine data type
        return operation_get_data_type (op->a, data);
    }
    case OPERATION_TYPE_FUNCTION_DEFINITION: {
        OperationFunctionDefinition *op = (OperationFunctionDefinition *) operation;
        return token_get_text (op->data_type, data);
    }
    case OPERATION_TYPE_IF:
    case OPERATION_TYPE_ELSE:
    case OPERATION_TYPE_WHILE:
        return NULL;
    }

    return NULL;
}

void
operation_add_child (Operation *operation, Operation *child)
{
    if (operation->type == OPERATION_TYPE_FUNCTION_DEFINITION) {
        OperationFunctionDefinition *o = (OperationFunctionDefinition *) operation;
        o->body_length++;
        o->body = realloc (o->body, sizeof (Operation *) * o->body_length);
        o->body[o->body_length - 1] = child;
    }
    else if (operation->type == OPERATION_TYPE_IF) {
        OperationIf *o = (OperationIf *) operation;
        o->body_length++;
        o->body = realloc (o->body, sizeof (Operation *) * o->body_length);
        o->body[o->body_length - 1] = child;
    }
    else if (operation->type == OPERATION_TYPE_ELSE) {
        OperationElse *o = (OperationElse *) operation;
        o->body_length++;
        o->body = realloc (o->body, sizeof (Operation *) * o->body_length);
        o->body[o->body_length - 1] = child;
    }
    else if (operation->type == OPERATION_TYPE_WHILE) {
        OperationWhile *o = (OperationWhile *) operation;
        o->body_length++;
        o->body = realloc (o->body, sizeof (Operation *) * o->body_length);
        o->body[o->body_length - 1] = child;
    }
}

size_t
operation_get_n_children (Operation *operation)
{
    if (operation->type == OPERATION_TYPE_FUNCTION_DEFINITION) {
        OperationFunctionDefinition *o = (OperationFunctionDefinition *) operation;
        return o->body_length;
    }
    else if (operation->type == OPERATION_TYPE_IF) {
        OperationIf *o = (OperationIf *) operation;
        return o->body_length;
    }
    else if (operation->type == OPERATION_TYPE_ELSE) {
        OperationElse *o = (OperationElse *) operation;
        return o->body_length;
    }
    else if (operation->type == OPERATION_TYPE_WHILE) {
        OperationWhile *o = (OperationWhile *) operation;
        return o->body_length;
    }

    return 0;
}

Operation *
operation_get_child (Operation *operation, size_t index)
{
    if (operation->type == OPERATION_TYPE_FUNCTION_DEFINITION) {
        OperationFunctionDefinition *o = (OperationFunctionDefinition *) operation;
        return o->body[index];
    }
    else if (operation->type == OPERATION_TYPE_IF) {
        OperationIf *o = (OperationIf *) operation;
        return o->body[index];
    }
    else if (operation->type == OPERATION_TYPE_ELSE) {
        OperationElse *o = (OperationElse *) operation;
        return o->body[index];
    }
    else if (operation->type == OPERATION_TYPE_WHILE) {
        OperationWhile *o = (OperationWhile *) operation;
        return o->body[index];
    }

    return NULL;
}

Operation *
operation_get_last_child (Operation *operation)
{
    size_t n_children = operation_get_n_children (operation);
    if (n_children > 0)
        return operation_get_child (operation, n_children - 1);
    else
        return NULL;
}

char *
operation_to_string (Operation *operation)
{
    switch (operation->type) {
    case OPERATION_TYPE_VARIABLE_DEFINITION:
        return str_printf ("VARIABLE_DEFINITION");
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT:
        return str_printf ("VARIABLE_ASSIGNMENT");
    case OPERATION_TYPE_IF:
        return str_printf ("IF");
    case OPERATION_TYPE_ELSE:
       return str_printf ("ELSE");
    case OPERATION_TYPE_WHILE:
       return str_printf ("WHILE");
    case OPERATION_TYPE_FUNCTION_DEFINITION:
        return str_printf ("FUNCTION_DEFINITION");
    case OPERATION_TYPE_FUNCTION_CALL:
        return str_printf ("FUNCTION_CALL");
    case OPERATION_TYPE_RETURN: {
        OperationReturn *op = (OperationReturn *) operation;
        autofree_str value_string = operation_to_string (op->value);
        return str_printf ("RETURN(%s)", value_string);
    }
    case OPERATION_TYPE_BOOLEAN_CONSTANT: {
        OperationNumberConstant *op = (OperationNumberConstant *) operation;
        autofree_str value_string = token_to_string (op->value);
        return str_printf ("BOOLEAN_CONSTANT(%s)", value_string);
    }
    case OPERATION_TYPE_NUMBER_CONSTANT: {
        OperationNumberConstant *op = (OperationNumberConstant *) operation;
        autofree_str value_string = token_to_string (op->value);
        return str_printf ("NUMBER_CONSTANT(%s)", value_string);
    }
    case OPERATION_TYPE_TEXT_CONSTANT: {
        OperationTextConstant *op = (OperationTextConstant *) operation;
        autofree_str value_string = token_to_string (op->value);
        return str_printf ("TEXT_CONSTANT(%s)", value_string);
    }
    case OPERATION_TYPE_VARIABLE_VALUE:
        return str_printf ("VARIABLE_VALUE");
    case OPERATION_TYPE_MEMBER_VALUE: {
        OperationMemberValue *op = (OperationMemberValue *) operation;
        char *member_string = token_to_string (op->member);
        return str_printf ("MEMBER_VALUE(%s)", member_string);
    }
    case OPERATION_TYPE_BINARY:
        return str_printf ("BINARY");
    }

    return str_printf ("UNKNOWN(%d)", operation->type);
}

void
operation_free (Operation *operation)
{
    if (operation == NULL)
        return;

    switch (operation->type) {
    case OPERATION_TYPE_VARIABLE_DEFINITION: {
        OperationVariableDefinition *op = (OperationVariableDefinition *) operation;
        operation_free (op->value);
        break;
    }
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT: {
        OperationVariableAssignment *op = (OperationVariableAssignment *) operation;
        operation_free (op->value);
        break;
    }
    case OPERATION_TYPE_IF: {
        OperationIf *op = (OperationIf *) operation;
        operation_free (op->condition);
        for (size_t i = 0; i < op->body_length; i++)
            operation_free (op->body[i]);
        free (op->body);
        break;
    }
    case OPERATION_TYPE_ELSE: {
        OperationElse *op = (OperationElse *) operation;
        for (size_t i = 0; i < op->body_length; i++)
            operation_free (op->body[i]);
        free (op->body);
        break;
    }
    case OPERATION_TYPE_WHILE: {
        OperationWhile *op = (OperationWhile *) operation;
        operation_free (op->condition);
        for (size_t i = 0; i < op->body_length; i++)
            operation_free (op->body[i]);
        free (op->body);
        break;
    }
    case OPERATION_TYPE_FUNCTION_DEFINITION: {
        OperationFunctionDefinition *op = (OperationFunctionDefinition *) operation;
        for (int i = 0; op->parameters[i] != NULL; i++)
            operation_free (op->parameters[i]);
        free (op->parameters);
        for (size_t i = 0; i < op->body_length; i++)
            operation_free (op->body[i]);
        free (op->body);
        break;
    }
    case OPERATION_TYPE_FUNCTION_CALL: {
        OperationFunctionCall *op = (OperationFunctionCall *) operation;
        for (int i = 0; op->parameters[i] != NULL; i++)
            operation_free (op->parameters[i]);
        free (op->parameters);
        break;
    }
    case OPERATION_TYPE_RETURN: {
        OperationReturn *op = (OperationReturn *) operation;
        operation_free (op->value);
        break;
    }
    case OPERATION_TYPE_MEMBER_VALUE: {
        OperationMemberValue *op = (OperationMemberValue *) operation;
        operation_free (op->object);
        break;
    }
    case OPERATION_TYPE_BINARY: {
        OperationBinary *op = (OperationBinary *) operation;
        operation_free (op->a);
        operation_free (op->b);
        break;
    }
    case OPERATION_TYPE_BOOLEAN_CONSTANT:
    case OPERATION_TYPE_NUMBER_CONSTANT:
    case OPERATION_TYPE_TEXT_CONSTANT:
    case OPERATION_TYPE_VARIABLE_VALUE:
        break;
    }

    free (operation);
}
