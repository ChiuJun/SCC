//
// Created by LJChi on 2021/4/13.
//

#include "cc.h"

/*
 * TODO:InternName
 * */
char *FormatName(const char *fmt, ...) {
    char *buf = HeapAllocate(CurrentHeap, 256);
    /*TODO:
     * char buf[256];*/
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, 256, fmt, ap);
    va_end(ap);

    return buf;
}
