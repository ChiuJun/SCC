//
// Created by LJChi on 2021/5/13.
//

#ifndef SCC_FIRST_H
#define SCC_FIRST_H

#define FIRST_STRUCT_DECLARATION                                                                \
    TK_VOID, TK_CHAR, TK_SHORT, TK_INT, TK_LONG, TK_FLOAT, TK_DOUBLE, TK_SIGNED, TK_UNSIGNED,   \
    TK_STRUCT, TK_UNION, TK_ENUM, TK_ID,                                                        \
    TK_CONST, TK_VOLATILE

#define FIRST_DECLARATION                                                                       \
    TK_TYPEDEF, TK_EXTERN, TK_STATIC, TK_AUTO, TK_REGISTER,                                     \
    FIRST_STRUCT_DECLARATION

#define FIRST_EXPRESSION                                                                    \
    TK_ID,                                                                                  \
    TK_INTCONST, TK_UINTCONST, TK_LONGCONST, TK_ULONGCONST, TK_LLONGCONST, TK_ULLONGCONST,  \
    TK_FLOATCONST, TK_DOUBLECONST, TK_LDOUBLECONST,                                         \
    TK_STRING, TK_WIDESTRING,                                                               \
    TK_LPAREN,                                                                              \
    TK_INC, TK_DEC,TK_BITAND, TK_MUL, TK_ADD, TK_SUB, TK_COMP, TK_NOT,                      \
    TK_SIZEOF

#define FIRST_STATEMENT                         \
    TK_CASE, TK_DEFAULT,                        \
    TK_LBRACE,                                  \
    TK_IF, TK_SWITCH,                           \
    TK_WHILE, TK_DO, TK_FOR,                    \
    TK_GOTO, TK_CONTINUE, TK_BREAK, TK_RETURN,  \
    TK_SEMICOLON, FIRST_EXPRESSION

#endif //SCC_FIRST_H
