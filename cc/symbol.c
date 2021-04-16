//
// Created by LJChi on 2021/4/15.
//

#include "cc.h"

#define SEARCH_OUTER_TABLE  1

/*所有的函数符号*/
Symbol Functions;
/*所有全局变量以及static类型变量*/
Symbol Globals;
/*所有字符串*/
Symbol Strings;
/*所有的浮点数字面量*/
Symbol FloatConstants;
/*全局作用域Level 0，进入新的作用域则递增*/
int Level;
/*临时变量计数*/
int TempNum;
/*标号计数*/
int LabelNum;

static const char *symbolKindName[] = {
        "SK_Tag", "SK_TypedefName", "SK_EnumConstant", "SK_Constant", "SK_Variable", "SK_Temp",
        "SK_Offset", "SK_String", "SK_Label", "SK_Function", "SK_Register", "SK_IRegister",
        "SK_NotAvailable"
};
/*字符串计数*/
static int StringNum;
/*全局符号表，用于struct、union、enum的名字*/
static struct table GlobalTags;
/*全局符合表，用去其他标识符*/
static struct table GlobalIDs;
/*全局符合表，用于常量*/
static struct table Constants;
/*当前作用域的struct、union、enum名*/
static Table Tags;
/*当前作用域的其他标识符*/
static Table Identifiers;
/*二级指针 用于构造链表*/
static Symbol *FunctionTail, *GlobalTail, *StringTail, *FloatTail;
/*
 * 不知道干啥的，奇奇怪怪的语法
 * see examples/scope/parameterList.c
 * */
static int inParameterList = 0;
static Table savedIdentifiers, savedTags;

/*
 * 初始化符号表
 * 每次编译一个新的源文件都会调用该函数
 * */
void InitSymbolTable(void) {
    int size = sizeof(Symbol) * (SYM_HASH_MASK + 1);

    GlobalTags.buckets = GlobalIDs.buckets = NULL;
    Constants.buckets = HeapAllocate(CurrentHeap, size);
    memset(Constants.buckets, 0, size);
    GlobalTags.level = GlobalIDs.level = Constants.level = 0;
    GlobalTags.outer = GlobalIDs.outer = Constants.outer = NULL;

    Tags = &GlobalTags;
    Identifiers = &GlobalIDs;

    Functions = Globals = Strings = FloatConstants = NULL;
    FunctionTail = &Functions;
    GlobalTail = &Globals;
    StringTail = &Strings;
    FloatTail = &FloatConstants;

    StringNum = Level = TempNum = LabelNum = 0;
}

/*
 * 进入新的作用域，创建该作用域的符号表
 * */
void EnterScope(void) {
    Table t;
    Level++;

    ALLOC(t);
    t->buckets = NULL;
    t->level = Level;
    t->outer = Identifiers;
    Identifiers = t;

    ALLOC(t);
    t->buckets = NULL;
    t->level = Level;
    t->outer = Tags;
    Tags = t;
}

/*
 * 离开某个作用域
 * */
void ExitScope(void) {
    Level--;
    Identifiers = Identifiers->outer;
    Tags = Tags->outer;
}

/*
 * 获取enum SK_Tag...SK_IRegister的名字
 * */
const char *GetSymbolKind(int kind) {
    return symbolKindName[kind];
}

static Symbol DoLookupSymbol(Table tbl, char *name, int searchOuter) {
    unsigned int hashVal = (unsigned int) name & SYM_HASH_MASK;
    BucketLinker linker;

    do {
        if (tbl->buckets != NULL) {
            for (linker = (BucketLinker) tbl->buckets[hashVal]; linker != NULL; linker = linker->link) {
                if (linker->sym->name == name) {
                    return linker->sym;
                }
            }
        }
        tbl = tbl->outer;
    } while (tbl != NULL && searchOuter);

    return NULL;
}

static Symbol LookupSymbol(Table tbl, char *name) {
    return DoLookupSymbol(tbl, name, SEARCH_OUTER_TABLE);
}

Symbol LookupID(char *name) {
    return LookupSymbol(Identifiers, name);
}

Symbol LookupTag(char *name) {
    return LookupSymbol(Tags, name);
}

/*
 * 在 @param tbl 指定的符号表中添加 @sym 指定的符号
 * */
static Symbol AddSymbol(Table tbl, Symbol sym) {
    unsigned int hashVal = (unsigned int) sym->name & SYM_HASH_MASK;
    BucketLinker linker;
    CALLOC(linker);
    if (tbl->buckets == NULL) {
        int size = sizeof(Symbol) * (SYM_HASH_MASK + 1);
        tbl->buckets = HeapAllocate(CurrentHeap, size);
        memset(tbl->buckets, 0, size);
    }

    linker->link = (BucketLinker)tbl->buckets[hashVal];
    tbl->buckets[hashVal] = (Symbol)linker;;
    linker->sym = sym;
    sym->level = tbl->level;

    return sym;
}

