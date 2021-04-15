//
// Created by LJChi on 2021/4/13.
//

#include "cc.h"

char* FormatName(const char *fmt, ...) {
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return IdentifierName(buf,strlen(buf));
}
