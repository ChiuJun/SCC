#include <stdio.h>
#include "include/lex.h"
#include "include/expr.h"
#include "include/decl.h"
#include "include/stmt.h"

static const char * srcCode = "{int (*arr)[3][5];}";
static char NextCharFromMem(void){
	int ch = *srcCode;
	srcCode++;
	if(ch == 0){
		return (char)EOF_CH;
	}else{
		return (char)ch;
	}
}

static char NextCharFromStdin(void){
	int ch = fgetc(stdin);
	if(ch == EOF){
		return (char)EOF_CH;
	}else{
		return (char)ch;
	}
}

int main(){
    freopen("demo.c","r",stdin);
	AstStmtNodePtr stmt = NULL;

	InitLexer(NextCharFromStdin);
	NEXT_TOKEN;
	stmt = CompoundStatement();
	Expect(TK_EOF);
	VisitStatementNode(stmt);

	return 0;
}


