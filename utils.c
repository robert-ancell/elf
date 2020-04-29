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

void
str_free (char **value)
{
    free (*value);
    *value = NULL;
}

bool
str_equal (const char *a, const char *b)
{
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i])
            return false;
        i++;
    }

    return a[i] == b[i];
}

bool
str_has_suffix (const char *value, const char *suffix)
{
    size_t value_length = strlen (value);
    size_t suffix_length = strlen (suffix);

    if (suffix_length > value_length)
        return false;

    return str_equal (value + value_length - suffix_length, suffix);
}

char *
str_slice (const char *value, int start, int end)
{
    size_t value_length = strlen (value);
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
    char *result = malloc (sizeof (char) * (result_length + 1));
    for (int i = 0; i < result_length; i++)
        result[i] = value[start + i];
    result[result_length] = '\0';

    return result;
}

char *
str_new (const char *value)
{
    size_t value_length = strlen (value) + 1;
    char *output = malloc (sizeof (char) * value_length);
    memcpy (output, value, value_length);
    return output;
}

char *
str_printf (const char *format, ...)
{
    va_list ap;

    va_start (ap, format);
    char c;
    int length = vsnprintf (&c, 1, format, ap);
    va_end (ap);

    char *output = malloc (sizeof (char) * (length + 1));

    va_start (ap, format);
    vsprintf (output, format, ap);
    va_end (ap);

    return output;
}

void
bytes_free (Bytes **value)
{
    if (*value == NULL)
        return;

    free ((*value)->data);
    free (*value);
    *value = NULL;
}

Bytes *
bytes_new (size_t size)
{
    Bytes *bytes = malloc (sizeof (Bytes));
    bytes->allocated = size;
    bytes->length = 0;
    bytes->data = malloc (sizeof (uint8_t) * size);

    return bytes;
}

void
bytes_resize (Bytes *bytes, size_t size)
{
    bytes->allocated = size;
    bytes->data = realloc (bytes->data, bytes->allocated);
    if (bytes->length > bytes->allocated)
        bytes->length = bytes->allocated;
}

void
bytes_add (Bytes *bytes, uint8_t value)
{
    bytes->length++;
    if (bytes->length > bytes->allocated)
        bytes_resize (bytes, bytes->allocated == 0 ? 16 : bytes->allocated * 2);
    bytes->data[bytes->length - 1] = value;
}

void
bytes_trim (Bytes *bytes)
{
    if (bytes->length >= bytes->allocated)
        return;

    bytes->data = realloc (bytes->data, bytes->length);
    bytes->allocated = bytes->length;
}

bool
bytes_equal (Bytes *a, Bytes *b)
{
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

Bytes *
readall (int fd)
{
    Bytes *buffer = bytes_new (1024);
    while (true) {
        // Make space to read
        if (buffer->length >= buffer->allocated)
            bytes_resize (buffer, buffer->allocated * 2);

        ssize_t n_read = read (fd, buffer->data + buffer->length, buffer->allocated - buffer->length);
        if (n_read < 0) {
            bytes_free (&buffer);
            return NULL;
        }

        if (n_read == 0)
            break;
        buffer->length += n_read;
    }
    bytes_trim (buffer);

    return buffer;
}

Bytes *
file_readall (const char *pathname)
{
    int fd = open (pathname, O_RDONLY);
    if (fd < 0)
        return NULL;

    Bytes *data = readall (fd);
    close (fd);

    return data;
}
