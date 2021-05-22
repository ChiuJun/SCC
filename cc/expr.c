//
// Created by LJChi on 2021/5/17.
//

#include "cc.h"
#include "expr.h"

static struct tokenOpr TokenOprs[] =
        {
#define TOKENOPR(tok, bop, uop) {bop, uop},
#include "tokenopr.h"
#undef  TOKENOPR
        };

static int oprPrecedence[] =
        {
#define OPRINFO(opr, prec, name, func, opcode) prec,
#include "oprinfo.h"
                0
#undef OPRINFO
        };

char *OprNames[] =
        {
#define OPRINFO(opr, prec, name, func, opcode) name,
#include "oprinfo.h"
                NULL
#undef OPRINFO
        };

AstExpression ConstantZero;

/*
 *  primary-expression:
 *		identifier
 *		constant
 *		string-literal
 *		( expression )
 */
static AstExpression ParsePrimaryExpression(void) {
    AstExpression expr;
    switch (CurrentToken) {
        case TK_ID:
            CREATE_AST_NODE(expr, Expression);
            expr->op = OP_ID;
            expr->exprValue = TokenValue;
            NEXT_TOKEN;
            return expr;
        case TK_INTCONST:
        case TK_UINTCONST:
        case TK_LONGCONST:
        case TK_ULONGCONST:
        case TK_LLONGCONST:
        case TK_ULLONGCONST:
        case TK_FLOATCONST:
        case TK_DOUBLECONST:
        case TK_LDOUBLECONST:
        CREATE_AST_NODE(expr, Expression);
            if (CurrentToken >= TK_FLOATCONST)
                expr->ty = T(FLOAT + CurrentToken - TK_FLOATCONST);
            else
                expr->ty = T(INT + CurrentToken - TK_INTCONST);
            expr->op = OP_CONST;
            expr->exprValue = TokenValue;
            NEXT_TOKEN;
            return expr;
        case TK_STRING:
        case TK_WIDESTRING:
            CREATE_AST_NODE(expr, Expression);
            expr->ty = ArrayOf(((String) TokenValue.p)->len + 1, CurrentToken == TK_STRING ? T(CHAR) : WCharType);
            expr->op = OP_STR;
            expr->exprValue = TokenValue;
            NEXT_TOKEN;
            return expr;
        case TK_LPAREN:
            NEXT_TOKEN;
            expr = ParseExpression();
            Expect(TK_RPAREN);
            return expr;
        default:
            Error(&TokenCoord, "Expect identifier, constant, string-literal or (");
            return ConstantZero;
    }
}

/*
 *  postfix-expression:
 *		primary-expression
 *		postfix-expression [ expression ]
 *		postfix-expression ( [argument-expression-list] )
 *		postfix-expression . identifier
 *		postfix-expression -> identifier
 *		postfix-expression ++
 *		postfix-expression --
 *	argument-expression-list:
 *	    assignment-expression
 *	    argument-expression-list , assignment-expression
 */
static AstExpression ParsePostfixExpression(void) {
    AstExpression postfixExpr, expr;

    postfixExpr = ParsePrimaryExpression();

    while (1) {
        switch (CurrentToken) {
            case TK_LBRACKET:/*postfix-expression [ expression ]*/
                CREATE_AST_NODE(expr, Expression);
                expr->op = OP_INDEX;
                NEXT_TOKEN;
                expr->kids[0] = postfixExpr;
                expr->kids[1] = ParseExpression();
                Expect(TK_RBRACKET);
                postfixExpr = expr;
                break;
            case TK_LPAREN:/*postfix-expression ( [argument-expression-list] )*/
                CREATE_AST_NODE(expr, Expression);
                expr->op = OP_CALL;
                expr->kids[0] = postfixExpr;
                NEXT_TOKEN;
                if (CurrentToken != TK_RPAREN) {
                    AstNode *tail;
                    expr->kids[1] = ParseAssignmentExpression();
                    tail = &expr->kids[1]->next;
                    while (CurrentToken == TK_COMMA) {
                        NEXT_TOKEN;
                        *tail = (AstNode) ParseAssignmentExpression();
                        tail = &(*tail)->next;
                    }
                }
                Expect(TK_RPAREN);
                postfixExpr = expr;
                break;
            case TK_DOT:/*postfix-expression . identifier*/
            case TK_POINTER:/*postfix-expression -> identifier*/
                CREATE_AST_NODE(expr, Expression);
                expr->op = (CurrentToken == TK_DOT ? OP_MEMBER : OP_PTR_MEMBER);
                expr->kids[0] = postfixExpr;
                NEXT_TOKEN;
                if (CurrentToken != TK_ID) {
                    Error(&expr->astNodeCoord, "Expect identifier as struct or union member");
                } else {
                    expr->exprValue = TokenValue;
                    NEXT_TOKEN;
                }
                postfixExpr = expr;
                break;
            case TK_INC:/*postfix-expression ++*/
            case TK_DEC:/*postfix-expression --*/
                CREATE_AST_NODE(expr, Expression);
                expr->op = (CurrentToken == TK_INC) ? OP_POSTINC : OP_POSTDEC;
                expr->kids[0] = postfixExpr;
                NEXT_TOKEN;
                postfixExpr = expr;
                break;
            default:
                return postfixExpr;
        }
    }
}

