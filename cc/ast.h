//
// Created by LJChi on 2021/4/16.
//

#ifndef SCC_AST_H
#define SCC_AST_H

enum nodeKind {
    NK_TranslationUnit, NK_Function, NK_Declaration,
    NK_TypeName, NK_Specifiers, NK_Token,
    NK_TypedefName, NK_EnumSpecifier, NK_Enumerator,
    NK_StructSpecifier, NK_UnionSpecifier, NK_StructDeclaration,
    NK_StructDeclarator, NK_PointerDeclarator, NK_ArrayDeclarator,
    NK_FunctionDeclarator, NK_ParameterTypeList, NK_ParameterDeclaration,
    NK_NameDeclarator, NK_InitDeclarator, NK_Initializer,

    NK_Expression,

    NK_ExpressionStatement, NK_LabelStatement, NK_CaseStatement,
    NK_DefaultStatement, NK_IfStatement, NK_SwitchStatement,
    NK_WhileStatement, NK_DoStatement, NK_ForStatement,
    NK_GotoStatement, NK_BreakStatement, NK_ContinueStatement,
    NK_ReturnStatement, NK_CompoundStatement
};

/*
 * astNodeKind:语法树节点的类型
 * next:指向下一语法树节点
 * astNodeCoord:语法树节点对应的coord
 * */
#define AST_NODE_COMMON   \
    int astNodeKind;      \
    struct astNode *next; \
    struct coord astNodeCoord;

typedef struct astNode {
    AST_NODE_COMMON
} *AstNode;

/*
 * labelCoord:label对应的coord
 * labelId:label对应的id
 * refCount:label的引用计数
 * isDefine:label是否定义
 * corrBBlock:label对应的基本块
 * nextLabel:指向下一个label的指针
 * */
typedef struct label {
    struct coord labelCoord;
    char *labelId;
    int refCount;
    int isDefine;
    BBlock corrBBlock;
    struct label *nextLabel;
} *Label;

typedef struct astExpression        *AstExpression;
typedef struct astStatement         *AstStatement;
typedef struct astDeclaration       *AstDeclaration;
typedef struct astTypeName          *AstTypeName;
typedef struct astTranslationUnit   *AstTranslationUnit;

struct initData {
    int offset;
    struct astExpression *expr;
    struct initData *next;
};

#define NEXT_TOKEN  CurrentToken = GetNextToken()

#define CREATE_AST_NODE(ptr, kind)      \
    CALLOC(ptr);                        \
    ptr->astNodeKind = NK_##kind;       \
    ptr->astNodeCoord = TokenCoord;     \

AstExpression      ParseExpression(void);
AstExpression      ParseAssignmentExpression(void);
AstExpression      ParseConstantExpression(void);
AstStatement       ParseCompoundStatement(void);
AstTypeName        ParseTypeName(void);
AstDeclaration     ParseDeclaration(void);
AstTranslationUnit ParseTranslationUnit(char *file);

int IsTypeName(int tok);

void PostCheckTypedef(void);
void CheckTranslationUnit(AstTranslationUnit transUnit);
void Translate(AstTranslationUnit transUnit);
void EmitTranslationUnit(AstTranslationUnit transUnit);

void DumpTranslationUnit(AstTranslationUnit transUnit);
void DisassemblyTranslationUnit(AstTranslationUnit transUnit);

extern int CurrentToken;
extern int FIRST_Declaration[];

extern const char *CurrentFileName;
extern int CurrentLineNum;

int IsCurrentTokenIn(int tokens[]);
void DoExpect(int tok);
void DoSkipTo(int tokens[], char *errorMessage);

#define Expect CurrentFileName = __FILE__, CurrentLineNum = __LINE__, DoExpect
#define SkipTo CurrentFileName = __FILE__, CurrentLineNum = __LINE__, DoSkipTo

#endif //SCC_AST_H
