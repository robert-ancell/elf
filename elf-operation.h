#pragma once

#include "elf-token.h"

typedef enum {
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
} OperationVariableValue;

typedef struct {
    OperationType type; // OPERATION_TYPE_BINARY

    Token *operator;
    Operation *a;
    Operation *b;
} OperationBinary;

Operation *make_variable_definition (Token *data_type, Token *name, Operation *value);

Operation *make_variable_assignment (Token *name, Operation *value);

Operation *make_if (Operation *condition);

Operation *make_else (void);

Operation *make_while (Operation *condition);

Operation *make_function_definition (Token *data_type, Token *name, Operation **parameters);

Operation *make_function_call (Token *name, Operation **parameters, OperationFunctionDefinition *function);

Operation *make_return (Operation *value);

Operation *make_boolean_constant (Token *value);

Operation *make_number_constant (Token *value);

Operation *make_text_constant (Token *value);

Operation *make_variable_value (Token *name);

Operation *make_binary (Token *operator, Operation *a, Operation *b);

void operation_add_child (Operation *operation, Operation *child);

Operation *operation_get_last_child (Operation *operation);

char *operation_to_string (Operation *operation);

void operation_free (Operation *operation);
