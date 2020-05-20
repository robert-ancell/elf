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

typedef struct {
  size_t allocated;
  size_t length;
  uint8_t *data;
} Bytes;

void bytes_free(Bytes **value);

#define autofree_bytes __attribute__((cleanup(bytes_free))) Bytes *

Bytes *bytes_new(size_t size);

void bytes_resize(Bytes *bytes, size_t size);

void bytes_add(Bytes *bytes, uint8_t value);

void bytes_trim(Bytes *bytes);

bool bytes_equal(Bytes *a, Bytes *b);

uint8_t *bytes_steal(Bytes *bytes);

Bytes *readall(int fd);

Bytes *file_readall(const char *pathname);
