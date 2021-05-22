//
// Created by LJChi on 2021/5/13.
//

#include "cc.h"
#include "decl.h"
#include "FIRST.h"

static Vector TypedefNames, OverloadNames;

int FIRST_Declaration[] = {FIRST_DECLARATION, 0};

static int FIRST_ExternalDeclaration[] = {FIRST_DECLARATION, TK_MUL, TK_LPAREN, TK_SEMICOLON, 0};

static int FIRST_StructDeclaration[] = {FIRST_STRUCT_DECLARATION, TK_SEMICOLON, 0};
static int FIRST_StructDeclarationWithRBrace[] = {FIRST_STRUCT_DECLARATION, TK_SEMICOLON, TK_RBRACE, 0};

static AstDeclarator ParseDeclarator(int abstract);
static AstSpecifiers ParseDeclarationSpecifiers(void);

static char *GetOutermostID(AstDeclarator dec) {
    if (dec->astNodeKind == NK_NameDeclarator)
        return dec->id;

    return GetOutermostID(dec->dec);
}

static int IsTypedefName(char *id) {
    Vector v = TypedefNames;
    TDName tdNameItem;
    FOR_EACH_VECTOR_ITEM(TDName, tdNameItem, v)
    if (tdNameItem->id == id && tdNameItem->level <= Level && !tdNameItem->overload)
        return 1;
    END_FOR_EACH_VECTOR_ITEM

    return 0;
}

static void CheckTypedefName(int sclass, char *id) {
    TDName tdNameItem;

    if (id == NULL)
        return;
    if (sclass == TK_TYPEDEF) {
        FOR_EACH_VECTOR_ITEM(TDName, tdNameItem, TypedefNames)
        if (tdNameItem->id == id && Level < tdNameItem->level) {
            /*遇到level更小的typedef则更新该typedef*/
            tdNameItem->level = Level;
            return;
        }
        END_FOR_EACH_VECTOR_ITEM
        ALLOC(tdNameItem);
        tdNameItem->id = id;
        tdNameItem->level = Level;
        tdNameItem->overload = 0;
        INSERT_VECTOR_ITEM(TypedefNames, tdNameItem);
    } else {
        FOR_EACH_VECTOR_ITEM(TDName, tdNameItem, TypedefNames)
        if (tdNameItem->id == id && Level > tdNameItem->level) {
            tdNameItem->overload = 1;
            tdNameItem->overloadLevel = Level;
            INSERT_VECTOR_ITEM(OverloadNames, tdNameItem);
            return;
        }
        END_FOR_EACH_VECTOR_ITEM
    }
}

/*
 * 对每一个声明作最小的语义检查
 * */
static void PreCheckTypedef(AstDeclaration decl) {
    int stgClass = TK_BEGIN;
    AstNode initDecl = decl->initDecs;

    if (decl->specs->stgClasses != NULL) {
        stgClass = ((AstToken) decl->specs->stgClasses)->token;
    }

    while (initDecl != NULL) {
        CheckTypedefName(stgClass, GetOutermostID(((AstInitDeclarator) initDecl)->dec));
        initDecl = initDecl->next;
    }
}

/*
 * 在即将离开一个作用域时对重载的变量名进行检查
 * */
void PostCheckTypedef(void) {
    TDName tdNameItem;
    int overloadCount = 0;

    // 操作TypedefNames中item 的 id为NULL的代码被删除了
    // TODO:不知道用意
    FOR_EACH_VECTOR_ITEM(TDName, tdNameItem, OverloadNames)
    if (Level <= (tdNameItem->overloadLevel)) {
        tdNameItem->overload = 0;
    } else if (tdNameItem->overload != 0) {
        overloadCount++;
    }
    END_FOR_EACH_VECTOR_ITEM

    if (overloadCount == 0) {
        OverloadNames->size = 0;
    }
}

/*
 *  initializer:
 *		assignment-expression
 *		{ initializer-list }
 *		{ initializer-list , }
 *  initializer-list:
 *		initializer
 *		initializer-list, initializer
 */
