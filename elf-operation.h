#pragma once

#include "elf-token.h"

typedef enum {
    OPERATION_TYPE_VARIABLE_DEFINITION,
    OPERATION_TYPE_VARIABLE_ASSIGNMENT,
    OPERATION_TYPE_FUNCTION_DEFINITION,
    OPERATION_TYPE_FUNCTION_CALL,
    OPERATION_TYPE_RETURN,
    OPERATION_TYPE_NUMBER_CONSTANT,
    OPERATION_TYPE_TEXT_CONSTANT,
    OPERATION_TYPE_VARIABLE_VALUE,
    OPERATION_TYPE_BINARY,
} OperationType;

typedef struct {
    OperationType type;
} Operation;

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
} OperationVariableAssignment;

typedef struct _OperationFunctionDefinition OperationFunctionDefinition;

struct _OperationFunctionDefinition {
    OperationType type; // OPERATION_TYPE_FUNCTION_DEFINITION

    OperationFunctionDefinition *parent;
    Token *data_type;
    Token *name;
    Operation **parameters;
    Operation **body;
};

typedef struct {
    OperationType type; // OPERATION_TYPE_FUNCTION_CALL

    Token *name;
    Operation **parameters;
} OperationFunctionCall;

typedef struct {
    OperationType type; // OPERATION_TYPE_RETURN

    Token *name;
    Operation *value;
} OperationReturn;

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
} OperationVariableValue;

typedef struct {
    OperationType type; // OPERATION_TYPE_BINARY

    Token *operator;
    Operation *a;
    Operation *b;
} OperationBinary;

Operation *make_variable_definition (Token *data_type, Token *name, Operation *value);

Operation *make_variable_assignment (Token *name, Operation *value);

Operation *make_function_definition (Token *data_type, Token *name, Operation **parameters);

Operation *make_function_call (Token *name, Operation **parameters);

Operation *make_return (Operation *value);

Operation *make_number_constant (Token *value);

Operation *make_text_constant (Token *value);

Operation *make_variable_value (Token *name);

Operation *make_binary (Token *operator, Operation *a, Operation *b);

char *operation_to_string (Operation *operation);

void operation_free (Operation *operation);
