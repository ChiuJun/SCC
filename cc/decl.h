//
// Created by LJChi on 2021/5/13.
//

#ifndef SCC_DECL_H
#define SCC_DECL_H

enum {
    DEC_ABSTRACT = 0x01, DEC_CONCRETE = 0x02
};

#define AST_DECLARATOR_COMMON   \
    AST_NODE_COMMON             \
    struct astDeclarator *dec;  \
    char *id;                   \
    TypeDerivList tyDrvList;

/*
 * typedef name
 * */
typedef struct tdname {
    char *id;
    int level;
    int overload;
    int overloadLevel;
} *TDName;

/**
	Type Derivation
		ctor:	type constructor, can be array,function, pointer
		len:		array length
		qual:	pointer qualifier
		sig:		function signature
		next:	pointer to next type derivation list

	int * a[5];
		array of 5 pointer to int

		astPointerDeclarator --> astArrayDeclarator(5) -->  astDeclarator(a)

	The above syntax tree will be produced during syntax parsing.
	The declaration's binding order is similar as operator's priority.
	Identifier's priority is highest, belongs to direct declarator.
	Array and function's priority is same, belongs to array declarator and
	function declarator respectively.
	Pointer's priority is lowest, belongs to pointer declarator.



	During semantic check ucc will construct the type derivation list
	from bottom up.
	(pointer to) -->(array of 5) --> NIL

	function  DeriveType(...) will apply the type derivation in the type
	derivation list from left to right for the base type ty,
	which in essence is in the order of type constructor's priority.
	For int * a[5], the result of type derivation is
		"array of 5 pointer to int"
	From left to right:
		base type is int
		(1)  use "pointer to", we have "pointer to int"
		(2)  use "array of 5", we have "array of 5" "ponter to int",
			that is , "array of 5 pointer to int"
*/
enum {
    POINTER_TO, ARRAY_OF, FUNCTION_RETURN
};

typedef struct typeDerivList {
    int ctor;        // type constructor		pointer/function/array
    union {
        int len;    // array size
        int qual;    // pointer qualifier
        Signature sig;    // function signature
    };
    struct typeDerivList *next;
} *TypeDerivList;

typedef struct astSpecifiers *AstSpecifiers;

typedef struct astDeclarator {
    AST_DECLARATOR_COMMON
} *AstDeclarator;

/*
 *  initializer:
 *		assignment-expression
 *		{ initializer-list }
 *		{ initializer-list , }
 *  initializer-list:
 *		initializer
 *		initializer-list, initializer
 */
typedef struct astInitializer {
    AST_NODE_COMMON
    int lbrace;
    union {
        AstNode initials;/*initializer-list*/
        AstExpression expr;/*assignment-expression*/
    };
    InitData idata;
} *AstInitializer;

/*
 * init-declarator:
 *		declarator
 *      declarator = initializer
 * */
typedef struct astInitDeclarator {
    AST_NODE_COMMON
    AstDeclarator dec;
    AstInitializer init;
} *AstInitDeclarator;

/*
 *  parameter-declaration:
 *		declaration-specifiers declarator
 *		declaration-specifiers [abstract-declarator]
 */
typedef struct astParameterDeclaration {
    AST_NODE_COMMON
    AstSpecifiers specs;
    AstDeclarator dec;
} *AstParameterDeclaration;

/*
 *  parameter-type-list:
 *		parameter-list
 *		parameter-list , ...
 *  parameter-list:
 *		parameter-declaration
 *		parameter-list , parameter-declaration
 */
typedef struct astParameterTypeList {
    AST_NODE_COMMON
    AstNode paramDecls;
    int ellipsis;
} *AstParameterTypeList;

typedef struct astFunctionDeclarator {
    AST_DECLARATOR_COMMON
    /*[identifier-list]*/
    Vector ids;
    AstParameterTypeList paramTyList;
    int partOfDef;
    Signature sig;
} *AstFunctionDeclarator;

