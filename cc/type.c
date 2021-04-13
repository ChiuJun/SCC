//
// Created by LJChi on 2021/4/11.
//

#include "cc.h"

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
            return FormatName("%s[%s]", TypeToString(ty->base_type), ty->szie / ty->base_type->szie);
        case FUNCTION:
            /*TODO:没生成参数*/
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
    return (ty->category == ARRAY && ((ArrayType) ty)->len == 0 && ty->szie == 0);
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


Type Enum(char *id);

Type Qualify(int qualify, Type ty);

Type RemoveQualify(Type ty) {
    if (ty->qualify)
        ty = ty->base_type;
    return ty;
}

Type PointerTo(Type ty);

Type ArrayOf(int len, Type ty);

Type FunctionReturn(Type ty, Signature sig);

Type Promote(Type ty);

Type StartRecord(char *id, int category);

Field AddField(Type ty, char *id, Type fieldTy, int bits);

Field LookupField(Type ty, char *id);

void EndRecord(Type ty, Coord coord);

int IsCompatibleType(Type ty1, Type ty2);

Type CompositeType(Type ty1, Type ty2);

Type CommonFPType(Type ty1, Type ty2);

Type AdjustParameter(Type ty);

void SetupTypeSystem(void);