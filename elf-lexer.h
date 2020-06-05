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
#include <vector>

#include "elf-token.h"

std::vector<std::shared_ptr<Token>> elf_lex(const char *data,
                                            size_t data_length);
