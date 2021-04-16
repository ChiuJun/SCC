//
// Created by LJChi on 2021/4/16.
//

#ifndef SCC_GEN_H
#define SCC_GEN_H

struct basicBlock {
    struct basicBlock *prev;
    struct basicBlock *next;
    Symbol sym;
};

#endif //SCC_GEN_H
