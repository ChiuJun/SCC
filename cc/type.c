//
// Created by LJChi on 2021/4/11.
//

#include "cc.h"
#include "config.h"

/*
 * 基本类型
 * 实际上C标准说明 pointer type 是派生类型
 * */
struct type Types[VOID - CHAR + 1];
Type DefaultFunctionType;
Type WCharType;

static const char *categoryNames[] = {
        "CHAR", "UCHAR", "SHORT", "USHORT", "INT", "UINT", "LONG", "ULONG", "LONGLONG", "ULONGLONG", "ENUM",
        "FLOAT", "DOUBLE", "LONGDOUBLE",
        "POINTER", "VOID", "UNION", "STRUCT", "ARRAY", "FUNCTION",
        "NA"
};

const char *GetCategoryName(int category) {
    return categoryNames[category];
}

int GetTypeCode(Type ty) {
    static int typeCodes[] = {
            I1, U1, I2, U2, I4, U4, I4, U4, I4, U4, I4,
            F4, F8, F8,
            U4, V, B, B, B
    };
    assert(ty->category != FUNCTION);

    return typeCodes[ty->category];
}

char *TypeToString(Type ty) {
    int qualify = ty->qualify;
    char *qualifyStr;
    static char *typeNames[] = {
            "char", "unsigned char", "short", "unsigned short", "int", "unsigned int",
            "long", "unsigned long", "long long", "unsigned long long", "enum",
            "float", "double", "long double"
    };
    /*包含修饰符*/
    if (qualify) {
        ty = RemoveQualify(ty);
        switch (qualify) {
            case CONST:
                qualifyStr = "const";
                break;
            case VOLATILE:
                qualifyStr = "volatile";
                break;
            default:
                qualifyStr = "const volatile";
                break;
        }
        return FormatName("%s %s", qualifyStr, TypeToString(ty));
    }
    /*基本类型*/
    if (ty->category >= CHAR && ty->category <= LONGDOUBLE && ty->category != ENUM) {
        return typeNames[ty->category];
    }

    switch (ty->category) {
        case ENUM:
            return FormatName("enum %s", ((EnumType) ty)->enum_id);
        case POINTER:
            return FormatName("%s *", TypeToString(ty->base_type));
        case VOID:
            return "void";
        case UNION:
            return FormatName("union %s", ((RecordType) ty)->record_id);
        case STRUCT:
            return FormatName("struct %s", ((RecordType) ty)->record_id);
        case ARRAY:
            return FormatName("%s[%s]", TypeToString(ty->base_type), ty->size / ty->base_type->size);
        case FUNCTION:
            return FormatName("%s()", TypeToString(((FunctionType) ty)->base_type));
        default:
            Fatal("unknown type category");
            return NULL;
    }
}

int IsIncompleteEnum(Type ty) {
    ty = RemoveQualify(ty);
    return (ty->category == ENUM && !((EnumType) ty)->isComplete);
}

int IsIncompleteRecord(Type ty) {
    ty = RemoveQualify(ty);
    return (IsRecordType(ty) && !((RecordType) ty)->isComplete);
}

int IsZeroSizeArray(Type ty) {
    ty = RemoveQualify(ty);
    return (ty->category == ARRAY && ((ArrayType) ty)->len == 0 && ty->size == 0);
}

int IsIncompleteType(Type ty, int ignoreZeroArray) {
    ty = RemoveQualify(ty);
    switch (ty->category) {
        case ENUM:
            return IsIncompleteEnum(ty);
        case UNION:
        case STRUCT:
            return IsIncompleteRecord(ty);
        case ARRAY:
            if (!ignoreZeroArray && IsZeroSizeArray(ty)) {
                return 1;
            } else {
                return IsIncompleteType(ty->base_type, ignoreZeroArray);
            }
        default:
            return 0;
    }
}

