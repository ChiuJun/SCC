//
// Created by LJChi on 2021/4/11.
//

#ifndef SCC_TYPE_H
#define SCC_TYPE_H

/*
 * FLOAT之前为整数类型，其中LONGLONG与它的unsigned形式为常见扩展(Common extensions)
 * FLOAT至LONGDOUBLE为浮点类型
 * LONGDOUBLE之后的为衍生类型，VOID为特殊的不完整类型，VOID之后为聚合类型
 * */
enum {
    CHAR, UCHAR, SHORT, USHORT, INT, UINT, LONG, ULONG, LONGLONG, ULONGLONG, ENUM,
    FLOAT, DOUBLE, LONGDOUBLE,
    POINTER, VOID, UNION, STRUCT, ARRAY, FUNCTION
};

/*
 * 机器层面的类型
 * I1: signed 1 byte       U1: unsigned 1 byte
 * I2: signed 2 byte       U2: unsigned 2 byte
 * I4: signed 4 byte       U4: unsigned 4 byte
 * F4: 4 byte floating     F8: 8 byte floating
 * V: no type              B: memory block, used for struct/union and array.
 * */
enum {
    I1, U1, I2, U2, I4, U4,
    F4, F8,
    V, B
};

/*
 * 类型限定符
 * */
enum {
    CONST = 0x1,
    VOLATILE = 0X2
};

/*
 * category:类型 对应enum中的CHAR...FUNCTION
 * qualify:类型限定符 对应enum中的CONST、VOLATILE
 * align:类型对齐要求
 * size:类型大小
 * base_type:衍生类型对应的基本类型，对于基本类型值为NULL
 * */
#define TYPE_COMMON     \
    int category : 8;   \
    int qualify : 8;    \
    int align   : 16;   \
    int size;           \
    struct type *base_type;

typedef struct type {
    TYPE_COMMON
} *Type;

typedef struct arrayType {
    TYPE_COMMON
    /*数组长度*/
    int len;
} *ArrayType;

typedef struct field {
    char *field_id;
    int offset;
    /*位域成员使用：大小*/
    int bits;
    /*位域成员使用：偏移bit数*/
    int pos;
    Type field_type;
    struct field *next_field_ptr;
} *Field;

typedef struct recordType {
    TYPE_COMMON
    char *record_id;
    Field record_fields;
    Field *tail_field_ptr;
    /*是否包含constant field*/
    int hasConstField: 16;
    /*是否包含flexible array*/
    int hasFlexArray: 16;
    int isComplete;
} *RecordType;

typedef struct enumType {
    TYPE_COMMON
    char *enum_id;
    int isComplete;
} *EnumType;

typedef struct parameter{
    char *parameter_id;
    Type parameter_type;
    int isRegister;
} *Parameter;

typedef struct signature{
    int hasProto : 16;
    int hasEllipsis :16;
    Vector params;
} *Signature;

typedef struct functionType{
    TYPE_COMMON
    Signature sig;
} *FunctionType;

/*取Types数组对应的struct*/
#define T(category) (Types + category)

#define IsIntegerType(ty)   (ty->category <= ENUM)
#define IsUnsigned(ty)      (ty->category & 0x1)
#define IsFPType(ty)        (ty->category >= FLOAT && ty->category <= LONGDOUBLE)
#define IsArithType(ty)     (ty->category <= LONGDOUBLE)
/*算数类型和指针类型称为标量类型*/
#define IsScalarType(ty)    (ty->category <= POINTER)
#define IsPtrType(ty)      (ty->category == POINTER)
#define IsRecordType(ty)   (ty->category == STRUCT || ty->category == UNION)
#define IsFunctionType(ty) (ty->category == FUNCTION)

#define IsObjectPtr(ty)     (ty->category == POINTER && ty->base_type->category != FUNCTION)
#define IsIncompletePtr(ty) (ty->category == POINTER && ty->base_type->size == 0)
#define IsVoidPtr(ty)       (ty->category == POINTER && ty->base_type->category == VOID)
#define IsNotFunctionPtr(ty)  (ty->category == POINTER && ty->base_type->category != FUNCTION)

#define BothIntegerType(ty1, ty2)   (IsIntegerType(ty1) && IsIntegerType(ty2))
#define BothArithType(ty1, ty2)     (IsArithType(ty1) && IsArithType(ty2))
#define BothScalarType(ty1, ty2)    (IsScalarType(ty1) && IsScalarType(ty2))
#define IsCompatiblePtr(ty1, ty2)   (IsPtrType(ty1) && IsPtrType(ty2) && IsCompatibleType(RemoveQualify(ty1->base_type), RemoveQualify(ty2->base_type)))

#define	IGNORE_ZERO_SIZE_ARRAY	1

const char * GetCategoryName(int category);
int GetTypeCode(Type ty);
char* TypeToString(Type ty);

int IsIncompleteEnum(Type ty);
int IsIncompleteRecord(Type ty);
int IsZeroSizeArray(Type ty);
int IsIncompleteType(Type ty, int ignoreZeroArray);

/*static Type DoTypeClone(Type ty);*/
Type Qualify(int qualify, Type ty);
Type RemoveQualify(Type ty);
Type Enum(char *id);
Type PointerTo(Type ty);
Type ArrayOf(int len, Type ty);
Type FunctionReturn(Type ty, Signature sig);
Type Promote(Type ty);
Type AdjustParameter(Type ty);

Type  StartRecord(char *id, int category);
Field AddField(Type ty, char *id, Type fieldTy, int bits);
Field LookupField(Type ty, char *id);
void AddOffset(RecordType rty, int offset);
void EndRecord(Type ty, Coord coord);

/*static int IsCompatibleFunction(FunctionType fty1, FunctionType fty2);*/
int  IsCompatibleType(Type ty1, Type ty2);
Type CompositeType(Type ty1, Type ty2);
Type CommonRealType(Type ty1, Type ty2);

void SetupTypeSystem(void);

extern Type DefaultFunctionType;
extern Type WCharType;
extern struct type Types[VOID - CHAR + 1];

#endif //SCC_TYPE_H
