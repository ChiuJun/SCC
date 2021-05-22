//
// Created by LJChi on 2021/5/17.
//

#ifndef SCC_EXPR_H
#define SCC_EXPR_H

enum OPR {
#define OPRINFO(opr, prec, name, func, opcode) opr,
#include "oprinfo.h"
#undef OPRINFO
};

/*用于标记operator市binary还是unary*/
struct tokenOpr {
    int bop  : 16;
    int uop  : 16;
};

struct astExpression {
    AST_NODE_COMMON
    Type ty;
    int op : 16;
    int isArray : 1;
    int isFunc : 1;
    int isLvalue : 1;
    int isBitfield : 1;
    int isInRegister : 1;
    int isUnused  : 11;
    struct astExpression *kids[2];
    union value exprValue;
};

#define IsBinaryOPR(tok) (tok >= TK_OR && tok <= TK_MOD)

#define	BINARY_OPR TokenOprs[CurrentToken - TK_ASSIGN].bop
#define UNARY_OPR TokenOprs[CurrentToken - TK_ASSIGN].uop

int CanAssign(Type ty, AstExpression expr);
AstExpression Constant(struct coord coord, Type ty, union value val);
AstExpression Cast(Type ty, AstExpression expr);
AstExpression Adjust(AstExpression expr, int rvalue);
AstExpression DoIntegerPromotion(AstExpression expr);
AstExpression FoldConstant(AstExpression expr);
AstExpression FoldCast(Type ty, AstExpression expr);
AstExpression Not(AstExpression expr);
AstExpression CheckExpression(AstExpression expr);
AstExpression CheckConstantExpression(AstExpression expr);

void TranslateBranch(AstExpression expr, BBlock trueBB, BBlock falseBB);
Symbol TranslateExpression(AstExpression expr);

extern char* OprNames[];

#endif //SCC_EXPR_H
