//
// Created by LJChi on 2021/3/28.
//
#include <stdarg.h>
#include "error.h"

void DoFatal(const char *format, ...) {
    va_list ap;

    fprintf(stderr, "fatal error:");
    va_start(ap, format);
    vprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    exit(-1);
}

void DoError(const char *format, ...) {
    va_list ap;

    fprintf(stderr, "error:");
    va_start(ap, format);
    vprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void DoWarning(const char *format, ...) {
    va_list ap;

    fprintf(stderr, "warning:");
    va_start(ap, format);
    vprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}