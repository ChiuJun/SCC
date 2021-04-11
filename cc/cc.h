//
// Created by LJChi on 2021/3/20.
//

#ifndef SCC_CC_H
#define SCC_CC_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "error.h"
#include "alloc.h"
#include "vector.h"

extern Heap CurrentHeap;

#define ALIGN(size, align)  ((align == 0) ? size: ((size + align - 1) & (~(align - 1))))

#endif //SCC_CC_H