static Type DoTypeClone(Type ty) {
    switch (ty->category) {
        case ENUM:{
            EnumType enumType;
            CALLOC(enumType);
            *enumType = *((EnumType) ty);
            return (Type) enumType;
        }
        case UNION:
        case STRUCT:{
            RecordType recordType;
            CALLOC(recordType);
            *recordType = *((RecordType) ty);
            return (Type) recordType;
        }
        case ARRAY:{
            ArrayType arrayType;
            CALLOC(arrayType);
            *arrayType = *((ArrayType) ty);
            return (Type) arrayType;
        }
        case FUNCTION:{
            FunctionType functionType;
            CALLOC(functionType);
            *functionType = *((FunctionType) ty);
            return (Type) functionType;
        }
        default:{
            Type resultType;
            CALLOC(resultType);
            *resultType = *ty;
            return resultType;
        }
    }
}

Type Qualify(int qualify, Type ty) {
    Type resultType;
    if (qualify == 0 || qualify == ty->qualify)
        return ty;
    resultType = DoTypeClone(ty);
    resultType->qualify |= qualify;
    if (ty->qualify != 0) {
        resultType->base_type = ty->base_type;
    } else {
        resultType->base_type = ty;
    }

    return resultType;
}

Type RemoveQualify(Type ty) {
    if (ty->qualify)
        ty = ty->base_type;
    return ty;
}

Type Enum(char *id) {
    EnumType enumTy;
    ALLOC(enumTy);

    enumTy->base_type = T(INT);
    enumTy->category = ENUM;
    enumTy->qualify = 0;
    enumTy->align = enumTy->base_type->align;
    enumTy->size = enumTy->base_type->size;
    enumTy->enum_id = id;

    return (Type) enumTy;
}

Type PointerTo(Type ty) {
    Type ptrType;
    ALLOC(ptrType);

    ptrType->category = POINTER;
    ptrType->qualify = 0;
    ptrType->align = T(POINTER)->align;
    ptrType->size = T(POINTER)->size;
    ptrType->base_type = ty;

    return ptrType;
}

Type ArrayOf(int len, Type ty) {
    ArrayType arrayType;
    CALLOC(arrayType);

    arrayType->category = ARRAY;
    arrayType->qualify = 0;
    arrayType->align = ty->align;
    arrayType->size = ty->size * len;
    arrayType->base_type = ty;
    arrayType->len = len;

    return (Type) arrayType;
}

Type FunctionReturn(Type ty, Signature sig) {
    FunctionType functionType;
    ALLOC(functionType);

    functionType->category = FUNCTION;
    functionType->qualify = 0;
    functionType->align = T(POINTER)->align;
    functionType->size = T(POINTER)->size;
    functionType->base_type = ty;
    functionType->sig = sig;

    return (Type) functionType;
}

/*无名参数的类型提升*/
Type Promote(Type ty) {
    return ty->category < INT ? T(INT) : (ty->category == FLOAT ? T(DOUBLE) : ty);
}

Type AdjustParameter(Type ty) {
    ty = RemoveQualify(ty);

    if (ty->category == ARRAY)
        return PointerTo(ty->base_type);
    if (ty->category == FUNCTION)
        return PointerTo(ty);

    return ty;
}

Type StartRecord(char *id, int category) {
    RecordType recordType;
    ALLOC(recordType);
    recordType->category = category;
    /*刚开始构造struct、union类型的时候align、size都是未知*/
    recordType->align = recordType->size = 0;
    recordType->record_id = id;
    recordType->tail_field_ptr = &recordType->record_fields;
    recordType->isComplete = 0;

    return (Type) recordType;
}

Field AddField(Type ty, char *id, Type fieldTy, int bits) {
    RecordType recordType = (RecordType) ty;
    Field fld;

    if (fieldTy->qualify & CONST) {
        recordType->hasConstField = 1;
    }
    if (fieldTy->category == ARRAY && fieldTy->size == 0) {
        recordType->hasFlexArray = 1;
    }

    ALLOC(fld);
    fld->field_id = id;
    fld->bits = bits;
    /*offset在EndRecord中完成*/
    fld->pos = fld->offset = 0;
    fld->field_type = fieldTy;
    fld->next_field_ptr = NULL;

    /*修改最后一个field的nex指针的值为fld*/
    *recordType->tail_field_ptr = fld;
    /*更新为最后一个节点的nex指针成员的地址*/
    recordType->tail_field_ptr = &(fld->next_field_ptr);

    return fld;
}

