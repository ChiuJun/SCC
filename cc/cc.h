//
// Created by LJChi on 2021/3/20.
//

#ifndef SCC_CC_H
#define SCC_CC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "input.h"
#include "output.h"
#include "error.h"
#include "alloc.h"
#include "vector.h"
#include "str.h"
#include "type.h"
#include "lex.h"
#include "symbol.h"
#include "gen.h"
#include "ast.h"

extern FILE *ASTFile;
extern FILE *IRFile;
extern FILE *ASMFile;
extern char *ASMExtName;
extern char *ASMFileName;
extern Heap CurrentHeap;
extern struct heap ProgramHeap;
extern struct heap FileHeap;
extern struct heap StringHeap;
extern int ErrorCount;
extern int WarningCount;

#define ALIGN(size, align)  ((align == 0) ? size: ((size + align - 1) & (~(align - 1))))

#endif //SCC_CC_H
