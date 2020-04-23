/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdbool.h>

#include "elf-token.h"

typedef enum {
    OPERATION_TYPE_MODULE,
    OPERATION_TYPE_VARIABLE_DEFINITION,
    OPERATION_TYPE_VARIABLE_ASSIGNMENT,
    OPERATION_TYPE_IF,
    OPERATION_TYPE_ELSE,
    OPERATION_TYPE_WHILE,
    OPERATION_TYPE_FUNCTION_DEFINITION,
    OPERATION_TYPE_FUNCTION_CALL,
    OPERATION_TYPE_RETURN,
    OPERATION_TYPE_BOOLEAN_CONSTANT,
    OPERATION_TYPE_NUMBER_CONSTANT,
    OPERATION_TYPE_TEXT_CONSTANT,
    OPERATION_TYPE_VARIABLE_VALUE,
    OPERATION_TYPE_MEMBER_VALUE,
    OPERATION_TYPE_BINARY,
} OperationType;

typedef struct {
    OperationType type;
} Operation;

typedef struct {
    OperationType type; // OPERATION_TYPE_MODULE

    Operation **body;
    size_t body_length;
} OperationModule;

typedef struct {
    OperationType type; // OPERATION_TYPE_VARIABLE_DEFINITION

    Token *data_type;
    Token *name;
    Operation *value;
} OperationVariableDefinition;

typedef struct {
    OperationType type; // OPERATION_TYPE_VARIABLE_ASSIGNMENT

    Token *name;
    Operation *value;

    OperationVariableDefinition *variable;
} OperationVariableAssignment;

typedef struct _OperationElse OperationElse;

typedef struct {
    OperationType type; // OPERATION_TYPE_IF

    Operation *condition;
    Operation **body;
    size_t body_length;
    OperationElse *else_operation;
} OperationIf;

struct _OperationElse {
    OperationType type; // OPERATION_TYPE_ELSE

    Operation **body;
    size_t body_length;
};

typedef struct {
    OperationType type; // OPERATION_TYPE_WHILE

    Operation *condition;
    Operation **body;
    size_t body_length;
} OperationWhile;

typedef struct _OperationFunctionDefinition OperationFunctionDefinition;

struct _OperationFunctionDefinition {
    OperationType type; // OPERATION_TYPE_FUNCTION_DEFINITION

    OperationFunctionDefinition *parent;
    Token *data_type;
    Token *name;
    Operation **parameters;
    Operation **body;
    size_t body_length;
};

typedef struct {
    OperationType type; // OPERATION_TYPE_FUNCTION_CALL

    Token *name;
    Operation **parameters;

    OperationFunctionDefinition *function;
} OperationFunctionCall;

typedef struct {
    OperationType type; // OPERATION_TYPE_RETURN

    Token *name;
    Operation *value;

    OperationFunctionDefinition *function;
} OperationReturn;

typedef struct {
    OperationType type; // OPERATION_TYPE_BOOLEAN_CONSTANT;

    Token *value;
} OperationBooleanConstant;

typedef struct {
    OperationType type; // OPERATION_TYPE_NUMBER_CONSTANT;

    Token *value;
} OperationNumberConstant;

typedef struct {
    OperationType type; // OPERATION_TYPE_TEXT_CONSTANT;

    Token *value;
} OperationTextConstant;

typedef struct {
    OperationType type; // OPERATION_TYPE_VARIABLE_VALUE;

    Token *name;

    OperationVariableDefinition *variable;
} OperationVariableValue;

typedef struct {
    OperationType type; // OPERATION_TYPE_MEMBER_VALUE;

    Operation *object;
    Token *member;
} OperationMemberValue;

typedef struct {
    OperationType type; // OPERATION_TYPE_BINARY

    Token *operator;
    Operation *a;
    Operation *b;
} OperationBinary;

Operation *make_module (void);

Operation *make_variable_definition (Token *data_type, Token *name, Operation *value);

Operation *make_variable_assignment (Token *name, Operation *value, OperationVariableDefinition *variable);

Operation *make_if (Operation *condition);

Operation *make_else (void);

Operation *make_while (Operation *condition);

Operation *make_function_definition (Token *data_type, Token *name, Operation **parameters);

Operation *make_function_call (Token *name, Operation **parameters, OperationFunctionDefinition *function);

Operation *make_return (Operation *value, OperationFunctionDefinition *function);

Operation *make_boolean_constant (Token *value);

Operation *make_number_constant (Token *value);

Operation *make_text_constant (Token *value);

Operation *make_member_value (Operation *object, Token *member);

Operation *make_variable_value (Token *name, OperationVariableDefinition *variable);

Operation *make_binary (Token *operator, Operation *a, Operation *b);

bool operation_is_constant (Operation *operation);

char *operation_get_data_type (Operation *operation, const char *data);

void operation_add_child (Operation *operation, Operation *child);

size_t operation_get_n_children (Operation *operation);

Operation *operation_get_child (Operation *operation, size_t index);

Operation *operation_get_last_child (Operation *operation);

char *operation_to_string (Operation *operation);

void operation_free (Operation *operation);
