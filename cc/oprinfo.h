//
// Created by LJChi on 2021/5/17.
//
#ifndef OPRINFO
#error "You must define OPRINFO macro before include this file"
#endif
/*     枚举名           优先级  操作符      函数名           编码*/
OPRINFO(OP_COMMA,         1,    ",",      Comma,          NOP)

OPRINFO(OP_ASSIGN,        2,    "=",      Assignment,     NOP)
OPRINFO(OP_BITOR_ASSIGN,  2,    "|=",     Assignment,     NOP)
OPRINFO(OP_BITXOR_ASSIGN, 2,    "^=",     Assignment,     NOP)
OPRINFO(OP_BITAND_ASSIGN, 2,    "&=",     Assignment,     NOP)
OPRINFO(OP_LSHIFT_ASSIGN, 2,    "<<=",    Assignment,     NOP)
OPRINFO(OP_RSHIFT_ASSIGN, 2,    ">>=",    Assignment,     NOP)
OPRINFO(OP_ADD_ASSIGN,    2,    "+=",     Assignment,     NOP)
OPRINFO(OP_SUB_ASSIGN,    2,    "-=",     Assignment,     NOP)
OPRINFO(OP_MUL_ASSIGN,    2,    "*=",     Assignment,     NOP)
OPRINFO(OP_DIV_ASSIGN,    2,    "/=",     Assignment,     NOP)
OPRINFO(OP_MOD_ASSIGN,    2,    "%=",     Assignment,     NOP)

OPRINFO(OP_QUESTION,      3,    "?",      Conditional,    NOP)
OPRINFO(OP_COLON,         3,    ":",      Error,          NOP)

OPRINFO(OP_OR,            4,    "||",     Binary,         NOP)

OPRINFO(OP_AND,           5,    "&&",     Binary,         NOP)

OPRINFO(OP_BITOR,         6,    "|",      Binary,         BOR)

OPRINFO(OP_BITXOR,        7,    "^",      Binary,         BXOR)

OPRINFO(OP_BITAND,        8,    "&",      Binary,         BAND)

OPRINFO(OP_EQUAL,         9,    "==",     Binary,         JE)
OPRINFO(OP_UNEQUAL,       9,    "!=",     Binary,         JNE)

OPRINFO(OP_GREAT,         10,   ">",      Binary,         JG)
OPRINFO(OP_LESS,          10,   "<",      Binary,         JL)
OPRINFO(OP_GREAT_EQ,      10,   ">=",     Binary,         JGE)
OPRINFO(OP_LESS_EQ,       10,   "<=",     Binary,         JLE)

OPRINFO(OP_LSHIFT,        11,   "<<",     Binary,         LSH)
OPRINFO(OP_RSHIFT,        11,   ">>",     Binary,         RSH)

OPRINFO(OP_ADD,           12,   "+",      Binary,         ADD)
OPRINFO(OP_SUB,           12,   "-",      Binary,         SUB)

OPRINFO(OP_MUL,           13,   "*",      Binary,         MUL)
OPRINFO(OP_DIV,           13,   "/",      Binary,         DIV)
OPRINFO(OP_MOD,           13,   "%",      Binary,         MOD)

OPRINFO(OP_CAST,          14,   "cast",   Unary,          NOP)
OPRINFO(OP_PREINC,        14,   "++",     Unary,          NOP)
OPRINFO(OP_PREDEC,        14,   "--",     Unary,          NOP)
OPRINFO(OP_ADDRESS,       14,   "&",      Unary,          ADDR)
OPRINFO(OP_DEREF,         14,   "*",      Unary,          DEREF)
OPRINFO(OP_POS,           14,   "+",      Unary,          NOP)
OPRINFO(OP_NEG,           14,   "-",      Unary,          NEG)
OPRINFO(OP_COMP,          14,   "~",      Unary,          BCOM)
OPRINFO(OP_NOT,           14,   "!",      Unary,          NOP)
OPRINFO(OP_SIZEOF,        14,   "sizeof", Unary,          NOP)

OPRINFO(OP_INDEX,         15,   "[]",     Postfix,        NOP)
OPRINFO(OP_CALL,          15,   "call",   Postfix,        NOP)
OPRINFO(OP_MEMBER,        15,   ".",      Postfix,        NOP)
OPRINFO(OP_PTR_MEMBER,    15,   "->",     Postfix,        NOP)
OPRINFO(OP_POSTINC,       15,   "++",     Postfix,        INC)
OPRINFO(OP_POSTDEC,       15,   "--",     Postfix,        DEC)

OPRINFO(OP_ID,            16,   "id",     Primary,        NOP)
OPRINFO(OP_CONST,         16,   "const",  Primary,        NOP)
OPRINFO(OP_STR,           16,   "str",    Primary,        NOP)

OPRINFO(OP_NONE,          17,   "nop",    Error,          NOP)