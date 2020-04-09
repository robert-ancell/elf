#include "elf-runner.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    DATA_TYPE_UINT8,
    DATA_TYPE_INT8,
    DATA_TYPE_UINT16,
    DATA_TYPE_INT16,
    DATA_TYPE_UINT32,
    DATA_TYPE_INT32,
    DATA_TYPE_UINT64,
    DATA_TYPE_INT64,
    DATA_TYPE_UTF8,
} DataType;

typedef struct {
    DataType type;
    char *name;
    uint8_t *data;
    size_t data_length;
} DataValue;

typedef struct {
    DataValue **variables;
} ProgramState;

static void run_operation (ProgramState *state, Operation *operation);

static void
run_function (ProgramState *state, OperationFunctionDefinition *function)
{
    for (int i = 0; function->body[i] != NULL; i++) {
        run_operation (state, function->body[i]);
    }
}

static void
run_variable_definition (ProgramState *state, OperationVariableDefinition *operation)
{
    printf ("define var\n");
}

static void
run_variable_assignment (ProgramState *state, OperationVariableAssignment *operation)
{
    printf ("assign var\n");
}

static void
run_function_call (ProgramState *state, OperationFunctionCall *operation)
{
    printf ("call function\n");
}

static void
run_return (ProgramState *state, OperationReturn *operation)
{
    printf ("return\n");
}

static void
run_number_constant (ProgramState *state, OperationNumberConstant *operation)
{
}

static void
run_text_constant (ProgramState *state, OperationTextConstant *operation)
{
}

static void
run_variable_value (ProgramState *state, OperationVariableValue *operation)
{
}

static void
run_operation (ProgramState *state, Operation *operation)
{
    switch (operation->type) {
    case OPERATION_TYPE_VARIABLE_DEFINITION:
        run_variable_definition (state, (OperationVariableDefinition *) operation);
        break;
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT:
        run_variable_assignment (state, (OperationVariableAssignment *) operation);
        break;
    case OPERATION_TYPE_FUNCTION_DEFINITION:
        // Nothing to do unless called
        break;
    case OPERATION_TYPE_FUNCTION_CALL:
        run_function_call (state, (OperationFunctionCall *) operation);
        break;
    case OPERATION_TYPE_RETURN:
        run_return (state, (OperationReturn *) operation);
        break;
    case OPERATION_TYPE_NUMBER_CONSTANT:
        run_number_constant (state, (OperationNumberConstant *) operation);
        break;
    case OPERATION_TYPE_TEXT_CONSTANT:
        run_text_constant (state, (OperationTextConstant *) operation);
        break;
    case OPERATION_TYPE_VARIABLE_VALUE:
        run_variable_value (state, (OperationVariableValue *) operation);
        break;
    case OPERATION_TYPE_BINARY:
        break;
    }
}

void
elf_run (OperationFunctionDefinition *function)
{
    ProgramState *state = malloc (sizeof (ProgramState));
    memset (state, 0, sizeof (ProgramState));
    state->variables = malloc (sizeof (DataValue *));
    state->variables[0] = NULL;

    run_function (state, function);
}
