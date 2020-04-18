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
#include <stddef.h>
#include <stdint.h>

void str_free (char **value);

#define autofree_str __attribute__((cleanup(str_free))) char*

bool str_equal (const char *a, const char *b);

bool str_has_suffix (const char *value, const char *suffix);

char *str_slice (const char *value, int start, int end);

char *str_new (const char *value);

char *str_printf (const char *format, ...) __attribute__ ((format (printf, 1, 2)));

typedef struct {
    size_t allocated;
    size_t length;
    uint8_t *data;
} Bytes;

void bytes_free (Bytes **value);

#define autofree_bytes __attribute__((cleanup(bytes_free))) Bytes*

Bytes *bytes_new (size_t size);

void bytes_add (Bytes *bytes, uint8_t value);
