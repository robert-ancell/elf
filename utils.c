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
