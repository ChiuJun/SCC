//
// Created by LJChi on 2021/5/18.
//

#ifndef TOKENOPR
#error "You must define TOKENOPR macro before include this file"
#endif

TOKENOPR(TK_ASSIGN,        OP_ASSIGN,        OP_NONE)
TOKENOPR(TK_BITOR_ASSIGN,  OP_BITOR_ASSIGN,  OP_NONE)
TOKENOPR(TK_BITXOR_ASSIGN, OP_BITXOR_ASSIGN, OP_NONE)
TOKENOPR(TK_BITAND_ASSIGN, OP_BITAND_ASSIGN, OP_NONE)
TOKENOPR(TK_LSHIFT_ASSIGN, OP_LSHIFT_ASSIGN, OP_NONE)
TOKENOPR(TK_RSHIFT_ASSIGN, OP_RSHIFT_ASSIGN, OP_NONE)
TOKENOPR(TK_ADD_ASSIGN,    OP_ADD_ASSIGN,    OP_NONE)
TOKENOPR(TK_SUB_ASSIGN,    OP_SUB_ASSIGN,    OP_NONE)
TOKENOPR(TK_MUL_ASSIGN,    OP_MUL_ASSIGN,    OP_NONE)
TOKENOPR(TK_DIV_ASSIGN,    OP_DIV_ASSIGN,    OP_NONE)
TOKENOPR(TK_MOD_ASSIGN,    OP_MOD_ASSIGN,    OP_NONE)

TOKENOPR(TK_OR,            OP_OR,            OP_NONE)
TOKENOPR(TK_AND,           OP_AND,           OP_NONE)
TOKENOPR(TK_BITOR,         OP_BITOR,         OP_NONE)
TOKENOPR(TK_BITXOR,        OP_BITXOR,        OP_NONE)

TOKENOPR(TK_BITAND,        OP_BITAND,        OP_ADDRESS)
TOKENOPR(TK_EQUAL,         OP_EQUAL,         OP_NONE)
TOKENOPR(TK_UNEQUAL,       OP_UNEQUAL,       OP_NONE)
TOKENOPR(TK_GREAT,         OP_GREAT,         OP_NONE)
TOKENOPR(TK_LESS,          OP_LESS,          OP_NONE)

TOKENOPR(TK_GREAT_EQ,     OP_GREAT_EQ ,       OP_NONE)
TOKENOPR(TK_LESS_EQ, 	  OP_LESS_EQ,		OP_NONE)

TOKENOPR(TK_LSHIFT,        OP_LSHIFT,        OP_NONE)
TOKENOPR(TK_RSHIFT,        OP_RSHIFT,        OP_NONE)

TOKENOPR(TK_ADD,           OP_ADD,           OP_POS)
TOKENOPR(TK_SUB,           OP_SUB,           OP_NEG)
TOKENOPR(TK_MUL,           OP_MUL,           OP_DEREF)
TOKENOPR(TK_DIV,           OP_DIV,           OP_NONE)
TOKENOPR(TK_MOD,           OP_MOD,           OP_NONE)

TOKENOPR(TK_INC,           OP_NONE,          OP_PREINC)
TOKENOPR(TK_DEC,           OP_NONE,          OP_PREDEC)
TOKENOPR(TK_NOT,           OP_NONE,          OP_NOT)
TOKENOPR(TK_COMP,          OP_NONE,          OP_COMP)