Field LookupField(Type ty, char *id) {
    RecordType recordType = (RecordType) ty;
    /*TODO:头节点似乎没有初始化*/
    Field fld = recordType->record_fields->next_field_ptr;
    while (fld != NULL) {
        if (fld->field_id == NULL && IsRecordType(fld->field_type)) {
            /*匿名struct union成员，递归查找*/
            Field tmp;
            tmp = LookupField(fld->field_type, id);
            if (tmp != NULL) return tmp;
        } else if (fld->field_id == id) {
            /*统一管理ID字符串，所以只需要比较字符串的地址是否相同*/
            return fld;
        }
        fld = fld->next_field_ptr;
    }

    return NULL;
}

void AddOffset(RecordType rty, int offset) {
    /*TODO:头节点似乎没有初始化*/
    Field fld = rty->record_fields->next_field_ptr;
    while (fld) {
        fld->offset += offset;
        if (fld->field_id == NULL && IsRecordType(fld->field_type)) {
            AddOffset((RecordType) fld->field_type, fld->offset);
        }
        fld = fld->next_field_ptr;
    }
}

void EndRecord(Type ty, Coord coord) {
    RecordType recordType = (RecordType) ty;
    /*TODO:头节点似乎没有初始化*/
    Field fld = recordType->record_fields->next_field_ptr;
    int bits = 0;
    int intTypeBits = T(INT)->size * 8;
    if (recordType->category == STRUCT) {
        while (fld != NULL) {
            fld->offset = recordType->size = ALIGN(recordType->size, fld->field_type->align);
            if (fld->field_id == NULL && IsRecordType(fld->field_type)) {
                AddOffset((RecordType) fld->field_type, fld->offset);
            }
            if (!fld->bits) {/*非位域成员*/
                if (bits != 0) {/*之前处理过位域成员*/
                    fld->offset = recordType->size = ALIGN(recordType->size + T(INT)->size, fld->field_type->align);
                }
                bits = 0;
                recordType->size += fld->field_type->size;
            } else if (bits + fld->bits <= intTypeBits) {/*是位域成员不需要另外增加空间*/
                fld->pos = LITTLE_ENDIAN ? bits : intTypeBits - bits;
                bits += fld->bits;
                if (bits == intTypeBits) {
                    recordType->size += T(INT)->size;
                    bits = 0;
                }
            } else {/*是位域成员[需要]另外增加空间*/
                recordType->size += T(INT)->size;
                fld->offset += T(INT)->size;
                fld->pos = LITTLE_ENDIAN ? 0 : intTypeBits - fld->bits;
                bits = fld->bits;
            }
            if (fld->field_type->align > recordType->align) {
                recordType->align = fld->field_type->align;
            }
            fld = fld->next_field_ptr;
        }
        if (bits != 0) {/*最后一个成员是位域成员*/
            recordType->size += T(INT)->size;
        }
        recordType->size = ALIGN(recordType->size, recordType->align);
    } else {/*UNION*/
        while (fld) {
            if (fld->field_type->align > recordType->align) {
                recordType->align = fld->field_type->align;
            }
            if (fld->field_type->size > recordType->size) {
                recordType->size = fld->field_type->size;
            }
            fld = fld->next_field_ptr;
        }
    }
    /*其实这个应该放在语义检查里*/
    if (recordType->category == UNION && recordType->hasFlexArray) {
        Error(coord, "flexible array member in union");
    }
    if (recordType->category == STRUCT && recordType->size == 0 && recordType->hasFlexArray) {
        Error(coord, "flexible array member in otherwise empty struct");
    }
}

