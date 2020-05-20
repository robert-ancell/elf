/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "utils.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void bytes_free(Bytes **value) {
  if (*value == NULL)
    return;

  free((*value)->data);
  delete *value;
  *value = NULL;
}

Bytes *bytes_new(size_t size) {
  Bytes *bytes = new Bytes;
  bytes->allocated = size;
  bytes->length = 0;
  bytes->data = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * size));

  return bytes;
}

void bytes_resize(Bytes *bytes, size_t size) {
  bytes->allocated = size;
  bytes->data = static_cast<uint8_t *>(realloc(bytes->data, bytes->allocated));
  if (bytes->length > bytes->allocated)
    bytes->length = bytes->allocated;
}

void bytes_add(Bytes *bytes, uint8_t value) {
  bytes->length++;
  if (bytes->length > bytes->allocated)
    bytes_resize(bytes, bytes->allocated == 0 ? 16 : bytes->allocated * 2);
  bytes->data[bytes->length - 1] = value;
}

void bytes_trim(Bytes *bytes) {
  if (bytes->length >= bytes->allocated)
    return;

  bytes->data = static_cast<uint8_t *>(realloc(bytes->data, bytes->length));
  bytes->allocated = bytes->length;
}

bool bytes_equal(Bytes *a, Bytes *b) {
  size_t a_length = a == NULL ? 0 : a->length;
  size_t b_length = b == NULL ? 0 : b->length;

  if (a_length != b_length)
    return false;

  for (size_t i = 0; i < a_length; i++) {
    if (a->data[i] != b->data[i])
      return false;
  }

  return true;
}

uint8_t *bytes_steal(Bytes *bytes) {
  uint8_t *value = bytes->data;
  bytes->data = NULL;
  return value;
}

Bytes *readall(int fd) {
  Bytes *buffer = bytes_new(1024);
  while (true) {
    // Make space to read
    if (buffer->length >= buffer->allocated)
      bytes_resize(buffer, buffer->allocated * 2);

    ssize_t n_read = read(fd, buffer->data + buffer->length,
                          buffer->allocated - buffer->length);
    if (n_read < 0) {
      bytes_free(&buffer);
      return NULL;
    }

    if (n_read == 0)
      break;
    buffer->length += n_read;
  }
  bytes_trim(buffer);

  return buffer;
}

Bytes *file_readall(const char *pathname) {
  int fd = open(pathname, O_RDONLY);
  if (fd < 0) {
    printf("Failed to open %s", pathname);
    return NULL;
  }

  Bytes *data = readall(fd);
  close(fd);

  return data;
}