static AstInitializer ParseInitializer(void) {
    AstInitializer init;
    AstNode *tail;

    CREATE_AST_NODE(init, Initializer);
    if (CurrentToken == TK_LBRACE) {
        init->lbrace = 1;
        NEXT_TOKEN;
        init->initials = (AstNode) ParseInitializer();
        tail = &init->initials->next;
        while (CurrentToken == TK_COMMA) {
            NEXT_TOKEN;
            if (CurrentToken == TK_RBRACE)
                break;
            *tail = (AstNode) ParseInitializer();
            tail = &(*tail)->next;
        }
        Expect(TK_RBRACE);
    } else {
        init->lbrace = 0;
        init->expr = ParseAssignmentExpression();
    }

    return init;
}

/*
 * init-declarator:
 *		declarator
 *      declarator = initializer
 * */
static AstInitDeclarator ParseInitDeclarator(void) {
    AstInitDeclarator initDecl;

    CREATE_AST_NODE(initDecl, InitDeclarator);
    initDecl->dec = ParseDeclarator(DEC_CONCRETE);
    if (CurrentToken == TK_ASSIGN) {
        NEXT_TOKEN;
        initDecl->init = ParseInitializer();
    }

    return initDecl;
}

/*
 *  direct-declarator:
 *		identifier
 *		( declarator )
 *  direct-abstract-declarator:
 *      NULL
 *		( abstract-declarator )
 */
static AstDeclarator ParseDirectDeclarator(int kind) {
    AstDeclarator decl;

    if (CurrentToken == TK_LPAREN) {
        int tmpToken;

        BeginPeekToken();
        tmpToken = GetNextToken();
        if (tmpToken != TK_ID && tmpToken != TK_LPAREN && tmpToken != TK_MUL) {
            EndPeekToken();
            CREATE_AST_NODE(decl, NameDeclarator);
        } else {
            EndPeekToken();
            NEXT_TOKEN;
            decl = ParseDeclarator(kind);
            Expect(TK_RPAREN);
        }
        return decl;
    }

    CREATE_AST_NODE(decl, NameDeclarator);

    if (CurrentToken == TK_ID) {
        if (kind == DEC_ABSTRACT) {
            Error(&TokenCoord, "Identifier is not permitted in the abstract declarator");
        }
        decl->id = TokenValue.p;
        NEXT_TOKEN;
    } else if (kind == DEC_CONCRETE) {
        Error(&TokenCoord, "Expect identifier");
    }

    return decl;
}

/*
 *  parameter-declaration:
 *		declaration-specifiers declarator
 *		declaration-specifiers [abstract-declarator]
 */
static AstParameterDeclaration ParseParameterDeclaration(void) {
    AstParameterDeclaration paramDecl;

    CREATE_AST_NODE(paramDecl, ParameterDeclaration);
    paramDecl->specs = ParseDeclarationSpecifiers();
    paramDecl->dec = ParseDeclarator(DEC_ABSTRACT | DEC_CONCRETE);

    return paramDecl;
}

/*
 *  parameter-type-list:
 *		parameter-list
 *		parameter-list , ...
 *  parameter-list:
 *		parameter-declaration
 *		parameter-list , parameter-declaration
 */
AstParameterTypeList ParseParameterTypeList(void) {
    AstParameterTypeList paramTyList;
    AstNode *tail;

    CREATE_AST_NODE(paramTyList, ParameterTypeList);
    paramTyList->paramDecls = (AstNode) ParseParameterDeclaration();
    tail = &paramTyList->paramDecls->next;
    while (CurrentToken == TK_COMMA) {
        NEXT_TOKEN;
        if (CurrentToken == TK_ELLIPSIS) {
            paramTyList->ellipsis = 1;
            NEXT_TOKEN;
            break;
        }
        *tail = (AstNode) ParseParameterDeclaration();
        tail = &(*tail)->next;
    }

    return paramTyList;
}

/*
 *  postfix-declarator:
 *		direct-declarator
 *		postfix-declarator [ [constant-expression] ]
 *		postfix-declarator ( parameter-type-list)
 *		postfix-declarator ( [identifier-list] )
 *  postfix-abstract-declarator:
 *		direct-abstract-declarator
 *		postfix-abstract-declarator ( [parameter-type-list] )
 *		postfix-abstract-declarator [ [constant-expression] ]
 */
