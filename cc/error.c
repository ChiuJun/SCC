//
// Created by LJChi on 2021/3/28.
//

#include "cc.h"

void DoFatal(const char *format, ...) {
    va_list ap;

    fprintf(stderr, "fatal error:");
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    exit(-1);
}

void DoError(Coord current_coord, const char *format, ...) {
    va_list ap;

    ErrorCount++;
    if(current_coord != NULL){
        fprintf(stderr,"(%s,%d):",current_coord->src_filename,current_coord->src_line);
    }
    fprintf(stderr, "error:");
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void DoWarning(Coord current_coord, const char *format, ...) {
    va_list ap;

    WarningCount++;
    if(current_coord != NULL){
        fprintf(stderr,"(%s,%d):",current_coord->src_filename,current_coord->src_line);
    }
    fprintf(stderr, "warning:");
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}