//
// Created by LJChi on 2021/4/16.
//

#ifndef SCC_LEX_H
#define SCC_LEX_H

union value{
    /*存放整数值*/
    int i[2];
    /*存放float类型的值*/
    float f;
    /*存放double类型的值*/
    double d;
    /*用于更复杂的数据*/
    void *p;
};

#endif //SCC_LEX_H