static AstDeclarator ParsePostfixDeclarator(int kind) {
    AstDeclarator dec = ParseDirectDeclarator(kind);

    while (1) {
        if (CurrentToken == TK_LBRACKET) {/* [ */
            AstArrayDeclarator arrDec;

            CREATE_AST_NODE(arrDec, ArrayDeclarator);
            NEXT_TOKEN;
            arrDec->dec = dec;

            if (CurrentToken != TK_RBRACKET) {
                arrDec->expr = ParseConstantExpression();
            }
            Expect(TK_RBRACKET);

            dec = (AstDeclarator) arrDec;
        } else if (CurrentToken == TK_LPAREN) {/* ( */
            AstFunctionDeclarator funcDec;

            CREATE_AST_NODE(funcDec, FunctionDeclarator);
            NEXT_TOKEN;
            funcDec->dec = dec;

            if (IsTypeName(CurrentToken)) {
                funcDec->paramTyList = ParseParameterTypeList();
            } else {
                /*
                 *  [identifier-list]
                 *  direct-abstract-declarator无法推导出identifier-list，但是这些内容放到语义检查阶段进行检查
                 * */
                funcDec->ids = CreateVector(4);
                if (CurrentToken == TK_ID) {
                    INSERT_VECTOR_ITEM(funcDec->ids, TokenValue.p);
                    NEXT_TOKEN;
                    while (CurrentToken == TK_COMMA) {
                        NEXT_TOKEN;
                        if (CurrentToken == TK_ID) {
                            INSERT_VECTOR_ITEM(funcDec->ids, TokenValue.p);
                            NEXT_TOKEN;
                        }
                    }
                }
            }
            Expect(TK_RPAREN);
            dec = (AstDeclarator) funcDec;
        } else {
            return dec;
        }
    }
}

/*
 *  declarator:
 *		[pointer] direct-declarator
 *		direct-declarator
 *
 *  pointer:
 *		* [type-qualifier-list]
 *		* [type-qualifier-list] pointer
 *  type-qualifier-list:
 *      type-qualifier
 *      type-qualifier-list type-qualifier
 *
 *  direct-declarator:
 *		identifier
 *		( declarator )
 *		direct-declarator [ [constant-expression] ]
 *		direct-declarator ( parameter-type-list )
 *		direct-declarator ( [identifier-list] )
 *  parameter-type-list:
 *      parameter-list
 *      parameter-list , ...
 *  parameter-list:
 *      parameter-declaration
 *      parameter-list , parameter-declaration
 *  parameter-declaration:
 *      declaration-specifier declarator
 *      declaration-specifier [abstract-declarator]
 *  abstract-declarator:
 *      pointer
 *		[pointer] direct-abstract-declarator
 *  direct-abstract-declarator:
 *		( abstract-declarator )
 *		[direct-abstract-declarator] [ [constant-expression] ]
 *		[direct-abstract-declarator] ( [parameter-type-list] )
 *  identifier-list:
 *      identifier-list , identifier
 *  改写后的语法:
 *  abstract-declarator:
 *		* [type-qualifier-list] abstract-declarator
 *		postfix-abstract-declarator
 *  postfix-abstract-declarator:
 *		direct-abstract-declarator
 *		postfix-abstract-declarator [ [constant-expression] ]
 *		postfix-abstract-declarator( [parameter-type-list] )
 *  direct-abstract-declarator:
 *		( abstract-declarator )
 *		NULL
 *  declarator:
 *		* [type-qualifier-list] declarator
 *		postfix-declarator
 *  postfix-declarator:
 *		direct-declarator
 *		postfix-declarator [ [constant-expression] ]
 *		postfix-declarator ( parameter-type-list)
 *		postfix-declarator ( [identifier-list] )
 *  direct-declarator:
 *		( declarator )
 *		identifier
 *
 */
/**
 *  @param kind
 *  DEC_CONCRETE: parse a declarator
 *	DEC_ABSTRACT: parse an abstract declarator
 *	DEC_CONCRETE | DEC_ABSTRACT: both of them are ok
 *
 * 	For function declaration:
 * 	    DEC_CONCRETE|DEC_ABSTRACT
 * 	For function definition
 * 	    DEC_CONCRETE
 * 	For type-name, 	sizeof(type-name),	 (type-name)(expr)
 * 	    DEC_ABSTRACT
 * */
