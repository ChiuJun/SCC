//
// Created by LJChi on 2021/5/23.
//

#ifndef OPRCODE
#error "You must define OPRCODE macro before include this file"
#endif
/**
	opcode here is used by UIL
 */
OPRCODE(BOR,     "|",                    Assign)
OPRCODE(BXOR,    "^",                    Assign)
OPRCODE(BAND,    "&",                    Assign)
OPRCODE(LSH,     "<<",                   Assign)
OPRCODE(RSH,     ">>",                   Assign)
OPRCODE(ADD,     "+",                    Assign)
OPRCODE(SUB,     "-",                    Assign)
OPRCODE(MUL,     "*",                    Assign)
OPRCODE(DIV,     "/",                    Assign)
OPRCODE(MOD,     "%",                    Assign)
OPRCODE(NEG,     "-",                    Assign)
OPRCODE(BCOM,    "~",                    Assign)
OPRCODE(JZ,      "",                     Branch)
OPRCODE(JNZ,     "!",                    Branch)
OPRCODE(JE,      "==",                   Branch)
OPRCODE(JNE,     "!=",                   Branch)
OPRCODE(JG,      ">",                    Branch)
OPRCODE(JL,      "<",                    Branch)
OPRCODE(JGE,     ">=",                   Branch)
OPRCODE(JLE,     "<=",                   Branch)
OPRCODE(JMP,     "jmp",                  Jump)
OPRCODE(IJMP,    "ijmp",                 IndirectJump)
OPRCODE(INC,     "++",                   Inc)
OPRCODE(DEC,     "--",                   Dec)
OPRCODE(ADDR,    "&",                    Address)
OPRCODE(DEREF,   "*",                    Deref)
OPRCODE(EXTI1,   "(int)(char)",          Cast)
OPRCODE(EXTU1,   "(int)(unsigned char)", Cast)
OPRCODE(EXTI2,   "(int)(short)",         Cast)
OPRCODE(EXTU2,   "(int)(unsigned short)",Cast)
OPRCODE(TRUI1,   "(char)(int)",          Cast)
OPRCODE(TRUI2,   "(short)(int)",         Cast)
OPRCODE(CVTI4F4, "(float)(int)",         Cast)
OPRCODE(CVTI4F8, "(double)(int)",        Cast)
OPRCODE(CVTU4F4, "(float)(unsigned)",    Cast)
OPRCODE(CVTU4F8, "(double)(unsigned)",   Cast)
OPRCODE(CVTF4,   "(double)(float)",      Cast)
OPRCODE(CVTF4I4, "(int)(float)",         Cast)
OPRCODE(CVTF4U4, "(unsigned)(float)",    Cast)
OPRCODE(CVTF8,   "(float)(double)",      Cast)
OPRCODE(CVTF8I4, "(int)(double)",        Cast)
OPRCODE(CVTF8U4, "(unsigned)(double)",   Cast)
OPRCODE(MOV,     "=",                    Move)
OPRCODE(IMOV,    "*=",                   IndirectMove)
OPRCODE(CALL,    "call",                 Call)
OPRCODE(RET,     "ret",                  Return)
OPRCODE(CLR,     "",                     Clear)
OPRCODE(NOP,     "NOP",                  NOP)