/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdint.h>

#include "utils.h"

void x86_64_mov (Bytes *buffer, int reg, uint32_t value);

void x86_64_syscall (Bytes *buffer);
