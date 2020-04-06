#pragma once

#include <stdlib.h>

#include "elf-token.h"
#include "elf-operation.h"

OperationFunctionDefinition *elf_parse (const char *data, size_t data_length);