/*
 * 添加SK_Tag类型的符号至当前Tags符号表
 * */
Symbol AddTag(char *name, Type ty, Coord pcoord) {
    Symbol ptr;
    CALLOC(ptr);

    ptr->symbol_kind = SK_Tag;
    ptr->name = name;
    ptr->symbol_type = ty;
    ptr->pcoord = pcoord;

    if (IsInParameterList()) {
        Warning(pcoord,
                "’%s %s‘ declared inside parameter list will not be visible outside of this definition or declaration",
                GetCategoryName(ty->category), name ? name : "<anonymous>");
    }

    return AddSymbol(Tags, ptr);
}

/*
 * 添加SK_TypedefName类型的符号至当前Identifiers符号表
 * */
Symbol AddTypedefName(char *name, Type ty, Coord pcoord) {
    Symbol ptr;
    CALLOC(ptr);

    ptr->symbol_kind = SK_TypedefName;
    ptr->name = name;
    ptr->symbol_type = ty;
    ptr->pcoord = pcoord;

    return AddSymbol(Identifiers, ptr);
}

/*
 * 添加SK_EnumConstant类型的符号至当前Identifiers符号表
 * 如果是浮点数 添加至FloatConstants链表
 * */
Symbol AddEnumConstant(char *name, Type ty, int val, Coord pcoord) {
    Symbol ptr;
    CALLOC(ptr);

    ptr->symbol_kind = SK_EnumConstant;
    ptr->name = name;
    ptr->symbol_type = ty;
    ptr->val.i[0] = val;
    ptr->pcoord = pcoord;

    return AddSymbol(Identifiers, ptr);
}

/*
 * 添加SK_Constant类型的符号至Constants符号表 包括整型、浮点型、指针
 * */
Symbol AddConstant(Type ty, union value val) {
    unsigned int hashVal = (unsigned int) val.i[0] & SYM_HASH_MASK;
    Symbol ptr;

    ty = RemoveQualify(ty);
    /* TODO:问题
     * 所有整数型处理成int？
     * LONGDOUBLE型处理成double? 构造语法树的时候浮点数给的都是long double？
     * */
    if (IsIntegerType(ty)) {
        ty = T(INT);
    } else if (IsPtrType(ty)) {
        ty = T(POINTER);
    } else if (ty->category == LONGDOUBLE) {
        ty = T(DOUBLE);
    }
    for (ptr = Constants.buckets[hashVal]; ptr != NULL; ptr = ptr->link) {
        if (ptr->symbol_type == ty && ptr->val.i[0] == val.i[0] && ptr->val.i[1] == val.i[1])
            return ptr;
    }

    CALLOC(ptr);
    ptr->symbol_kind = SK_Constant;
    switch (ty->category) {
        case INT:
            ptr->name = FormatName("%d", val.i[0]);
            break;
        case POINTER:
            if (val.i[0] == 0) {
                ptr->name = "0";
            } else {
                ptr->name = FormatName("0x%x", val.i[0]);
            }
            break;
        case FLOAT:
            ptr->name = FormatName("%g", val.f);
            break;
        case DOUBLE:
            ptr->name = FormatName("%g", val.d);
            break;
        default:
            Fatal("unknown category");
    }
    ptr->symbol_type = ty;
    ptr->pcoord = FSYM->pcoord;
    ptr->storage_class = TK_STATIC;
    ptr->val = val;

    ptr->link = Constants.buckets[hashVal];
    Constants.buckets[hashVal] = ptr;

    if (ty->category == FLOAT || ty->category == DOUBLE) {
        *FloatTail = ptr;
        FloatTail = &ptr->next;
    }

    return ptr;
}

Symbol IntConstant(int i) {
    union value val;

    val.i[0] = i;
    val.i[1] = 0;

    return AddConstant(T(INT), val);
}

/*
 * 添加SK_Variable类型的符号
 * 在当前的Identifiers符号表添加，如果是extern类型变量，在符号表GlobalIDs添加
 * 如果是static修饰的变量或者全局变量还将在Globals链表添加，否则在当前函数的locals链表添加
 * */
