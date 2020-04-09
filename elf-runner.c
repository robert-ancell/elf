#include "elf-runner.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

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
    const char *data;
    DataValue **variables;
} ProgramState;

static DataValue *run_operation (ProgramState *state, Operation *operation);

static DataValue *
data_value_new (DataType type, const char *name)
{
    DataValue *value = malloc (sizeof (DataValue));
    memset (value, 0, sizeof (DataValue));
    value->name = strdup_printf ("%s", name);
    value->type = type;

    switch (type) {
    case DATA_TYPE_UINT8:
    case DATA_TYPE_INT8:
        value->data_length = 1;
        break;
    case DATA_TYPE_UINT16:
    case DATA_TYPE_INT16:
        value->data_length = 2;
        break;
    case DATA_TYPE_UINT32:
    case DATA_TYPE_INT32:
        value->data_length = 4;
        break;
    case DATA_TYPE_UINT64:
    case DATA_TYPE_INT64:
        value->data_length = 8;
        break;
    case DATA_TYPE_UTF8:
        value->data_length = 1;
        break;
    }
    value->data = malloc (value->data_length);
    memset (value->data, 0, value->data_length);

    return value;
}

static DataValue *
data_value_new_uint8 (const char *name, uint8_t int_value)
{
    DataValue *value = data_value_new (DATA_TYPE_UINT8, name);
    value->data[0] = int_value;
    return value;
}

static DataValue *
data_value_new_uint16 (const char *name, uint16_t int_value)
{
    DataValue *value = data_value_new (DATA_TYPE_UINT16, name);
    value->data[0] = (int_value >>  8) & 0xFF;
    value->data[1] = (int_value >>  0) & 0xFF;
    return value;
}

static DataValue *
data_value_new_uint32 (const char *name, uint32_t int_value)
{
    DataValue *value = data_value_new (DATA_TYPE_UINT32, name);
    value->data[0] = (int_value >> 24) & 0xFF;
    value->data[1] = (int_value >> 16) & 0xFF;
    value->data[2] = (int_value >>  8) & 0xFF;
    value->data[3] = (int_value >>  0) & 0xFF;
    return value;
}

static DataValue *
data_value_new_uint64 (const char *name, uint64_t int_value)
{
    DataValue *value = data_value_new (DATA_TYPE_UINT64, name);
    value->data[0] = (int_value >> 56) & 0xFF;
    value->data[1] = (int_value >> 48) & 0xFF;
    value->data[2] = (int_value >> 40) & 0xFF;
    value->data[3] = (int_value >> 32) & 0xFF;
    value->data[4] = (int_value >> 24) & 0xFF;
    value->data[5] = (int_value >> 16) & 0xFF;
    value->data[6] = (int_value >>  8) & 0xFF;
    value->data[7] = (int_value >>  0) & 0xFF;
    return value;
}

static DataValue *
data_value_new_utf8 (const char *name, const char *string_value)
{
    DataValue *value = malloc (sizeof (DataValue));
    memset (value, 0, sizeof (DataValue));
    value->name = strdup_printf ("%s", name);
    value->type = DATA_TYPE_UTF8;

    value->data_length = strlen (string_value) + 1;
    value->data = malloc (sizeof (char) * value->data_length);
    for (size_t i = 0; i < value->data_length; i++)
        value->data[i] = string_value[i];

    return value;
}

void
data_value_free (DataValue *value)
{
    if (value == NULL)
        return;

    free (value->name);
    free (value->data);
    free (value);
}

static DataValue *
run_function (ProgramState *state, OperationFunctionDefinition *function)
{
    for (int i = 0; function->body[i] != NULL; i++) {
        run_operation (state, function->body[i]);
    }
    return NULL;
}

static DataValue *
run_variable_definition (ProgramState *state, OperationVariableDefinition *operation)
{
    char *variable_name = token_get_text (operation->name, state->data);
    printf ("define variable %s\n", variable_name);
    free (variable_name);

    return NULL;
}

static DataValue *
run_variable_assignment (ProgramState *state, OperationVariableAssignment *operation)
{
    char *variable_name = token_get_text (operation->name, state->data);
    printf ("assign variable %s\n", variable_name);
    free (variable_name);

    return NULL;
}