typedef struct astArrayDeclarator {
    AST_DECLARATOR_COMMON
    AstExpression expr;
} *AstArrayDeclarator;

/*
 *  declarator:
 *		* [type-qualifier-list] declarator
 *		postfix-declarator
 *  abstract-declarator:
 *		* [type-qualifier-list] abstract-declarator
 *		postfix-abstract-declarator
 *  type-qualifier-list:
 *      type-qualifier
 *      type-qualifier-list type-qualifier
 * */
typedef struct astPointerDeclarator {
    AST_DECLARATOR_COMMON
    AstNode tyQuals;
} *AstPointerDeclarator;

/*
 *  struct-declarator:
 *		declarator
 *		[declarator] : constant-expression
 */
typedef struct astStructDeclarator {
    AST_NODE_COMMON
    AstDeclarator dec;
    AstExpression expr;
} *AstStructDeclarator;

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
typedef struct astStructDeclaration {
    AST_NODE_COMMON
    AstSpecifiers specs;
    AstNode stDecs;
} *AstStructDeclaration;

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
typedef struct astStructSpecifier {
    AST_NODE_COMMON
    char *id;
    AstNode stDecls;
    int hasLbrace;
} *AstStructSpecifier;

/*
 *  enumerator:
 *		enumeration-constant
 *		enumeration-constant = constant-expression
 * */
typedef struct astEnumerator {
    AST_NODE_COMMON
    char *id;
    AstExpression expr;
} *AstEnumerator;

/*
 * enum-specifier
 *		enum [identifier] { enumerator-list [,] }
 *		enum identifier
 *  enumerator-list:
 *		enumerator
 *		enumerator-list , enumerator
 * */
typedef struct astEnumSpecifier {
    AST_NODE_COMMON
    char *id;
    AstNode enumers;
} *AstEnumSpecifier;

typedef struct astTypedefName {
    AST_NODE_COMMON
    char *id;
    Symbol sym;
} *AstTypedefName;

typedef struct astToken {
    AST_NODE_COMMON
    int token;
} *AstToken;

/*
 *  declaration-specifiers:
 *		storage-class-specifier [declaration-specifiers]
 *		type-specifier [declaration-specifiers]
 *		type-qualifier [declaration-specifiers]
 *	诸如static const int
 */
struct astSpecifiers {
    AST_NODE_COMMON
    AstNode stgClasses;
    AstNode tyQuals;
    AstNode tySpecs;

    int sclass;
    Type ty;
};

/*
 * type-name:
 *     specifier-qualifier-list [abstract-declarator]
 * */
struct astTypeName {
    AST_NODE_COMMON
    AstSpecifiers specs;
    AstDeclarator dec;
};

/*
 * declaration:
 *      declaration-specifiers [init-declarator-list] ;
 * */
struct astDeclaration {
    AST_NODE_COMMON
    AstSpecifiers specs;
    AstNode initDecs;
};

/*
 *  function-definition:
 *      [declaration-specifiers] declarator [declaration-list] compound-statement
 * */
typedef struct astFunction {
    AST_NODE_COMMON
    /*[declaration-specifiers]*/
    AstSpecifiers specs;
    /*declarator*/
    AstDeclarator dec;

    AstFunctionDeclarator fdec;

    /*[declaration-list]*/
    /*用于int fun(a,b,c)int a,b;double c;{}这样的函数定义*/
    AstNode decls;
    /*compound-statement*/
    AstStatement stmt;
    FunctionSymbol funSymbol;
    Label labels;
    Vector loops;
    Vector swtches;
    Vector breakable;
    int hasReturn;
} *AstFunction;

struct astTranslationUnit {
    AST_NODE_COMMON
    AstNode externalDeclarations;
};

void CheckLocalDeclaration(AstDeclaration decl, Vector v);
Type CheckTypeName(AstTypeName tname);

extern AstFunction CURRENTF;
#define	IsRecordSpecifier(spec)	(spec->astNodeKind == NK_StructSpecifier || spec->astNodeKind == NK_UnionSpecifier)

#endif //SCC_DECL_H