Symbol AddVariable(char *name, Type ty, int storage_class, Coord pcoord) {
    VariableSymbol ptr;
    CALLOC(ptr);

    ptr->symbol_kind = SK_Variable;
    ptr->name = name;
    ptr->symbol_type = ty;
    ptr->storage_class = storage_class;
    ptr->pcoord = pcoord;
    if (Level == 0 || storage_class == TK_STATIC) {
        *GlobalTail = (Symbol) ptr;
        GlobalTail = &ptr->next;
    } else if (storage_class != TK_EXTERN) {
        *FSYM->lastLocalsVar = (Symbol) ptr;
        FSYM->lastLocalsVar = &ptr->next;
    }
    if (storage_class == TK_EXTERN && Identifiers != &GlobalIDs) {
        AddSymbol(&GlobalIDs, (Symbol)ptr);
    }

    return AddSymbol(Identifiers, (Symbol)ptr);
}

/*
 * 添加SK_Temp类型的符号至当前函数的locals链表
 * 属于变量类型VariableSymbol
 * */
Symbol CreateTemp(Type ty) {
    VariableSymbol ptr;
    CALLOC(ptr);

    ptr->symbol_kind = SK_Temp;
    /*临时变量取名为t0...tn*/
    ptr->name = FormatName("t%d", TempNum++);
    ptr->symbol_type = ty;
    ptr->level = 1;
    ptr->pcoord = FSYM->pcoord;

    *FSYM->lastLocalsVar = (Symbol)ptr;
    FSYM->lastLocalsVar = &ptr->next;

    return (Symbol)ptr;
}

/*
 * 创建SK_Offset类型的符号，用于访问数据成员或者数组
 * */
Symbol CreateOffset(Type ty, Symbol base, int constant_offset, Coord pcoord) {
    VariableSymbol ptr;

    if (constant_offset == 0 && (IsArithType(base->symbol_type) || (ty == base->symbol_type))) {
        return base;
    }

    CALLOC(ptr);
    if (base->symbol_kind == SK_Offset) {
        constant_offset += AsVar(base)->offset;
        base = base->link;
    }
    base->ref++;
    ptr->symbol_kind = SK_Offset;
    ptr->name = FormatName("%s[%d]", base->name, constant_offset);
    ptr->symbol_type = ty;
    ptr->addressed = 1;
    ptr->link = base;
    ptr->pcoord = pcoord;
    ptr->offset = constant_offset;

    return (Symbol)ptr;
}

/*
 * 创建SK_String类型的符号，添加至Strings链表
 * */
Symbol AddString(Type ty, String str, Coord pcoord) {
    Symbol ptr;
    CALLOC(ptr);

    ptr->symbol_kind = SK_String;
    ptr->name = FormatName("str%d", StringNum++);
    ptr->symbol_type = ty;
    ptr->storage_class = TK_STATIC;
    ptr->val.p = str;
    ptr->pcoord = pcoord;

    *StringTail = ptr;
    StringTail = &ptr->next;

    return ptr;
}

/*
 * 创建SK_Label类型的符号，直接返回
 * */
Symbol CreateLabel(void) {
    Symbol ptr;
    CALLOC(ptr);

    ptr->symbol_kind = SK_Label;
    ptr->name = FormatName("BB%d", LabelNum++);
    ptr->pcoord = FSYM->pcoord;

    return ptr;
}

/*
 * 创建SK_Function类型的符号，添加到当前Identifiers符号表与GlobalIDs符号表
 * 添加到Functions链表
 * */
Symbol AddFunction(char *name, Type ty, int storage_class, Coord pcoord) {
    FunctionSymbol ptr;
    CALLOC(ptr);

    ptr->symbol_kind = SK_Function;
    ptr->name = name;
    ptr->symbol_type = ty;
    ptr->storage_class = storage_class;
    ptr->lastLocalsVar = &ptr->params;
    ptr->pcoord = pcoord;

    *FunctionTail = (Symbol) ptr;
    FunctionTail = &ptr->next;

    if (Identifiers != &GlobalIDs) {
        AddSymbol(Identifiers, (Symbol) ptr);
    }
    return AddSymbol(&GlobalIDs, (Symbol) ptr);
}

int IsInParameterList(void) {
    return inParameterList;
}

void EnterParameterList(void) {
    inParameterList = 1;
    EnterScope();
}

void LeaveParameterList(void) {
    inParameterList = 0;
    ExitScope();
}

void SaveParameterListTable(void) {
    savedIdentifiers = Identifiers;
    savedTags = Tags;
}

void RestoreParameterListTable(void) {
    Level++;
    savedIdentifiers->level = Level;
    savedIdentifiers->outer = Identifiers;
    Identifiers = savedIdentifiers;

    savedTags->level = Level;
    savedTags->outer = Tags;
    Tags = savedTags;
}