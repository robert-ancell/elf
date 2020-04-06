#include "elf-operation.h"

#include <string.h>

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
make_variable_assignment (Token *name, Operation *value)
{
    OperationVariableAssignment *o = malloc (sizeof (OperationVariableAssignment));
    memset (o, 0, sizeof (OperationVariableAssignment));
    o->type = OPERATION_TYPE_VARIABLE_ASSIGNMENT;
    o->name = name;
    o->value = value;
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
make_function_call (Token *name, Operation **parameters)
{
    OperationFunctionCall *o = malloc (sizeof (OperationFunctionCall));
    memset (o, 0, sizeof (OperationFunctionCall));
    o->type = OPERATION_TYPE_FUNCTION_CALL;
    o->name = name;
    o->parameters = parameters;
    return (Operation *) o;
}

Operation *
make_return (Operation *value)
{
    OperationReturn *o = malloc (sizeof (OperationReturn));
    memset (o, 0, sizeof (OperationReturn));
    o->type = OPERATION_TYPE_RETURN;
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
make_variable_value (Token *name)
{
    OperationVariableValue *o = malloc (sizeof (OperationVariableValue));
    memset (o, 0, sizeof (OperationVariableValue));
    o->type = OPERATION_TYPE_VARIABLE_VALUE;
    o->name = name;
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

char *
operation_to_string (Operation *operation)
{
    switch (operation->type) {
    case OPERATION_TYPE_VARIABLE_DEFINITION:
        return strdup ("VARIABLE_DEFINITION");
    case OPERATION_TYPE_VARIABLE_ASSIGNMENT:
        return strdup ("VARIABLE_ASSIGNMENT");
    case OPERATION_TYPE_FUNCTION_DEFINITION:
        return strdup ("FUNCTION_DEFINITION");
    case OPERATION_TYPE_FUNCTION_CALL:
        return strdup ("FUNCTION_CALL");
    case OPERATION_TYPE_RETURN:
        return strdup ("RETURN");
    case OPERATION_TYPE_NUMBER_CONSTANT:
        return strdup ("NUMBER_CONSTANT");
    case OPERATION_TYPE_TEXT_CONSTANT:
        return strdup ("TEXT_CONSTANT");
    case OPERATION_TYPE_VARIABLE_VALUE:
        return strdup ("VARIABLE_VALUE");
    case OPERATION_TYPE_BINARY:
        return strdup ("BINARY");
    }

    return strdup ("UNKNOWN");
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
    case OPERATION_TYPE_FUNCTION_DEFINITION: {
        OperationFunctionDefinition *op = (OperationFunctionDefinition *) operation;
        for (int i = 0; op->parameters[i] != NULL; i++)
            operation_free (op->parameters[i]);
        free (op->parameters);
        for (int i = 0; op->body[i] != NULL; i++)
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
    case OPERATION_TYPE_BINARY: {
        OperationBinary *op = (OperationBinary *) operation;
        operation_free (op->a);
        operation_free (op->b);
        break;
    }
    case OPERATION_TYPE_NUMBER_CONSTANT:
    case OPERATION_TYPE_TEXT_CONSTANT:
    case OPERATION_TYPE_VARIABLE_VALUE:
        break;
    }

    free (operation);
}
