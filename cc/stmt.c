//
// Created by LJChi on 2021/5/18.
//

#include "cc.h"
#include "FIRST.h"
#include "stmt.h"

static int FIRST_Statement[] = {FIRST_STATEMENT, 0};

static AstStatement ParseStatement(void);

/*
 *	labeled-statement:
 *	    identifier : statement
 */
static AstStatement ParseLabelStatement(void) {
    AstLabelStatement labelStmt;

    CREATE_AST_NODE(labelStmt, LabelStatement);
    labelStmt->id = TokenValue.p;
    NEXT_TOKEN;
    NEXT_TOKEN;
    labelStmt->stmt = ParseStatement();

    return (AstStatement) labelStmt;
}

/*
 *  expression-statement:
 *		[expression] ;
 */
static AstStatement ParseExpressionStatement(void) {
    AstExpressionStatement exprStmt;

    CREATE_AST_NODE(exprStmt, ExpressionStatement);
    if (CurrentToken != TK_SEMICOLON) {
        exprStmt->expr = ParseExpression();
    }
    Expect(TK_SEMICOLON);

    return (AstStatement) exprStmt;
}

/*
 *  labeled-statement:
 *		case constant-expression : statement
 */
static AstStatement ParseCaseStatement(void) {
    AstCaseStatement caseStmt;

    CREATE_AST_NODE(caseStmt, CaseStatement);
    NEXT_TOKEN;
    caseStmt->expr = ParseConstantExpression();
    Expect(TK_COLON);
    caseStmt->stmt = ParseStatement();

    return (AstStatement) caseStmt;
}

/*
 *  labeled-statement:
 *		default : statement
 */
static AstStatement ParseDefaultStatement(void) {
    AstDefaultStatement defaultStmt;

    CREATE_AST_NODE(defaultStmt, DefaultStatement);
    NEXT_TOKEN;
    Expect(TK_COLON);
    defaultStmt->stmt = ParseStatement();

    return (AstStatement) defaultStmt;
}

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
AstStatement ParseCompoundStatement(void) {
    AstCompoundStatement compoundStmt;
    AstNode *tail;

    Level++;
    CREATE_AST_NODE(compoundStmt, CompoundStatement);
    NEXT_TOKEN;

    tail = &compoundStmt->decls;
    while (IsCurrentTokenIn(FIRST_Declaration)) {
        if (CurrentToken == TK_ID && !IsTypeName(CurrentToken))
            break;
        *tail = (AstNode) ParseDeclaration();
        tail = &(*tail)->next;
    }
    tail = &compoundStmt->stmts;
    while (CurrentToken != TK_RBRACE && CurrentToken != TK_END) {
        *tail = (AstNode) ParseStatement();
        tail = &(*tail)->next;
        if (CurrentToken == TK_RBRACE)
            break;
        SkipTo(FIRST_Statement, "the beginning of a statement");
    }
    Expect(TK_RBRACE);

    PostCheckTypedef();
    Level--;

    return (AstStatement)compoundStmt;
}

/*
 *  selection-statement:
 *		if ( expression ) statement
 *		if ( expression ) statement else statement
 */
static AstStatement ParseIfStatement(void) {
    AstIfStatement ifStmt;

    CREATE_AST_NODE(ifStmt, IfStatement);
    NEXT_TOKEN;
    Expect(TK_LPAREN);
    ifStmt->expr = ParseExpression();
    Expect(TK_RPAREN);
    ifStmt->thenStmt = ParseStatement();
    if (CurrentToken == TK_ELSE) {
        NEXT_TOKEN;
        ifStmt->elseStmt = ParseStatement();
    }

    return (AstStatement) ifStmt;
}

/*
 *  selection-statement:
 *		switch ( expression ) statement
 */
static AstStatement ParseSwitchStatement(void) {
    AstSwitchStatement switchStmt;

    CREATE_AST_NODE(switchStmt, SwitchStatement);
    NEXT_TOKEN;
    Expect(TK_LPAREN);
    switchStmt->expr = ParseExpression();
    Expect(TK_RPAREN);
    switchStmt->stmt = ParseStatement();

    return (AstStatement) switchStmt;
}

/*
 *  iteration-statement:
 *		while ( expression ) statement
 */
static AstStatement ParseWhileStatement(void) {
    AstLoopStatement whileStmt;

    CREATE_AST_NODE(whileStmt, WhileStatement);
    NEXT_TOKEN;
    Expect(TK_LPAREN);
    whileStmt->expr = ParseExpression();
    Expect(TK_RPAREN);
    whileStmt->stmt = ParseStatement();

    return (AstStatement) whileStmt;
}

/*
 *  iteration-statement:
 *		do statement while ( expression ) ;
 */
static AstStatement ParseDoStatement() {
    AstLoopStatement doStmt;

    CREATE_AST_NODE(doStmt, DoStatement);
    NEXT_TOKEN;
    doStmt->stmt = ParseStatement();
    Expect(TK_WHILE);
    Expect(TK_LPAREN);
    doStmt->expr = ParseExpression();
    Expect(TK_RPAREN);
    Expect(TK_SEMICOLON);

    return (AstStatement) doStmt;
}