static AstDeclarator ParseDeclarator(int kind) {
    if (CurrentToken == TK_MUL) {/* * [type-qualifier-list] */
        AstPointerDeclarator ptrDec;
        AstToken astToken;
        AstNode *tail;

        CREATE_AST_NODE(ptrDec, PointerDeclarator);
        tail = &ptrDec->tyQuals;
        NEXT_TOKEN;

        while (CurrentToken == TK_CONST || CurrentToken == TK_VOLATILE) {
            CREATE_AST_NODE(astToken, Token);
            astToken->token = CurrentToken;
            *tail = (AstNode) astToken;
            tail = &astToken->next;
            NEXT_TOKEN;
        }

        ptrDec->dec = ParseDeclarator(kind);

        return (AstDeclarator) ptrDec;
    }

    return ParsePostfixDeclarator(kind);
}

/*
 *  struct-declarator:
 *		declarator
 *		[declarator] : constant-expression
 */
static AstStructDeclarator ParseStructDeclarator(void) {
    AstStructDeclarator structDecl;

    CREATE_AST_NODE(structDecl, StructDeclarator);
    if (CurrentToken != TK_COLON) {
        structDecl->dec = ParseDeclarator(DEC_CONCRETE);
    }
    if (CurrentToken == TK_COLON) {
        NEXT_TOKEN;
        structDecl->expr = ParseConstantExpression();
    }

    return structDecl;
}

/*
 *  struct-declaration:
 *		specifier-qualifier-list struct-declarator-list ;
 *  specifier-qualifier-list:
 *		type-specifier [specifier-qualifier-list]
 *		type-qualifier [specifier-qualifier-list]
 *  struct-declarator-list:
 *		struct-declarator
 *		struct-declarator-list , struct-declarator
 */
static AstStructDeclaration ParseStructDeclaration(void) {
    AstStructDeclaration structDecl;
    AstNode *tail;
    CREATE_AST_NODE(structDecl, StructDeclaration);

    /*支持空struct-declaration*/
    if (CurrentToken == TK_SEMICOLON) {
        NEXT_TOKEN;
        return NULL;
    }

    /*specifier-qualifier-list*/
    structDecl->specs = ParseDeclarationSpecifiers();
    if (structDecl->specs->stgClasses != NULL) {
        Error(&structDecl->astNodeCoord, "struct or union member should not have storage-class-specifier");
        structDecl->specs->stgClasses = NULL;
    }
    if (structDecl->specs->tyQuals == NULL && structDecl->specs->tySpecs == NULL) {
        Error(&structDecl->astNodeCoord, "Expect type-specifier or type-qualifier");
    }

    /*支持匿名成员*/
    if (CurrentToken == TK_SEMICOLON) {
        NEXT_TOKEN;
        return structDecl;
    }

    /*struct-declarator-list*/
    structDecl->stDecs = (AstNode) ParseStructDeclarator();
    tail = &structDecl->stDecs->next;
    while (CurrentToken == TK_COMMA) {
        NEXT_TOKEN;
        *tail = (AstNode) ParseStructDeclarator();
        tail = &(*tail)->next;
    }

    Expect(TK_SEMICOLON);

    return structDecl;
}

/*
 *  struct-or-union-specifier:
 *		struct-or-union [identifier] { struct-declaration-list }
 *		struct-or-union identifier
 *  struct-or-union:
 *		struct
 *		union
 *  struct-declaration-list:
 *      struct-declaration
 *		struct-declaration-list struct-declaration
 */
static AstStructSpecifier ParseStructOrUnionSpecifier(void) {
    AstStructSpecifier structOrUnionSpec;
    AstNode *tail;

    if (CurrentToken == TK_STRUCT) {
        CREATE_AST_NODE(structOrUnionSpec, StructSpecifier);
    } else {
        CREATE_AST_NODE(structOrUnionSpec, UnionSpecifier);
    }
    NEXT_TOKEN;

    switch (CurrentToken) {
        case TK_ID:
            structOrUnionSpec->id = TokenValue.p;
            NEXT_TOKEN;
            if (CurrentToken == TK_LBRACE)
                goto caseLBRACE;
            /*else*/
                return structOrUnionSpec;
        case TK_LBRACE:
        caseLBRACE:
            NEXT_TOKEN;
            structOrUnionSpec->hasLbrace = 1;
            if (CurrentToken == TK_RBRACE) {
                NEXT_TOKEN;
                return structOrUnionSpec;
            }

            tail = &structOrUnionSpec->stDecls;
            while (IsCurrentTokenIn(FIRST_StructDeclaration)) {
                *tail = (AstNode) ParseStructDeclaration();
                if (*tail != NULL) {
                    tail = &(*tail)->next;
                }
                SkipTo(FIRST_StructDeclarationWithRBrace, "the start of struct-declaration or }");
            }
            Expect(TK_RBRACE);
            return structOrUnionSpec;
        default:
            Error(&TokenCoord, "Expect identifier or { after struct-or-union");
            return structOrUnionSpec;
    }
}

