#pragma once

#include <stdlib.h>

typedef enum {
    TOKEN_TYPE_WORD,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_TEXT,
    TOKEN_TYPE_ASSIGN,
    TOKEN_TYPE_ADD,
    TOKEN_TYPE_SUBTRACT,
    TOKEN_TYPE_MULTIPLY,
    TOKEN_TYPE_DIVIDE,
    TOKEN_TYPE_OPEN_PAREN,
    TOKEN_TYPE_CLOSE_PAREN,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_OPEN_BRACE,
    TOKEN_TYPE_CLOSE_BRACE,
} TokenType;

typedef struct {
    TokenType type;
    size_t offset;
    size_t length;
} Token;

typedef enum {
    OPERATION_TYPE_VARIABLE_DEFINITION,
    OPERATION_TYPE_VARIABLE_ASSIGNMENT,
    OPERATION_TYPE_FUNCTION_DEFINE,
    OPERATION_TYPE_FUNCTION_CALL,
    OPERATION_TYPE_RETURN,
    OPERATION_TYPE_NUMBER_CONSTANT,
    OPERATION_TYPE_TEXT_CONSTANT,
    OPERATION_TYPE_VARIABLE_VALUE,
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

OperationFunctionDefinition *elf_parse (const char *data, size_t data_length);

char *operation_to_string (Operation *operation);

void operation_free (Operation *operation);