/*
 *  iteration-statement:
 *		for ( [expression] ; [expression] ; [expression] ) statement
 */
static AstStatement ParseForStatement() {
    AstForStatement forStmt;

    CREATE_AST_NODE(forStmt, ForStatement);
    NEXT_TOKEN;

    Expect(TK_LPAREN);
    if (CurrentToken != TK_SEMICOLON) {
        forStmt->initExpr = ParseExpression();
    }
    Expect(TK_SEMICOLON);

    if (CurrentToken != TK_SEMICOLON) {
        forStmt->expr = ParseExpression();
    }
    Expect(TK_SEMICOLON);

    if (CurrentToken != TK_RPAREN) {
        forStmt->incrExpr = ParseExpression();
    }
    Expect(TK_RPAREN);

    forStmt->stmt = ParseStatement();

    return (AstStatement) forStmt;
}

/*
 *  jump-statement:
 *		goto identifier ;
 */
static AstStatement ParseGotoStatement(void) {
    AstGotoStatement gotoStmt;

    CREATE_AST_NODE(gotoStmt, GotoStatement);
    NEXT_TOKEN;
    if (CurrentToken == TK_ID) {
        gotoStmt->id = TokenValue.p;
        NEXT_TOKEN;
        Expect(TK_SEMICOLON);
    } else {
        Error(&TokenCoord, "Expect identifier");
        if (CurrentToken == TK_SEMICOLON)
            NEXT_TOKEN;
    }

    return (AstStatement) gotoStmt;
}

/*
 *  jump-statement:
 *		continue ;
 */
static AstStatement ParseContinueStatement(void) {
    AstContinueStatement continueStmt;

    CREATE_AST_NODE(continueStmt, ContinueStatement);
    NEXT_TOKEN;
    Expect(TK_SEMICOLON);

    return (AstStatement) continueStmt;
}

/*
 *  jump-statement:
 *		break ;
 */
static AstStatement ParseBreakStatement(void) {
    AstBreakStatement breakStmt;

    CREATE_AST_NODE(breakStmt, BreakStatement);
    NEXT_TOKEN;
    Expect(TK_SEMICOLON);

    return (AstStatement) breakStmt;
}

/*
 *  jump-statement:
 *		return [expression] ;
 */
static AstStatement ParseReturnStatement(void) {
    AstReturnStatement returnStmt;

    CREATE_AST_NODE(returnStmt, ReturnStatement);
    NEXT_TOKEN;
    if (CurrentToken != TK_SEMICOLON) {
        returnStmt->expr = ParseExpression();
    }
    Expect(TK_SEMICOLON);

    return (AstStatement) returnStmt;
}

/*
 *  statement:
 *		labeled-statement
 *		compound-statement
 *		expression-statement
 *      selection-statement
 *      iteration-statement
 *      jump-statement
 *	labeled-statement:
 *	    identifier : statement
 *	    case constant-expression : statement
 *	    default : statement
 *	selection-statement:
 *	    if ( expression ) statement
 *	    if ( expression ) statement else statement
 *	    switch ( expression ) statement
 *	iteration-statement:
 *	    while ( expression ) statement
 *	    do statement while ( expression ) ;
 *	    for ( [expression] ; [expression] ; [expression] ) statement
 *	jump-statement:
 *	    goto identifier ;
 *	    continue ;
 *	    break ;
 *	    return [expression] ;
 */
static AstStatement ParseStatement(void) {
    int tmpToken;
    switch (CurrentToken) {
        /*labeled-statement*/
        case TK_ID:
            BeginPeekToken();
            tmpToken = GetNextToken();
            EndPeekToken();
            if(tmpToken == TK_COLON) {/*identifier : statement*/
                return ParseLabelStatement();
            } else {/*expression-statement*/
                return ParseExpressionStatement();
            }
        case TK_CASE:
            return ParseCaseStatement();
        case TK_DEFAULT:
            return ParseDefaultStatement();
        /*compound-statement*/
        case TK_LBRACE:
            return ParseCompoundStatement();
        /*selection-statement*/
        case TK_IF:
            return ParseIfStatement();
        case TK_SWITCH:
            return ParseSwitchStatement();
        /*iteration-statement*/
        case TK_WHILE:
            return ParseWhileStatement();
        case TK_DO:
            return ParseDoStatement();
        case TK_FOR:
            return ParseForStatement();
        /*jump-statement*/
        case TK_GOTO:
            return ParseGotoStatement();
        case TK_CONTINUE:
            return ParseContinueStatement();
        case TK_BREAK:
            return ParseBreakStatement();
        case TK_RETURN:
            return ParseReturnStatement();
        /*expression-statement*/
        default:
            return ParseExpressionStatement();
    }
}
