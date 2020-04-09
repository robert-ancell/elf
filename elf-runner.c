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
    int ref_count;
    DataType type;
    uint8_t *data;
    size_t data_length;
} DataValue;

typedef struct {
    char *name;
    DataValue *value;
} Variable;

typedef struct {
    const char *data;

    Variable **variables;
    size_t variables_length;
} ProgramState;

static DataValue *run_operation (ProgramState *state, Operation *operation);

static DataValue *
data_value_new (DataType type)
{
    DataValue *value = malloc (sizeof (DataValue));
    memset (value, 0, sizeof (DataValue));
    value->ref_count = 1;
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
data_value_new_uint8 (uint8_t int_value)
{
    DataValue *value = data_value_new (DATA_TYPE_UINT8);
    value->data[0] = int_value;
    return value;
}

static DataValue *
data_value_new_uint16 (uint16_t int_value)
{
    DataValue *value = data_value_new (DATA_TYPE_UINT16);
    value->data[0] = (int_value >>  8) & 0xFF;
    value->data[1] = (int_value >>  0) & 0xFF;
    return value;
}

static DataValue *
data_value_new_uint32 (uint32_t int_value)
{
    DataValue *value = data_value_new (DATA_TYPE_UINT32);
    value->data[0] = (int_value >> 24) & 0xFF;
    value->data[1] = (int_value >> 16) & 0xFF;
    value->data[2] = (int_value >>  8) & 0xFF;
    value->data[3] = (int_value >>  0) & 0xFF;
    return value;
}

static DataValue *
data_value_new_uint64 (uint64_t int_value)
{
    DataValue *value = data_value_new (DATA_TYPE_UINT64);
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
data_value_new_utf8 (const char *string_value)
{
    DataValue *value = malloc (sizeof (DataValue));
    memset (value, 0, sizeof (DataValue));
    value->ref_count = 1;
    value->type = DATA_TYPE_UTF8;

    value->data_length = strlen (string_value) + 1;
    value->data = malloc (sizeof (char) * value->data_length);
    for (size_t i = 0; i < value->data_length; i++)
        value->data[i] = string_value[i];

    return value;
}

static DataValue *
data_value_ref (DataValue *value)
{
    if (value == NULL)
        return NULL;

    value->ref_count++;
    return value;
}

static void
data_value_unref (DataValue *value)
{
    if (value == NULL)
        return;

    value->ref_count--;
    if (value->ref_count > 0)
        return;

    free (value->data);
    free (value);
}

static Variable *
variable_new (const char *name, DataValue *value)
{
    Variable *variable = malloc (sizeof (Variable));
    memset (variable, 0, sizeof (Variable));
    variable->name = strdup_printf ("%s", name);
    variable->value = data_value_ref (value);

    return variable;
}

void
variable_free (Variable *variable)
{
    if (variable == NULL)
        return;

    free (variable->name);
    data_value_unref (variable->value);
    free (variable);
}

static DataValue *
run_function (ProgramState *state, OperationFunctionDefinition *function)
{
    for (int i = 0; function->body[i] != NULL; i++) {
        DataValue *value = run_operation (state, function->body[i]);
        if (function->body[i]->type == OPERATION_TYPE_RETURN) {
            // FIXME: Convert result to match return type
            return value;
        }

        data_value_unref (value);
    }

    return NULL;
}

static DataValue *
make_default_value (ProgramState *state, Token *data_type)
{
    char *type_name = token_get_text (data_type, state->data);

    DataValue *result = NULL;
    if (strcmp (type_name, "uint8") == 0)
        result = data_value_new_uint8 (0);
    else if (strcmp (type_name, "uint16") == 0)
        result = data_value_new_uint16 (0);
    else if (strcmp (type_name, "uint32") == 0)
        result = data_value_new_uint32 (0);
    else if (strcmp (type_name, "uint64") == 0)
        result = data_value_new_uint64 (0);
    else if (strcmp (type_name, "utf8") == 0)
        result = data_value_new_utf8 ("");

    if (result == NULL)
        printf ("default value (%s)\n", type_name);

    free (type_name);

    return result;
}

static void
add_variable (ProgramState *state, const char *name, DataValue *value)
{
    state->variables_length++;
    state->variables = realloc (state->variables, sizeof (Variable *) * state->variables_length);
    state->variables[state->variables_length - 1] = variable_new (name, value);
}

static DataValue *
run_variable_definition (ProgramState *state, OperationVariableDefinition *operation)
{
    char *variable_name = token_get_text (operation->name, state->data);

    DataValue *value = NULL;
    if (operation->value != NULL)
        value = run_operation (state, operation->value);
    else
        value = make_default_value (state, operation->data_type);

    if (value != NULL)
        add_variable (state, variable_name, value);

    free (variable_name);
    data_value_unref (value);

    return NULL;
}

static DataValue *
run_variable_assignment (ProgramState *state, OperationVariableAssignment *operation)
{
    char *variable_name = token_get_text (operation->name, state->data);

    DataValue *value = run_operation (state, operation->value);

    for (size_t i = 0; state->variables[i] != NULL; i++) {
        Variable *variable = state->variables[i];
        if (strcmp (variable->name, variable_name) == 0) {
            data_value_unref (variable->value);
            variable->value = data_value_ref (value);
        }
    }

    free (variable_name);

    return NULL;
}

static DataValue *
run_function_call (ProgramState *state, OperationFunctionCall *operation)
{
    if (operation->function != NULL) {
        // FIXME: Use a stack, these variables shouldn't remain after the call
        for (int i = 0; operation->parameters[i] != NULL; i++) {
            DataValue *value = run_operation (state, operation->parameters[i]);
            OperationVariableDefinition *parameter_definition = (OperationVariableDefinition *) operation->function->parameters[i];
            char *variable_name = token_get_text (parameter_definition->name, state->data);
            add_variable (state, variable_name, value);
            free (variable_name);
        }

        return run_function (state, operation->function);
    }

    char *function_name = token_get_text (operation->name, state->data);

    DataValue *result = NULL;
    if (strcmp (function_name, "print") == 0) {
        DataValue *value = run_operation (state, operation->parameters[0]);

        for (size_t i = 0; i < value->data_length; i++)
             printf ("%02X", value->data[i]);
        printf ("\n");

        data_value_unref (value);
    }

    free (function_name);

    return result;
}

static DataValue *
run_return (ProgramState *state, OperationReturn *operation)
{
    return run_operation (state, operation->value);
}

static DataValue *
run_number_constant (ProgramState *state, OperationNumberConstant *operation)
{
    uint64_t value = token_parse_number_constant (operation->value, state->data);
    // FIXME: Catch overflow (numbers > 64 bit not supported)

    if (value <= UINT8_MAX)
        return data_value_new_uint8 (value);
    else if (value <= UINT16_MAX)
        return data_value_new_uint16 (value);
    else if (value <= UINT32_MAX)
        return data_value_new_uint32 (value);
    else
        return data_value_new_uint64 (value);
}

static DataValue *
run_text_constant (ProgramState *state, OperationTextConstant *operation)
{
    char *value = token_parse_text_constant (operation->value, state->data);
    DataValue *result = data_value_new_utf8 (value);
    free (value);
    return result;
}

static DataValue *
run_variable_value (ProgramState *state, OperationVariableValue *operation)
{
    char *variable_name = token_get_text (operation->name, state->data);

    for (size_t i = 0; i < state->variables_length; i++) {
        Variable *variable = state->variables[i];
        if (strcmp (variable->name, variable_name) == 0)
            return data_value_ref (variable->value);
    }

    printf ("variable value %s\n", variable_name);
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

    DataValue *result = data_value_new_uint8 (int_value);
    data_value_unref (a);
    data_value_unref (b);

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
    case OPERATION_TYPE_FUNCTION_DEFINITION: {
        // All resolved at compile time
        return NULL;
    }
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

    DataValue *result = run_function (state, function);
    data_value_unref (result);

    for (size_t i = 0; i < state->variables_length; i++)
        variable_free (state->variables[i]);
    free (state->variables);
    free (state);
}
