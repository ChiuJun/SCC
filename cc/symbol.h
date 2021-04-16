//
// Created by LJChi on 2021/4/15.
//

#ifndef SCC_SYMBOL_H
#define SCC_SYMBOL_H

#define SYM_HASH_MASK 127

typedef struct basicBlock *BBlock;
typedef struct initData *InitData;

/*符号类型*/
enum {
    SK_Tag, SK_TypedefName, SK_EnumConstant, SK_Constant, SK_Variable, SK_Temp,
    SK_Offset, SK_String, SK_Label, SK_Function, SK_Register, SK_IRegister
};


enum{
    TK_AUTO, TK_EXTERN, TK_REGISTER, TK_STATIC
};

 /*
  * symbol_kind 符号类型 SK_Tag...SK_IRegister
  * name 符号的标识符即名字
  * access_name 用于汇编代码生成，对应符号在汇编代码中的名字
  * symbol_type 符号的类型 参见类型子系统
  * level 符号所处的作用域层级 全局为0往后递增
  * storage_class 存储方式 TK_AUTO...TK_STATIC
  *
  * */
#define SYMBOL_COMMON       \
    int symbol_kind;        \
    char *name;             \
    char *access_name;      \
    Type symbol_type;       \
    int level;              \
    int storage_class;      \
    int ref;                \
    int defined : 1;        \
    int addressed : 1;      \
    int need_writeback : 1; \
    int unused : 29;        \
    union value val;        \
    struct symbol *reg;     \
    struct symbol *link;    \
    struct symbol *next;    \
    Coord pcoord;

typedef struct symbol {
    SYMBOL_COMMON
} *Symbol;

/*
 * 用于SCC内部计算的临时变量
 * 描述临时变量的定义信息
 * */
typedef struct valueDef {
    Symbol src1;
    int op;
    Symbol src2;
    Symbol dst;
    BBlock ownBB;
    struct valueDef *nextValDef;
} *ValueDef;

/*
 * 用于SCC内部计算的临时变量
 * 描述变量被引用至哪个临时变量的信息
 * */
typedef struct valueUse {
    ValueDef def;
    struct valueUse *nextValUse;
} *ValueUse;

typedef struct variableSymbol {
    SYMBOL_COMMON
    /*用于记录变量的初始化信息*/
    InitData data;
    /*当该变量是临时变量时指向定义信息*/
    ValueDef def;
    /*当该变量是普通变量时指向被引用的信息*/
    ValueUse uses;
    /*用于结构体成员*/
    int offset;
} *VariableSymbol;

typedef struct functionSymbol {
    SYMBOL_COMMON
    /*用于记录参数列表*/
    Symbol params;
    /*用于记录函数内部的局部变量*/
    Symbol locals;
    /*二级指针，用于更新locals链表*/
    Symbol *lastLocalsVar;
    /*基本块的数量*/
    int nBBlock;
    /*入口基本块*/
    BBlock entryBB;
    /*出口基本块*/
    BBlock exitBB;
    /*哈希表 用于快速查找临时变量*/
    ValueDef valNumTable[16];
} *FunctionSymbol;

/*每一个作用域的符号表*/
typedef struct table {
    /*二级指针 指向哈希表*/
    Symbol *buckets;
    /*表示作用域 全局符号表为0 往后递推*/
    int level;
    /*指向上一级符号表*/
    struct table *outer;
} *Table;

typedef struct bucketLinker {
    struct bucketLinker *link;
    Symbol sym;
} *BucketLinker;

#define AsVar(sym)  ((VariableSymbol)sym)

void InitSymbolTable(void);
void EnterScope(void);
void ExitScope(void);
const char *GetSymbolKind(int kind);
Symbol LookupID(char *id);
Symbol LookupTag(char *id);

Symbol AddTag(char *id, Type ty, Coord pcoord);
Symbol AddTypedefName(char *id, Type ty, Coord pcoord);
Symbol AddEnumConstant(char *id, Type ty, int val, Coord pcoord);
Symbol AddConstant(Type ty, union value val);
Symbol IntConstant(int i);
Symbol AddVariable(char *id, Type ty, int storage_class, Coord pcoord);
Symbol CreateTemp(Type ty);
Symbol CreateOffset(Type ty, Symbol base, int constant_offset, Coord pcoord);
Symbol AddString(Type ty, String str, Coord pcoord);
Symbol CreateLabel(void);
Symbol AddFunction(char *id, Type ty, int storage_class, Coord pcoord);

int IsInParameterList(void);
void EnterParameterList(void);
void LeaveParameterList(void);
void SaveParameterListTable(void);
void RestoreParameterListTable(void);

extern int Level;
extern int TempNum;
extern int LabelNum;
extern Symbol Functions;
extern Symbol Globals;
extern Symbol Strings;
extern Symbol FloatConstants;

/*语义检查模块中定义*/
extern FunctionSymbol FSYM;

#endif //SCC_SYMBOL_H
