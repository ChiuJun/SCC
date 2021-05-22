//
// Created by LJChi on 2021/5/18.
//

#ifndef SCC_STMT_H
#define SCC_STMT_H

#define AST_STATEMENT_COMMON AST_NODE_COMMON

#define AST_LOOP_STATEMENT_COMMON \
    AST_STATEMENT_COMMON          \
    AstExpression expr;           \
    AstStatement stmt;            \
    BBlock loopBB;                \
    BBlock contBB;                \
    BBlock nextBB;

struct astStatement {
    AST_STATEMENT_COMMON
};

/*
 * labeled-statement:
 *      identifier : statement
 * */
typedef struct astLabelStatement {
    AST_STATEMENT_COMMON
    char *id;
    AstStatement stmt;
    Label label;
} *AstLabelStatement;

/*
 * expression-statement:
 *		[expression] ;
 * */
typedef struct astExpressionStatement {
    AST_STATEMENT_COMMON
    AstExpression expr;
} *AstExpressionStatement;

/*
 *  labeled-statement:
 *		case constant-expression : statement
 */
typedef struct astCaseStatement {
    AST_STATEMENT_COMMON
    AstExpression expr;
    AstStatement stmt;
    struct astCaseStatement *nextCase;
    BBlock respBB;
} *AstCaseStatement;

/*
 *  labeled-statement:
 *		default : statement
 */
typedef struct astDefaultStatement {
    AST_STATEMENT_COMMON
    AstStatement stmt;
    BBlock respBB;
} *AstDefaultStatement;

/*
 *  compound-statement:
 *		{ [declaration-list] [statement-list] }
 *  declaration-list:
 *		declaration
 *		declaration-list declaration
 *  statement-list:
 *		statement
 *		statement-list statement
 */
typedef struct astCompoundStatement {
    AST_STATEMENT_COMMON
    AstNode decls;
    AstNode stmts;
    /*局部变量的初始化列表*/
    Vector ilocals;
} *AstCompoundStatement;

/*
 *  selection-statement:
 *		if ( expression ) statement
 *		if ( expression ) statement else statement
 */
typedef struct astIfStatement {
    AST_STATEMENT_COMMON
    AstExpression expr;
    AstStatement thenStmt;
    AstStatement elseStmt;
} *AstIfStatement;

/*
 *  selection-statement:
 *		switch ( expression ) statement
 */
typedef struct switchBucket {
    int ncase;
    int minVal;
    int maxVal;
    AstCaseStatement cases;
    AstCaseStatement *tail;
    struct switchBucket *prev;
} *SwitchBucket;
typedef struct astSwitchStatement {
    AST_STATEMENT_COMMON
    AstExpression expr;
    AstStatement stmt;
    AstCaseStatement cases;
    AstDefaultStatement defStmt;
    SwitchBucket buckets;
    int nbucket;
    BBlock nextBB;
    BBlock defBB;
} *AstSwitchStatement;

/*
 *  iteration-statement:
 *	    while ( expression ) statement
 *	    do statement while ( expression ) ;
 * */
typedef struct astLoopStatement {
    AST_LOOP_STATEMENT_COMMON
} *AstLoopStatement;

/*
 *  iteration-statement:
 *		for ( [expression] ; [expression] ; [expression] ) statement
 */
typedef struct astForStatement {
    AST_LOOP_STATEMENT_COMMON
    AstExpression initExpr;
    AstExpression incrExpr;
    BBlock testBB;
} *AstForStatement;

/*
 *  jump-statement:
 *		goto identifier ;
 */
typedef struct astGotoStatement {
    AST_STATEMENT_COMMON
    char *id;
    Label label;
} *AstGotoStatement;

/*
 *  jump-statement:
 *		continue ;
 */
typedef struct astContinueStatement {
    AST_STATEMENT_COMMON
    AstLoopStatement target;
} *AstContinueStatement;

/*
 *  jump-statement:
 *		break ;
 */
typedef struct astBreakStatement {
    AST_STATEMENT_COMMON
    AstStatement target;
} *AstBreakStatement;

/*
 *  jump-statement:
 *		return [expression] ;
 */
typedef struct astReturnStatement {
    AST_STATEMENT_COMMON
    AstExpression expr;
} *AstReturnStatement;

#define AsLabel(stmt)  ((AstLabelStatement)stmt)
#define AsExpr(stmt)   ((AstExpressionStatement)stmt)
#define AsCase(stmt)   ((AstCaseStatement)stmt)
#define AsDef(stmt)    ((AstDefaultStatement)stmt)
#define AsComp(stmt)   ((AstCompoundStatement)stmt)
#define AsIf(stmt)     ((AstIfStatement)stmt)
#define AsSwitch(stmt) ((AstSwitchStatement)stmt)
#define AsLoop(stmt)   ((AstLoopStatement)stmt)
#define AsFor(stmt)    ((AstForStatement)stmt)
#define AsGoto(stmt)   ((AstGotoStatement)stmt)
#define AsCont(stmt)   ((AstContinueStatement)stmt)
#define AsBreak(stmt)  ((AstBreakStatement)stmt)
#define AsRet(stmt)    ((AstReturnStatement)stmt)

AstStatement CheckCompoundStatement(AstStatement stmt);

#endif //SCC_STMT_H