/*
 *  enumerator:
 *		enumeration-constant
 *		enumeration-constant = constant-expression
 * */
static AstEnumerator ParseEnumerator(void) {
    AstEnumerator enumerator;

    CREATE_AST_NODE(enumerator, Enumerator);

    /*enumeration-constant*/
    if (CurrentToken != TK_ID) {
        Error(&TokenCoord, "The enumeration-constant must be identifier");
        return enumerator;
    }/*else*/
    enumerator->id = TokenValue.p;
    NEXT_TOKEN;
    /*constant-expression*/
    if (CurrentToken == TK_ASSIGN) {
        NEXT_TOKEN;
        enumerator->expr = ParseConstantExpression();
    }

    return enumerator;
}

/*
 * enum-specifier
 *		enum [identifier] { enumerator-list [,] }
 *		enum identifier
 *  enumerator-list:
 *		enumerator
 *		enumerator-list , enumerator
 * */
static AstEnumSpecifier ParseEnumSpecifier(void) {
    AstEnumSpecifier enumSpec;
    AstNode *tail;

    CREATE_AST_NODE(enumSpec, EnumSpecifier);
    NEXT_TOKEN;

    if (CurrentToken == TK_ID) {
        enumSpec->id = TokenValue.p;
        NEXT_TOKEN;
    }
    if (CurrentToken == TK_LBRACE) {
        NEXT_TOKEN;

        if (CurrentToken == TK_RBRACE) {
            NEXT_TOKEN;
            Error(&TokenCoord, "Expect identifier before '}' token");
            return enumSpec;
        }
        /*enumerator-list*/
        enumSpec->enumers = (AstNode) ParseEnumerator();
        tail = &enumSpec->enumers->next;
        while (CurrentToken == TK_COMMA) {
            NEXT_TOKEN;
            if (CurrentToken == TK_RBRACE)
                break;
            *tail = (AstNode) ParseEnumerator();
            tail = &(*tail)->next;
        }

        Expect(TK_RBRACE);
    } else if (enumSpec->id == NULL) {
        Error(&TokenCoord, "Expect identifier or { after enum");
    }

    return enumSpec;
}

/*
 *  declaration-specifiers:
 *		storage-class-specifier [declaration-specifiers]
 *		type-specifier [declaration-specifiers]
 *		type-qualifier [declaration-specifiers]
 *  storage-class-specifier:
 *		typedef
 *		extern
 *		static
 *		auto
 *		register
 *  type-specifier:
 *		void
 *		char
 *		short
 *		int
 *		long
 *		float
 *		double
 *		signed
 *		unsigned
 *		struct-or-union-specifier
 *		enum-specifier
 *		typedef-name
 *  type-qualifier:
 *		const
 *		volatile
 */
