/*
 * Copyright (C) 2020 Robert Ancell.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
str_free (char **value)
{
    free (*value);
    *value = NULL;
}

bool
str_has_suffix (const char *value, const char *suffix)
{
    size_t value_length = strlen (value);
    size_t suffix_length = strlen (suffix);

    if (suffix_length > value_length)
        return false;

    return strcmp (value + value_length - suffix_length, suffix) == 0;
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
