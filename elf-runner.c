/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "elf-runner.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

typedef enum {
    DATA_TYPE_BOOL,
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

    bool run_return;
    DataValue *return_value;
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
    case DATA_TYPE_BOOL:
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
data_value_new_bool (bool bool_value)
{
    DataValue *value = data_value_new (DATA_TYPE_BOOL);
    value->data[0] = bool_value ? 0xFF : 0x00;
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
data_value_new_utf8_sized (size_t length)
{
    DataValue *value = malloc (sizeof (DataValue));
    memset (value, 0, sizeof (DataValue));
    value->ref_count = 1;
    value->type = DATA_TYPE_UTF8;

    value->data_length = length;
    value->data = malloc (sizeof (char) * value->data_length);
    value->data[0] = '\0';

    return value;
}

static DataValue *
data_value_new_utf8 (const char *string_value)
{
    DataValue *value = data_value_new_utf8_sized (strlen (string_value) + 1);
    for (size_t i = 0; i < value->data_length; i++)
        value->data[i] = string_value[i];
    value->data[value->data_length] = '\0';

    return value;
}

static DataValue *
data_value_new_utf8_join (DataValue *a, DataValue *b)
{
    DataValue *value = data_value_new_utf8_sized ((a->data_length - 1) + (b->data_length - 1) + 1);
    value->data = malloc (sizeof (char) * value->data_length);
    for (size_t i = 0; i < a->data_length; i++)
        value->data[i] = a->data[i];
    for (size_t i = 0; i < b->data_length; i++)
        value->data[a->data_length - 1 + i] = b->data[i];
    value->data[value->data_length] = '\0';

    return value;
}

static bool
data_value_is_integer (DataValue *value)
{
    return value->type == DATA_TYPE_UINT8 ||
           value->type == DATA_TYPE_INT8 ||
           value->type == DATA_TYPE_UINT16 ||
           value->type == DATA_TYPE_INT16 ||
           value->type == DATA_TYPE_UINT32 ||
           value->type == DATA_TYPE_INT32 ||
           value->type == DATA_TYPE_UINT64 ||
           value->type == DATA_TYPE_INT64;
}

static bool
data_value_equal (DataValue *a, DataValue *b)
{
    if (a->type != b->type)
        return false;

    if (a->data_length != b->data_length)
        return false;

    for (size_t i = 0; i < a->data_length; i++)
        if (a->data[i] != b->data[i])
            return false;

    return true;
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
    variable->name = str_printf ("%s", name);
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
run_module (ProgramState *state, OperationModule *module)
{
    for (size_t i = 0; i < module->body_length && !state->run_return; i++) {
        DataValue *value = run_operation (state, module->body[i]);
        data_value_unref (value);
    }

    return NULL;
}

static DataValue *
run_function (ProgramState *state, OperationFunctionDefinition *function)
{
    for (size_t i = 0; i < function->body_length && !state->run_return; i++) {
        DataValue *value = run_operation (state, function->body[i]);
        data_value_unref (value);
    }

    DataValue *return_value = state->return_value;
    state->run_return = false;
    state->return_value = NULL;
    return return_value;
}

static DataValue *
make_default_value (ProgramState *state, Token *data_type)
{
    char *type_name = token_get_text (data_type, state->data);

    DataValue *result = NULL;
    if (str_equal (type_name, "bool"))
        result = data_value_new_bool (false);
    else if (str_equal (type_name, "uint8"))
        result = data_value_new_uint8 (0);
    else if (str_equal (type_name, "uint16"))
        result = data_value_new_uint16 (0);
    else if (str_equal (type_name, "uint32"))
        result = data_value_new_uint32 (0);
    else if (str_equal (type_name, "uint64"))
        result = data_value_new_uint64 (0);
    else if (str_equal (type_name, "utf8"))
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
        if (str_equal (variable->name, variable_name)) {
            data_value_unref (variable->value);
            variable->value = data_value_ref (value);
        }
    }

    free (variable_name);

    return NULL;
}

static DataValue *
run_if (ProgramState *state, OperationIf *operation)
{
    DataValue *value = run_operation (state, operation->condition);
    if (value->type != DATA_TYPE_BOOL) {
        data_value_unref (value);
        return NULL;
    }
    bool condition = value->data[0] != 0;
    data_value_unref (value);

    Operation **body = NULL;
    size_t body_length = 0;
    if (condition) {
        body = operation->body;
        body_length = operation->body_length;
    }
    else if (operation->else_operation != NULL) {
        body = operation->else_operation->body;
        body_length = operation->else_operation->body_length;
    }

    for (size_t i = 0; i < body_length && !state->run_return; i++) {
        DataValue *value = run_operation (state, body[i]);
        data_value_unref (value);
    }

    return NULL;
}

static DataValue *
run_while (ProgramState *state, OperationWhile *operation)
{
    while (true) {
        DataValue *value = run_operation (state, operation->condition);
        if (value->type != DATA_TYPE_BOOL) {
            data_value_unref (value);
            return NULL;
        }
        bool condition = value->data[0] != 0;
        data_value_unref (value);

        if (!condition)
            return NULL;

        for (size_t i = 0; i < operation->body_length && !state->run_return; i++) {
            DataValue *value = run_operation (state, operation->body[i]);
            data_value_unref (value);
        }
    }
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
    if (str_equal (function_name, "print")) {
        DataValue *value = run_operation (state, operation->parameters[0]);

        switch (value->type) {
        case DATA_TYPE_BOOL:
            if (value->data[0] == 0)
                printf ("false");
            else
                printf ("false");
            break;
        case DATA_TYPE_UINT8:
            printf ("%d", value->data[0]);
            break;
        case DATA_TYPE_UINT16:
            printf ("%d", value->data[0] << 8 | value->data[1]);
            break;
        case DATA_TYPE_UINT32:
            printf ("%d", value->data[3] << 24 | value->data[1] << 16 | value->data[1] << 8 || value->data[0]);
            break;
        case DATA_TYPE_UTF8:
            printf ("%.*s", (int) value->data_length, value->data);
            break;
        default:
            for (size_t i = 0; i < value->data_length; i++)
                printf ("%02X", value->data[i]);
            break;
        }
        printf ("\n");

        data_value_unref (value);
    }

    free (function_name);

    return result;
}

static DataValue *
run_return (ProgramState *state, OperationReturn *operation)
{
    DataValue *value = run_operation (state, operation->value);
    state->run_return = true;
    state->return_value = data_value_ref (value);
    return value;
}

static DataValue *
run_boolean_constant (ProgramState *state, OperationBooleanConstant *operation)
{
    bool value = token_parse_boolean_constant (operation->value, state->data);
    return data_value_new_bool (value);
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
        if (str_equal (variable->name, variable_name))
            return data_value_ref (variable->value);
    }

    printf ("variable value %s\n", variable_name);
    return NULL;
}

static DataValue *
run_member_value (ProgramState *state, OperationMemberValue *operation)
{
    DataValue *result = NULL;
    if (operation->object->type == OPERATION_TYPE_ACCESS_MODULE) {
        OperationAccessModule *op = (OperationAccessModule *) operation->object;
        if (token_has_text (op->module_name, state->data, "syscall")) {
            if (token_has_text (operation->member, state->data, ".exit")) {
                printf ("EXIT\n");
            }
        }
    } else {
        DataValue *object = run_operation (state, operation->object);

        if (object->type == DATA_TYPE_UTF8) {
            if (token_has_text (operation->member, state->data, ".length"))
                result = data_value_new_uint8 (object->data_length - 1); // FIXME: Total hack
            else if (token_has_text (operation->member, state->data, ".upper"))
                result = data_value_new_utf8 ("FOO"); // FIXME: Total hack
            else if (token_has_text (operation->member, state->data, ".lower"))
                result = data_value_new_utf8 ("foo"); // FIXME: Total hack
        }

        data_value_unref (object);
    }

    return result;
}

static DataValue *
run_binary_boolean (ProgramState *state, OperationBinary *operation, DataValue *a, DataValue *b)
{
    switch (operation->operator->type) {
    case TOKEN_TYPE_EQUAL:
        return data_value_new_bool (a->data[0] == b->data[0]);
    case TOKEN_TYPE_NOT_EQUAL:
        return data_value_new_bool (a->data[0] != b->data[0]);
    default:
        return NULL;
    }
}

static DataValue *
run_binary_integer (ProgramState *state, OperationBinary *operation, DataValue *a, DataValue *b)
{
    if (a->type != DATA_TYPE_UINT8 || b->type != DATA_TYPE_UINT8)
        return NULL;

    switch (operation->operator->type) {
    case TOKEN_TYPE_EQUAL:
        return data_value_new_bool (a->data[0] == b->data[0]);
    case TOKEN_TYPE_NOT_EQUAL:
        return data_value_new_bool (a->data[0] != b->data[0]);
    case TOKEN_TYPE_GREATER:
        return data_value_new_bool (a->data[0] > b->data[0]);
    case TOKEN_TYPE_GREATER_EQUAL:
        return data_value_new_bool (a->data[0] >= b->data[0]);
    case TOKEN_TYPE_LESS:
        return data_value_new_bool (a->data[0] < b->data[0]);
    case TOKEN_TYPE_LESS_EQUAL:
        return data_value_new_bool (a->data[0] <= b->data[0]);
    case TOKEN_TYPE_ADD:
        return data_value_new_uint8 (a->data[0] + b->data[0]);
    case TOKEN_TYPE_SUBTRACT:
        return data_value_new_uint8 (a->data[0] - b->data[0]);
    case TOKEN_TYPE_MULTIPLY:
        return data_value_new_uint8 (a->data[0] * b->data[0]);
    case TOKEN_TYPE_DIVIDE:
        return data_value_new_uint8 (a->data[0] / b->data[0]);
    default:
        return NULL;
    }
}

static DataValue *
run_binary_text (ProgramState *state, OperationBinary *operation, DataValue *a, DataValue *b)
{
    switch (operation->operator->type) {
    case TOKEN_TYPE_EQUAL:
        return data_value_new_bool (data_value_equal (a, b));
    case TOKEN_TYPE_NOT_EQUAL:
        return data_value_new_bool (!data_value_equal (a, b));
    case TOKEN_TYPE_ADD:
        return data_value_new_utf8_join (a, b);
    default:
        return NULL;
    }
}

static DataValue *
run_binary (ProgramState *state, OperationBinary *operation)
{
    DataValue *a = run_operation (state, operation->a);
    DataValue *b = run_operation (state, operation->b);

    // FIXME: Support string multiply "*" * 5 == "*****"
    DataValue *result;
    if (a->type == DATA_TYPE_BOOL && b->type == DATA_TYPE_BOOL)
        result = run_binary_boolean (state, operation, a, b);
    else if (data_value_is_integer (a) && data_value_is_integer (b))
        result = run_binary_integer (state, operation, a, b);
    else if (a->type == DATA_TYPE_UTF8 && b->type == DATA_TYPE_UTF8)
        result = run_binary_text (state, operation, a, b);

    data_value_unref (a);
    data_value_unref (b);

    return result;
}

static DataValue *
run_operation (ProgramState *state, Operation *operation)
{
    switch (operation->type) {
    case OPERATION_TYPE_MODULE:
        return run_module (state, (OperationModule *) operation);
    case OPERATION_TYPE_USE_MODULE:
        return NULL; // Resolved at compile time
    case OPERATION_TYPE_VARIABLE_DEFINITION:
        return run_variable_definition (state, (OperationVariableDefinition *) operation);
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT:
        return run_variable_assignment (state, (OperationVariableAssignment *) operation);
    case OPERATION_TYPE_IF:
        return run_if (state, (OperationIf *) operation);
    case OPERATION_TYPE_ELSE:
        return NULL; // Resolved in IF
    case OPERATION_TYPE_WHILE:
        return run_while (state, (OperationWhile *) operation);
    case OPERATION_TYPE_FUNCTION_DEFINITION:
        return NULL; // Resolved at compile time
    case OPERATION_TYPE_FUNCTION_CALL:
        return run_function_call (state, (OperationFunctionCall *) operation);
    case OPERATION_TYPE_RETURN:
        return run_return (state, (OperationReturn *) operation);
    case OPERATION_TYPE_BOOLEAN_CONSTANT:
        return run_boolean_constant (state, (OperationBooleanConstant *) operation);
    case OPERATION_TYPE_NUMBER_CONSTANT:
        return run_number_constant (state, (OperationNumberConstant *) operation);
    case OPERATION_TYPE_TEXT_CONSTANT:
        return run_text_constant (state, (OperationTextConstant *) operation);
    case OPERATION_TYPE_ACCESS_MODULE:
        return NULL; // Resolved at compile time
    case OPERATION_TYPE_VARIABLE_VALUE:
        return run_variable_value (state, (OperationVariableValue *) operation);
    case OPERATION_TYPE_MEMBER_VALUE:
        return run_member_value (state, (OperationMemberValue *) operation);
    case OPERATION_TYPE_BINARY:
        return run_binary (state, (OperationBinary *) operation);
    }

    return NULL;
}

void
elf_run (const char *data, OperationModule *module)
{
    ProgramState *state = malloc (sizeof (ProgramState));
    memset (state, 0, sizeof (ProgramState));
    state->data = data;

    DataValue *result = run_module (state, module);
    data_value_unref (result);

    for (size_t i = 0; i < state->variables_length; i++)
        variable_free (state->variables[i]);
    free (state->variables);
    free (state);
}