static AstSpecifiers ParseDeclarationSpecifiers(void) {
    AstSpecifiers specs;
    AstToken astTok;
    AstNode *stgClassTail, *tyQualTail, *tySpecsTail;
    int hasTySpecs = 0;

    CREATE_AST_NODE(specs, Specifiers);

    stgClassTail = &specs->stgClasses;
    tyQualTail = &specs->tyQuals;
    tySpecsTail = &specs->tySpecs;

    while (1) {
        switch (CurrentToken) {
            case TK_TYPEDEF:
            case TK_EXTERN:
            case TK_STATIC:
            case TK_AUTO:
            case TK_REGISTER:
                /*storage-class-specifier:*/
                CREATE_AST_NODE(astTok, Token);
                astTok->token = CurrentToken;
                *stgClassTail = (AstNode) astTok;
                stgClassTail = &astTok->next;
                NEXT_TOKEN;
                break;
            case TK_CONST:
            case TK_VOLATILE:
                /*type-qualifier*/
                CREATE_AST_NODE(astTok, Token);
                astTok->token = CurrentToken;
                *tyQualTail = (AstNode) astTok;
                tyQualTail = &astTok->next;
                NEXT_TOKEN;
                break;
            case TK_VOID:
            case TK_CHAR:
            case TK_SHORT:
            case TK_INT:
            case TK_LONG:
            case TK_FLOAT:
            case TK_DOUBLE:
            case TK_SIGNED:
            case TK_UNSIGNED:
                /*type-specifier*/
                CREATE_AST_NODE(astTok, Token);
                astTok->token = CurrentToken;
                *tySpecsTail = (AstNode) astTok;
                tySpecsTail = &astTok->next;
                hasTySpecs = 1;
                NEXT_TOKEN;
                break;
            case TK_STRUCT:
            case TK_UNION:
                *tySpecsTail = (AstNode) ParseStructOrUnionSpecifier();
                tySpecsTail = &(*tySpecsTail)->next;
                hasTySpecs = 1;
                break;
            case TK_ENUM:
                *tySpecsTail = (AstNode) ParseEnumSpecifier();
                tySpecsTail = &(*tySpecsTail)->next;
                hasTySpecs = 1;
                break;
            case TK_ID:
                if (!hasTySpecs && IsTypedefName(TokenValue.p)) {
                    AstTypedefName astTdName;
                    CREATE_AST_NODE(astTdName, TypedefName);
                    astTdName->id = TokenValue.p;
                    *tySpecsTail = (AstNode) astTdName;
                    tySpecsTail = &astTdName->next;
                    NEXT_TOKEN;
                    hasTySpecs = 1;
                    break;
                }/*else*/
                return specs;
            default:
                return specs;
        }
    }
}

int IsTypeName(int tok) {
    switch (tok) {
        /*type-specifier*/
        case TK_VOID:
        case TK_CHAR:
        case TK_SHORT:
        case TK_INT:
        case TK_LONG:
        case TK_FLOAT:
        case TK_DOUBLE:
        case TK_SIGNED:
        case TK_UNSIGNED:
        case TK_STRUCT:
        case TK_UNION:
        case TK_ENUM:
            return 1;
        case TK_ID:
            return IsTypedefName(TokenValue.p);
        /*type-qualifier*/
        case TK_CONST:
        case TK_VOLATILE:
            return 1;
        /*storage-class-specifier*/
        case TK_TYPEDEF:
        case TK_EXTERN:
        case TK_STATIC:
        case TK_AUTO:
        case TK_REGISTER:
            return 1;
        default:
            return 0;
    }
}

/*
 * type-name:
 *     specifier-qualifier-list [abstract-declarator]
 * */
AstTypeName ParseTypeName(void) {
    AstTypeName tyName;

    CREATE_AST_NODE(tyName, TypeName);
    tyName->specs = ParseDeclarationSpecifiers();
    if (tyName->specs->stgClasses != NULL) {
        Error(&tyName->astNodeCoord, "type name should not have storage class");
        tyName->specs->stgClasses = NULL;
    }
    tyName->dec = ParseDeclarator(DEC_ABSTRACT);

    return tyName;
}

/*
 *  [declaration-specifiers] [init-declarator-list] ;
 *  init-declarator-list:
 *      init-declarator
 *      init-declarator-list , init-declarator
 * */
static AstDeclaration ParseCommonHeader(void) {
    AstDeclaration decl;
    AstNode *tail;

    CREATE_AST_NODE(decl, Declaration);
    decl->specs = ParseDeclarationSpecifiers();
    if (CurrentToken != TK_SEMICOLON) {
        decl->initDecs = (AstNode) ParseInitDeclarator();
        tail = &decl->initDecs->next;
        while (CurrentToken == TK_COMMA) {
            NEXT_TOKEN;
            *tail = (AstNode) ParseInitDeclarator();
            tail = &(*tail)->next;
        }
    }

    return decl;
}

/*
 * declaration:
 *      declaration-specifiers [init-declarator-list] ;
 * */
AstDeclaration ParseDeclaration(void) {
    AstDeclaration decl;

    decl = ParseCommonHeader();
    Expect(TK_SEMICOLON);
    PreCheckTypedef(decl);

    return decl;
}

/*
 * declarator被视为init-declarator-list处理，这里对其进行检查
 * */
