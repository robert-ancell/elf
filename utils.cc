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

void str_free(char **value) {
  free(*value);
  *value = NULL;
}

bool str_equal(const char *a, const char *b) {
  int i = 0;
  while (a[i] != '\0' && b[i] != '\0') {
    if (a[i] != b[i])
      return false;
    i++;
  }

  return a[i] == b[i];
}

bool str_has_suffix(const char *value, const char *suffix) {
  size_t value_length = strlen(value);
  size_t suffix_length = strlen(suffix);

  if (suffix_length > value_length)
    return false;

  return str_equal(value + value_length - suffix_length, suffix);
}

char *str_slice(const char *value, size_t start, size_t end) {
  size_t value_length = strlen(value);
  if (start < 0)
    start += value_length;
  if (end <= 0)
    end += value_length;

  if (start < 0)
    start = 0;
  if (start > value_length)
    start = value_length;
  if (end > value_length)
    end = value_length;

  if (end < start)
    end = start;

  size_t result_length = end - start;
  char *result =
      static_cast<char *>(malloc(sizeof(char) * (result_length + 1)));
  for (size_t i = 0; i < result_length; i++)
    result[i] = value[start + i];
  result[result_length] = '\0';

  return result;
}

char *str_new(const char *value) {
  size_t value_length = strlen(value) + 1;
  char *output = static_cast<char *>(malloc(sizeof(char) * value_length));
  memcpy(output, value, value_length);
  return output;
}

char *str_printf(const char *format, ...) {
  va_list ap;

  va_start(ap, format);
  char c;
  int length = vsnprintf(&c, 1, format, ap);
  va_end(ap);

  char *output = static_cast<char *>(malloc(sizeof(char) * (length + 1)));

  va_start(ap, format);
  vsprintf(output, format, ap);
  va_end(ap);

  return output;
}

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

uint32_t utf8_decode(const uint8_t *data, size_t data_length, size_t *offset) {
  if (*offset >= data_length)
    return UTF8_INVALID_CHAR;

  uint8_t c = data[*offset];
  (*offset)++;
  if (c < (1 << 7))
    return c;

  size_t byte_count;
  if ((c & 0xE0) == 0xC0) {
    c = c & 0x1F;
    byte_count = 1;
  } else if ((c & 0xF0) == 0xE0) {
    c = c & 0x0F;
    byte_count = 2;
  } else if ((c & 0xF8) == 0xF0) {
    c = c & 0x07;
    byte_count = 3;
  } else
    return UTF8_INVALID_CHAR;

  if (*offset + byte_count > data_length)
    return UTF8_INVALID_CHAR;

  for (size_t i = 0; i < byte_count; i++) {
    uint8_t b = data[*offset];
    (*offset)++;

    if ((b & 0xC0) != 0x80)
      return UTF8_INVALID_CHAR;

    c = c << 6 | (b & 0x3F);
  }

  return c;
}

void utf8_encode(Bytes *bytes, uint32_t c) {
  if (c < (1 << 7)) {
    bytes_add(bytes, c);
  } else if (c < (1 << 11)) {
    bytes_add(bytes, 0xC0 | (c >> 6));
    bytes_add(bytes, 0x80 | ((c >> 0) & 0x3F));
  } else if (c < (1 << 16)) {
    bytes_add(bytes, 0xE0 | (c >> 12));
    bytes_add(bytes, 0x80 | ((c >> 6) & 0x3F));
    bytes_add(bytes, 0x80 | ((c >> 0) & 0x3F));
  } else if (c < (1 << 21)) {
    bytes_add(bytes, 0xF0 | (c >> 18));
    bytes_add(bytes, 0x80 | ((c >> 12) & 0x3F));
    bytes_add(bytes, 0x80 | ((c >> 6) & 0x3F));
    bytes_add(bytes, 0x80 | ((c >> 0) & 0x3F));
  } else {
    bytes_add(bytes, '?');
  }
}
