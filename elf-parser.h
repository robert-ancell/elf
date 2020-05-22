/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <memory>

#include "elf-operation.h"
#include "elf-token.h"

std::shared_ptr<OperationModule> elf_parse(const char *data,
                                           size_t data_length);