static int IsCompatibleFunction(FunctionType fty1, FunctionType fty2) {
    Signature sig1 = fty1->sig;
    Signature sig2 = fty2->sig;
    Parameter param1, param2;
    int paramLen1, paramLen2;
    int idx;

    /*返回类型不相容 则不相容*/
    if (!IsCompatibleType(fty1->base_type, fty2->base_type))
        return 0;
    /*返回类型相容，都没有函数原型 则相容*/
    if (!sig1->hasProto && !sig2->hasProto) {
        return 1;
    } else if (sig1->hasProto && sig2->hasProto) {
        /*是否可变参数为异 则不相容*/
        if (sig1->hasEllipsis ^ sig2->hasEllipsis)
            return 0;
        paramLen1 = GET_VECTOR_SIZE(sig1->params);
        paramLen2 = GET_VECTOR_SIZE(sig2->params);
        if (paramLen1 != paramLen2)
            return 0;
        for (idx = 0; idx < paramLen1; ++idx) {
            param1 = (Parameter) GET_VECTOR_ITEM(sig1->params, idx);
            param2 = (Parameter) GET_VECTOR_ITEM(sig2->params, idx);
            if (!IsCompatibleType(param1->parameter_type, param2->parameter_type))
                return 0;
        }
        return 1;
    }

    /*else 有一个sig没有原型*/
    if (!sig1->hasProto) {/*做个swap方便后面统一处理*/
        sig1 = fty2->sig;
        sig2 = fty1->sig;
    }
    /*之后处理的情况是 sig1有原型 sig2没有原型*/
    paramLen1 = GET_VECTOR_SIZE(sig1->params);
    paramLen2 = GET_VECTOR_SIZE(sig2->params);
    if (sig1->hasEllipsis)
        return 0;
    if (paramLen2 == 0) {
        FOR_EACH_VECTOR_ITEM(Parameter, param1, sig1->params)
                if (!IsCompatibleType(Promote(param1->parameter_type), param1->parameter_type))
                    return 0;
        END_FOR_EACH_VECTOR_ITEM
        return 1;
    } else if (paramLen1 != paramLen2) {/*paramLen2 != 0*/
        return 0;
    } else {/*paramLen2 != 0 && paramLen1 == paramLen2*/
        for (idx = 0; idx < paramLen1; ++idx) {
            param1 = (Parameter) GET_VECTOR_ITEM(sig1->params, idx);
            param2 = (Parameter) GET_VECTOR_ITEM(sig2->params, idx);
            if (!IsCompatibleType(RemoveQualify(param1->parameter_type),
                                  Promote(RemoveQualify(param2->parameter_type))))
                return 0;
        }
        return 1;
    }
}

int IsCompatibleType(Type ty1, Type ty2) {
    if (ty1 == ty2)
        return 1;
    if (ty1->qualify != ty2->qualify)
        return 0;

    ty1 = RemoveQualify(ty1);
    ty2 = RemoveQualify(ty2);

    if ((ty1->category == ENUM && ty1->base_type->category == ty2->category) ||
        (ty2->category == ENUM && ty2->base_type->category == ty1->category))
        return 1;
    if (ty1->category != ty2->category)
        return 0;

    switch (ty1->category) {
        case POINTER:
            return IsCompatibleType(ty1->base_type, ty2->base_type);
        case ARRAY:
            return (ty1->size == ty2->size || ty1->size == 0 || ty2->size == 0) &&
                   IsCompatibleType(ty1->base_type, ty2->base_type);
        case FUNCTION:
            return IsCompatibleFunction((FunctionType) ty1, (FunctionType) ty2);
        default:
            return ty1 == ty2;
    }
}

