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

char *
strdup_printf (const char *format, ...)
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