/*
 *  unary-expression:
 *		postfix-expression
 *		++ unary-expression
 *		-- unary-expression
 *		unary-operator unary-expression
 *		sizeof unary-expression
 *		sizeof ( type-name )
 *		( type-name ) unary-expression
 *  unary-operator:
 *		& * + - ~ !
 */
static AstExpression ParseUnaryExpression() {
    AstExpression expr;
    int tmpToken;

    switch (CurrentToken) {
        case TK_INC:
        case TK_DEC:
        case TK_BITAND:
        case TK_MUL:
        case TK_ADD:
        case TK_SUB:
        case TK_COMP:
        case TK_NOT:
            CREATE_AST_NODE(expr, Expression);
            expr->op = UNARY_OPR;
            NEXT_TOKEN;
            expr->kids[0] = ParseUnaryExpression();
            return expr;
        case TK_LPAREN:
            BeginPeekToken();
            tmpToken = GetNextToken();
            if (IsTypeName(tmpToken)) {/*( type-name ) unary-expression*/
                EndPeekToken();
                CREATE_AST_NODE(expr, Expression);
                expr->op = OP_CAST;
                NEXT_TOKEN;

                expr->kids[0] = (AstExpression) ParseTypeName();
                Expect(TK_RPAREN);
                expr->kids[1] = ParseUnaryExpression();

                return expr;
            } else {/*primary-expression*/
                EndPeekToken();
                return ParsePostfixExpression();
            }
        case TK_SIZEOF:
            CREATE_AST_NODE(expr, Expression);
            expr->op = OP_SIZEOF;
            NEXT_TOKEN;
            if (CurrentToken == TK_LPAREN) {
                BeginPeekToken();
                tmpToken = GetNextToken();
                if (IsTypeName(tmpToken)) {/*sizeof ( type-name )*/
                    EndPeekToken();
                    NEXT_TOKEN;
                    expr->kids[0] = (AstExpression) ParseTypeName();
                    Expect(TK_RPAREN);
                } else {/*sizeof unary-expression*/
                    EndPeekToken();
                    expr->kids[0] = ParseUnaryExpression();
                }
            } else {/*sizeof unary-expression*/
                expr->kids[0] = ParseUnaryExpression();
            }
            return expr;
        default:
            return ParsePostfixExpression();
    }
}

/*
 * 统一处理二元操作符
 * */
static AstExpression ParseBinaryExpression(int precedence) {
    AstExpression expr, binaryExpr;
    int newPrecedence;

    expr = ParseUnaryExpression();

    while (IsBinaryOPR(CurrentToken) && (newPrecedence = oprPrecedence[BINARY_OPR]) >= precedence) {
        CREATE_AST_NODE(binaryExpr, Expression);

        binaryExpr->op = BINARY_OPR;
        binaryExpr->kids[0] = expr;
        NEXT_TOKEN;
        binaryExpr->kids[1] = ParseBinaryExpression(newPrecedence + 1);

        expr = binaryExpr;
    }

    return expr;

}

/*
 * conditional-expression:
 *      logical-OR-expression
 *      logical-OR-expression ? expression : conditional-expression
 * */
static AstExpression ParseConditionalExpression(void) {
    AstExpression logicalOrExpr;

    logicalOrExpr = ParseBinaryExpression(oprPrecedence[OP_OR]);
    if (CurrentToken == TK_QUESTION) {
        AstExpression condExpr;
        CREATE_AST_NODE(condExpr, Expression);

        condExpr->op = OP_QUESTION;
        condExpr->kids[0] = logicalOrExpr;
        NEXT_TOKEN;

        CREATE_AST_NODE(condExpr->kids[1], Expression);
        condExpr->kids[1]->op = OP_COLON;
        condExpr->kids[1]->kids[0] = ParseExpression();
        Expect(TK_COLON);
        condExpr->kids[1]->kids[1] = ParseConditionalExpression();

        return condExpr;
    }

    return logicalOrExpr;
}

/*
 * expression:
 *      assignment-expression
 *      expression , assignment-expression
 * */
AstExpression ParseExpression(void) {
    AstExpression expr, commaExpr;

    expr = ParseAssignmentExpression();
    while (CurrentToken == TK_COMMA) {
        CREATE_AST_NODE(commaExpr, Expression);

        commaExpr->op = OP_COMMA;
        NEXT_TOKEN;
        commaExpr->kids[0] = expr;
        commaExpr->kids[1] = ParseAssignmentExpression();

        expr = commaExpr;
    }

    return expr;
}

/*
 *  assignment-expression:
 *      conditional-expression
 *      unary-expression assignment-operator assignment-expression
 *  assignment-operator:
 *      = *= /= %= += -= <<= >>= &= ^= |=
 *  将unary-expression视作conditional-expression的特殊情况，之后进行语义分析时候再检查这部分语法的正确性
 */
AstExpression ParseAssignmentExpression(void) {
    AstExpression expr, assignExpr;
    expr = ParseConditionalExpression();

    if (CurrentToken >= TK_ASSIGN && CurrentToken <= TK_MOD_ASSIGN) {
        CREATE_AST_NODE(assignExpr, Expression);

        assignExpr->op = BINARY_OPR;
        assignExpr->kids[0] = expr;
        NEXT_TOKEN;
        assignExpr->kids[1] = ParseAssignmentExpression();

        return assignExpr;
    }

    return expr;
}

/*
 * constant-expression:
 *      conditional-expression
 * */
AstExpression ParseConstantExpression(void) {
    return ParseConditionalExpression();
}