/*参见6.1.2.6*/
Type CompositeType(Type ty1, Type ty2) {
    assert(IsCompatibleType(ty1, ty2));

    if (ty1->category == ENUM)
        return ty1;
    if (ty2->category == ENUM)
        return ty2;

    switch (ty1->category) {
        case POINTER:
            return Qualify(ty1->qualify, PointerTo(CompositeType(ty1->base_type, ty2->base_type)));
        case ARRAY:
            return ty1->size == 0 ? ty2 : ty1;
        case FUNCTION: {
            FunctionType fty1 = (FunctionType) ty1;
            FunctionType fty2 = (FunctionType) ty2;
            fty1->base_type = CompositeType(fty1->base_type, fty2->base_type);
            if (fty1->sig->hasProto && fty2->sig->hasProto) {
                Parameter param1, param2;
                int idx, paramLen = GET_VECTOR_SIZE(fty1->sig->params);
                for (idx = 0; idx < paramLen; ++idx) {
                    param1 = (Parameter) GET_VECTOR_ITEM(fty1->sig->params, idx);
                    param2 = (Parameter) GET_VECTOR_ITEM(fty2->sig->params, idx);
                    param1->parameter_type = CompositeType(param1->parameter_type, param2->parameter_type);
                }
                return ty1;
            }
            return fty1->sig->hasProto ? ty1 : ty2;
        }
        default:
            return ty1;
    }
}

/* 用于隐式类型转换
 * 参见 6.2.1.5 Usual arithmetic conversions
 * */
Type CommonRealType(Type ty1, Type ty2) {
    if (ty1->category == LONGDOUBLE || ty2->category == LONGDOUBLE)
        return T(LONGDOUBLE);
    if (ty1->category == DOUBLE || ty2->category == DOUBLE)
        return T(DOUBLE);
    if (ty1->category == FLOAT || ty2->category == FLOAT)
        return T(FLOAT);

    ty1 = ty1->category < INT ? T(INT) : ty1;
    ty2 = ty2->category < INT ? T(INT) : ty2;
    if (ty1->category == ty2->category)
        return ty1;
    /*相同的signed*/
    if ((IsUnsigned(ty1) ^ IsUnsigned(ty2)) == 0)
        return ty1->category > ty2->category ? ty1 : ty2;
    /*不相同的signed 做swap方便后期处理 ty1:unsigned ty2:signed*/
    if (IsUnsigned(ty2)) {
        Type tmp = ty1;
        ty1 = ty2;
        ty2 = tmp;
    }
    if (ty1->category >= ty2->category) {
        return ty1;
    } else {/*ty1->category < ty2->category*/
        if (ty2->size > ty1->size)
            return ty2;
        else
            return T(ty2->category + 1);
    }
}

/*
 * 根据config.h构建类型系统
 * 实际上config.h变更的话还需要变更GetTypeCode(Type ty)相关
 * */
void SetupTypeSystem(void) {
    int idx;
    FunctionType functionType;
    T(CHAR)->size = T(UCHAR)->size = CHAR_SIZE;
    T(SHORT)->size = T(USHORT)->size = SHORT_SIZE;
    T(INT)->size = T(UINT)->size = INT_SIZE;
    T(LONG)->size = T(ULONG)->size = LONG_SIZE;
    T(LONGLONG)->size = T(ULONGLONG)->size = LONG_LONG_SIZE;
    T(FLOAT)->size = FLOAT_SIZE;
    T(DOUBLE)->size = DOUBLE_SIZE;
    T(LONGDOUBLE)->size = LONG_DOUBLE_SIZE;
    T(POINTER)->size = POINTER_SIZE;
    T(POINTER)->base_type = T(INT);

    for (idx = CHAR; idx < VOID; ++idx){
        T(idx)->category = idx;
        T(idx)->align = T(idx)->size;
    }

    ALLOC(functionType);
    functionType->category = FUNCTION;
    functionType->qualify = 0;
    functionType->align = T(POINTER)->align;
    functionType->size = T(POINTER)->size;
    functionType->base_type = T(INT);
    ALLOC(functionType->sig);
    functionType->sig->hasProto = 0;
    functionType->sig->hasEllipsis = 0;
    ALLOC(functionType->sig->params);
    DefaultFunctionType = (Type)functionType;

    WCharType = T(WCHAR);
}