static AstFunctionDeclarator GetFunctionDeclarator(AstInitDeclarator initDec) {
    AstDeclarator decl;

    if (initDec == NULL || initDec->next != NULL || initDec->init != NULL)
        return NULL;

    decl = initDec->dec;
    while (decl) {
        if (decl->astNodeKind == NK_FunctionDeclarator
                && decl->dec && decl->dec->astNodeKind == NK_NameDeclarator) {
            break;
        }
        decl = decl->dec;
    }

    return (AstFunctionDeclarator) decl;
}

/*
 *  external-declaration:
 *	    function-definition
 *		declaration
 *	function-definition:
 *		[declaration-specifiers] declarator [declaration-list] compound-statement
 *  declaration:
 *		declaration-specifiers [init-declarator-list] ;
 *	这里暂时将function-definition与declaration的产生式视作拥有公共前缀
 *	    [declaration-specifiers] [init-declarator-list]
 *      后续再进行处理
 * */
static AstNode ParseExternalDeclaration(void) {
    AstDeclaration decl = NULL;
    AstInitDeclarator initDecl = NULL;
    AstFunctionDeclarator functionDecl;

    decl = ParseCommonHeader();
    initDecl = (AstInitDeclarator) decl->initDecs;
    if (decl->specs->stgClasses != NULL && ((AstToken) decl->specs->stgClasses)->token == TK_TYPEDEF)
        goto not_func;
    functionDecl = GetFunctionDeclarator(initDecl);
    if (functionDecl != NULL) {
        AstFunction func;
        AstNode *tail;
        if (CurrentToken == TK_SEMICOLON) {/*declaration*/
            NEXT_TOKEN;
            if (CurrentToken != TK_LBRACE)
                return (AstNode) decl;
            Error(&decl->astNodeCoord, "expected identifier before ‘{’ token");
        } else if (functionDecl->paramTyList && CurrentToken != TK_LBRACE) {
            goto not_func;
        }

        CREATE_AST_NODE(func, Function);
        func->astNodeCoord = decl->astNodeCoord;
        func->specs = decl->specs;
        func->dec = initDecl->dec;
        func->fdec = functionDecl;

        Level++;
        if (func->fdec->paramTyList) {
            /*检查形参是否和typedef name冲突*/
            AstNode paramDecl = func->fdec->paramTyList->paramDecls;
            while (paramDecl) {
                CheckTypedefName(TK_BEGIN, GetOutermostID(((AstParameterDeclaration) paramDecl)->dec));
                paramDecl = paramDecl->next;
            }
        }

        /*
         * [declaration-list]
         * 用于int fun(a,b,c)int a,b;double c;{}这样的函数定义
         * PreCheckTypedef()在ParseDeclaration()中进行
         * */
        tail = &func->decls;
        while (IsCurrentTokenIn(FIRST_Declaration)) {
            *tail = (AstNode) ParseDeclaration();
            tail = &(*tail)->next;
        }
        Level--;

        func->stmt = ParseCompoundStatement();

        return (AstNode) func;
    }

    not_func:
    if (!decl->specs->stgClasses && !decl->specs->tyQuals && !decl->specs->tySpecs) {
        Warning(&decl->astNodeCoord, "declaration specifier missing, defaulting to 'int'");
    }
    Expect(TK_SEMICOLON);
    PreCheckTypedef(decl);

    return (AstNode) decl;
}

/*
 *  translation-unit:
 *		external-declaration
 *		translation-unit external-declaration
 * */
AstTranslationUnit ParseTranslationUnit(char *file) {
    AstTranslationUnit translationUnit;
    AstNode *tailPtr;

    ReadSourceFile(file);
    TokenCoord.src_filename = file;
    TokenCoord.src_line = TokenCoord.pp_line = TokenCoord.col = 1;
    TypedefNames = CreateVector(8);
    OverloadNames = CreateVector(8);
    CREATE_AST_NODE(translationUnit, TranslationUnit);
    tailPtr = &translationUnit->externalDeclarations;

    NEXT_TOKEN;
    while (CurrentToken != TK_END) {
        *tailPtr = ParseExternalDeclaration();
        tailPtr = &(*tailPtr)->next;
        SkipTo(FIRST_ExternalDeclaration, "the beginning of external-declaration");
    }

    CloseSourceFile();

    return translationUnit;
}
