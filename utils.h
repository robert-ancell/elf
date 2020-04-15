/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdbool.h>

void str_free (char **value);

#define autofree_str __attribute__((cleanup(str_free))) char*

bool str_has_suffix (const char *value, const char *suffix);

char *str_slice (const char *value, int start, int end);

char *str_new (const char *value);

char *str_printf (const char *format, ...) __attribute__ ((format (printf, 1, 2)));
