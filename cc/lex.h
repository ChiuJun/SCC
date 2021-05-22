//
// Created by LJChi on 2021/4/16.
//

#ifndef SCC_LEX_H
#define SCC_LEX_H

/*
 * TK_BEGIN用于占位，使token枚举从1开始
 * 方便后期函数编写
 * */
enum token {
    TK_BEGIN,
#define TOKEN(k, s) k,
#include "token.h"
#undef  TOKEN
};

union value {
    /*存放整数值*/
    int i[2];
    /*存放float类型的值*/
    float f;
    /*存放double类型的值*/
    double d;
    /*用于更复杂的数据*/
    void *p;
};

#define IsDigit(c)         (c >= '0' && c <= '9')
#define IsOctDigit(c)      (c >= '0' && c <= '7')
#define IsHexDigit(c)      (IsDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
#define IsLetter(c)        ((c >= 'a' && c <= 'z') || (c == '_') || (c >= 'A' && c <= 'Z'))
#define IsLetterOrDigit(c) (IsLetter(c) || IsDigit(c))
// 1101 1111 e.g. a:97 A 65
#define ToUpper(c)		   (c & ~0x20)
#define HIGH_4BIT(v)       ((v) >> (8 * sizeof(int) - 4) & 0x0f)
#define HIGH_3BIT(v)       ((v) >> (8 * sizeof(int) - 3) & 0x07)
#define HIGH_1BIT(v)       ((v) >> (8 * sizeof(int) - 1) & 0x01)

void BeginPeekToken(void);
void EndPeekToken(void);
void SetupLexer(void);
int  GetNextToken(void);

extern union value  TokenValue;
extern struct coord TokenCoord;
extern struct coord PrevCoord;
extern char* TokenStrings[];

#endif //SCC_LEX_H