static DataValue *
run_function_call (ProgramState *state, OperationFunctionCall *operation)
{
    char *function_name = token_get_text (operation->name, state->data);

    if (strcmp (function_name, "print") == 0) {
        DataValue *value = run_operation (state, operation->parameters[0]);

        for (size_t i = 0; i < value->data_length; i++)
             printf ("%02X", value->data[i]);
        printf ("\n");

        data_value_free (value);
    }
    else
        printf ("call function %s\n", function_name);

    free (function_name);

    return NULL;
}

static DataValue *
run_return (ProgramState *state, OperationReturn *operation)
{
    printf ("return\n");
    return NULL;
}

static DataValue *
run_number_constant (ProgramState *state, OperationNumberConstant *operation)
{
    uint64_t value = 0;

    for (size_t i = 0; i < operation->value->length; i++)
        value = value * 10 + state->data[operation->value->offset + i] - '0';
    // FIXME: Catch overflow (numbers > 64 bit not supported)

    if (value <= UINT8_MAX)
        return data_value_new_uint8 (NULL, value);
    else if (value <= UINT16_MAX)
        return data_value_new_uint16 (NULL, value);
    else if (value <= UINT32_MAX)
        return data_value_new_uint32 (NULL, value);
    else
        return data_value_new_uint64 (NULL, value);
}

static DataValue *
run_text_constant (ProgramState *state, OperationTextConstant *operation)
{
    printf ("text constant\n");
    return data_value_new_utf8 (NULL, "TEST"); // FIXME
}

static DataValue *
run_variable_value (ProgramState *state, OperationVariableValue *operation)
{
    printf ("variable value\n");
    return NULL;
}

static DataValue *
run_binary (ProgramState *state, OperationBinary *operation)
{
    DataValue *a = run_operation (state, operation->a);
    DataValue *b = run_operation (state, operation->b);

    if (a->type != DATA_TYPE_UINT8 || b->type != DATA_TYPE_UINT8)
        return NULL;

    uint8_t int_value = 0;
    switch (operation->operator->type) {
    case TOKEN_TYPE_ADD:
        int_value = a->data[0] + b->data[0];
        break;
    case TOKEN_TYPE_SUBTRACT:
        int_value = a->data[0] - b->data[0];
        break;
    case TOKEN_TYPE_MULTIPLY:
        int_value = a->data[0] * b->data[0];
        break;
    case TOKEN_TYPE_DIVIDE:
        int_value = a->data[0] / b->data[0];
        break;
    default:
        break;
    }

    DataValue *result = data_value_new_uint8 (NULL, int_value);
    data_value_free (a);
    data_value_free (b);

    return result;
}

static DataValue *
run_operation (ProgramState *state, Operation *operation)
{
    switch (operation->type) {
    case OPERATION_TYPE_VARIABLE_DEFINITION:
        return run_variable_definition (state, (OperationVariableDefinition *) operation);
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT:
        return run_variable_assignment (state, (OperationVariableAssignment *) operation);
    case OPERATION_TYPE_FUNCTION_DEFINITION:
        // Nothing to do unless called
        return NULL;
    case OPERATION_TYPE_FUNCTION_CALL:
        return run_function_call (state, (OperationFunctionCall *) operation);
    case OPERATION_TYPE_RETURN:
        return run_return (state, (OperationReturn *) operation);
    case OPERATION_TYPE_NUMBER_CONSTANT:
        return run_number_constant (state, (OperationNumberConstant *) operation);
    case OPERATION_TYPE_TEXT_CONSTANT:
        return run_text_constant (state, (OperationTextConstant *) operation);
    case OPERATION_TYPE_VARIABLE_VALUE:
        return run_variable_value (state, (OperationVariableValue *) operation);
    case OPERATION_TYPE_BINARY:
        return run_binary (state, (OperationBinary *) operation);
    }

    return NULL;
}

void
elf_run (const char *data, OperationFunctionDefinition *function)
{
    ProgramState *state = malloc (sizeof (ProgramState));
    memset (state, 0, sizeof (ProgramState));
    state->data = data;
    state->variables = malloc (sizeof (DataValue *));
    state->variables[0] = NULL;

    DataValue *result = run_function (state, function);
    data_value_free (result);

    free (state->variables);
    free (state);
}
