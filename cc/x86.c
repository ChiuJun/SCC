//
// Created by LJChi on 2021/5/23.
//

#include "cc.h"
#include "expr.h"
#include "gen.h"
#include "reg.h"
#include "target.h"
#include "output.h"

extern int SwitchTableNum;

enum ASMCode {
#define TEMPLATE(code, str) code,

#include "x86.tpl"

#undef TEMPLATE
};

/**
	 TEMPLATE(X86_ORI4, 	"orl %2, %0")		X86_OR		I4
	 TEMPLATE(X86_ORU4, 	"orl %2, %0")					U4
	 TEMPLATE(X86_ORF4, 	NULL)						F4
	 TEMPLATE(X86_ORF8, 	NULL)						F8

	 TEMPLATE(X86_XORI4,	"xorl %2, %0")			X86_XOR		I4
	 TEMPLATE(X86_XORU4,	"xorl %2, %0")					U4
	 TEMPLATE(X86_XORF4,	NULL)								F4
	 TEMPLATE(X86_XORF8,	NULL)								F8

 */
#define ASM_CODE(opcode, tcode) ((opcode << 2) + tcode - I4)
#define DST  inst->opds[0]
#define SRC1 inst->opds[1]
#define SRC2 inst->opds[2]
#define IsNormalRecord(rty) (rty->size != 1 && rty->size != 2 && rty->size != 4 && rty->size != 8)
/**
	 main:
		 pushl %ebp
		 pushl %ebx
		 pushl %esi
		 pushl %edi
		 movl %esp, %ebp
 */
#define PRESERVE_REGS 4
#define SCRATCH_REGS  4
#define STACK_ALIGN_SIZE 4

static Symbol X87Top;
static int X87TCode;

static int ORG;
static int FloatNum;
/**
	TEMPLATE(X86_JMP,      "jmp %0")
	TEMPLATE(X86_IJMP,     "jmp *%0(,%1,4)")

	TEMPLATE(X86_PROLOGUE, "pushl %%ebp;pushl %%ebx;pushl %%esi;pushl %%edi;movl %%esp, %%ebp")
 */
static char *ASMTemplate[] =
        {
#define TEMPLATE(code, str) str,

#include "x86.tpl"

#undef TEMPLATE
        };

/**
 * Put assembly code to move src into dst
 */
static void Move(int code, Symbol dst, Symbol src) {
    Symbol opds[2];

    opds[0] = dst;
    opds[1] = src;
    PutASMCode(code, opds);
}

static int GetListLen(Symbol reg) {
    int len = 0;
    Symbol next = reg->link;
    while (next) {
        len++;
        next = next->link;
    }
    return len;
}


/**
 * Mark the variable is in register
 */
/**
	This function only add @v into @reg link-list,
	and we are sure that @v is a SK_Temp.
	This function is different from static Symbol PutInReg(Symbol p),
		whick @p could be SK_Variable.
 */
static void AddVarToReg(Symbol reg, Symbol v) {

    assert(v->symbol_kind == SK_Temp);
    assert(GetListLen(reg) == 0);

    v->link = reg->link;
    reg->link = v;
    v->reg = reg;
}


/**
 * When a variable is modified, if it is not in a register, do nothing;
 * otherwise, spill othere variables in this register, set the variable's
 * needWB flag.(need write back to memory)
 */
static void ModifyVar(Symbol p) {
    Symbol reg;

    if (p->reg == NULL)
        return;

    p->need_writeback = 0;
    reg = p->reg;
    /**
        The following assertion seems to be right.
        In fact, the only thing we have to do here
        is set
        p->needwb = 1; 	?
     */
    assert(GetListLen(reg) == 1);
    assert(p == reg->link);

    SpillReg(reg);

    AddVarToReg(reg, p);

    p->need_writeback = 1;
}

/**
 * Allocate register for instruction operand. index indicates which operand
 */
static void AllocateReg(IRInst inst, int index) {
    Symbol reg;
    Symbol p;

    p = inst->opds[index];

    // In x86, UCC only allocates register for temporary
    if (p->symbol_kind != SK_Temp)
        return;

    // if it is already in a register, mark the register as used by current uil
    if (p->reg != NULL) {
        UsedRegs |= 1 << p->reg->val.i[0];
        return;
    }

    /// in x86, the first source operand is also destination operand,
    /// reuse the first source operand's register if the first source operand
    /// will not be used after this instruction
    if (index == 0 && SRC1->ref == 1 && SRC1->reg != NULL) {
        reg = SRC1->reg;
        reg->link = NULL;
        AddVarToReg(reg, p);
        return;
    }

    /// get a register, if the operand is not destination operand, load the
    /// variable into the register
    reg = GetReg();
    if (index != 0) {
        Move(X86_MOVI4, reg, p);
    }
    AddVarToReg(reg, p);
}

/**
 * Before executing an float instruction, UCC ensures that only TOP register of x87 stack
 * may be used. And whenenver a basic block ends, save the x87 stack top if needed. Thus
 * we can assure that when entering and leaving each basic block, the x87 register stack
 * is in init status.
 */
static void SaveX87Top(void) {
    if (X87Top == NULL)
        return;

    assert(X87Top->symbol_kind == SK_Temp);
    if (X87Top->need_writeback && X87Top->ref > 0) {
        /**
             TEMPLATE(X86_LDF4, 	"flds %0")
             TEMPLATE(X86_LDF8, 	"fldl %0")
             TEMPLATE(X86_STF4, 	"fstps %0")
             TEMPLATE(X86_STF8, 	"fstpl %0")
        */
        PutASMCode(X86_STF4 + X87TCode - F4, &X87Top);
    }
    X87Top = NULL;
}

/**
 * Emit floating point branch instruction
 */
static void EmitX87Branch(IRInst inst, int tcode) {
    // see EmitBranch(). We have called ClearRegs() before jumping.
    PutASMCode(X86_LDF4 + tcode - F4, &SRC1);
    PutASMCode(ASM_CODE(inst->opcode, tcode), inst->opds);

}

/**
	@index
		0		DST
		1		SRC1
		2		SRC2
 */
static int IsLiveAfterCurEmit(IRInst inst, int index) {
    int isLive = 1;
    // Try decrement ref
    if (!(inst->opcode >= JZ && inst->opcode <= IJMP) &&
        inst->opcode != CALL) {
        DST->ref--;
        if (SRC1 && SRC1->symbol_kind != SK_Function) SRC1->ref--;
        if (SRC2 && SRC2->symbol_kind != SK_Function) SRC2->ref--;
    }
    isLive = (inst->opds[index]->ref > 0);

    // roll back
    if (!(inst->opcode >= JZ && inst->opcode <= IJMP) &&
        inst->opcode != CALL) {
        DST->ref++;
        if (SRC1 && SRC1->symbol_kind != SK_Function) SRC1->ref++;
        if (SRC2 && SRC2->symbol_kind != SK_Function) SRC2->ref++;
    }
    return isLive;
}

/**
 * Emit floating point move instruction
 */
static void EmitX87Move(IRInst inst, int tcode) {
    // let X87 TOP be the first source operand
    if (X87Top != SRC1) {
        SaveX87Top();
        // TEMPLATE(X86_LDF4,	   "flds %0")
        PutASMCode(X86_LDF4 + tcode - F4, &SRC1);
    }

    // only put temporary in register
    /**
        Because
        TryAddValue(Type ty, int op, Symbol src1, Symbol src2) in gen.c
        knows nothing about X87 FPU,
        It seems that we have to
    */
    if (DST->symbol_kind != SK_Temp) {
        /**
            When we get here, we are sure that
            SRC1 are on the TOP of X87 stack.
            We will do the following:
            (1) store X87 top to DST, by X86_STF4_NO_POP / X86_STF4
            (2) if SRC1 is a temporary and still alive later,
                mark SRC1 be the X87Top.
                 else
                     mark X87Top = NULL;
         */
        if (SRC1->symbol_kind == SK_Temp && IsLiveAfterCurEmit(inst, 1)) {
            PutASMCode(X86_STF4_NO_POP + tcode - F4, &DST);
            SRC1->need_writeback = 1;
            X87Top = SRC1;
            X87TCode = tcode;
        } else {
            PutASMCode(X86_STF4 + tcode - F4, &DST);
            X87Top = NULL;
        }

    } else {
        /**
            It seems that
            the  situations we Move value To a temporary variable
            are
            (1)in function EmitDeref().
            (2)a = b > 1? c:d;
         */
        DST->need_writeback = 1;
        X87Top = DST;
        X87TCode = tcode;
    }

}

/**
 * Emit floating point assign instruction
 */
//	1.23f + 1.1f,	1.0 * 2.0	,  -a,  see opcode.h
static void EmitX87Assign(IRInst inst, int tcode) {
    // SRC2 could be NULL when it is a unary operator, like '-a'
    assert(DST->symbol_kind == SK_Temp && SRC1 != NULL);
    // PRINT_CUR_IRInst(inst);
    if (SRC1 != X87Top) {
        SaveX87Top();
        PutASMCode(X86_LDF4 + tcode - F4, &SRC1);
    } else {
        /**
            void GetValue(void){
                float f1 = 1.0f, f2;
                f2 = f1+1.0f;
                f2 = (f1+1.0f)+(f1+1.0f);
                printf("%f \n",f2);
            }
            If SRC1 is also SRC2, or SRC1 is still alive after current emit,
            we copy the top to memory of SRC1, because DST will be
            the X87 top later.
            Else,
                we can discard SRC1 later, no need to write back.
         */
        if (SRC1 == SRC2 || IsLiveAfterCurEmit(inst, 1)) {
            PutASMCode(X86_STF4_NO_POP + tcode - F4, &SRC1);
        }
    }
    /**
        TEMPLATE(X86_ADDF8,    "faddl %2")
        TEMPLATE(X86_SUBF8,    "fsubl %2")
     */
    PutASMCode(ASM_CODE(inst->opcode, tcode), inst->opds);
    // now DST be the new TOP of X87
    DST->need_writeback = 1;
    X87Top = DST;
    X87TCode = tcode;

}

/**
 * Emit assembly code for block move
 */
static void EmitMoveBlock(IRInst inst) {

    if (inst->ty->size == 0) {
        return;
    }

    SpillReg(X86Regs[EDI]);
    SpillReg(X86Regs[ESI]);
    SpillReg(X86Regs[ECX]);
    /**
         typedef struct{
             int data[10];
         }Data;
         Data a,b;
         int main(){
             a = b;
             return 0;
         }
        --------------------

         leal a, %edi
         leal b, %esi
         movl $40, %ecx
         rep movsb

     */
    SRC2 = IntConstant(inst->ty->size);
    // TEMPLATE(X86_MOVB,	   "leal %0, %%edi;leal %1, %%esi;movl %2, %%ecx;rep movsb")
    PutASMCode(X86_MOVB, inst->opds);
}

/**
 * Emit assembly code for move
 */
static void EmitMove(IRInst inst) {
    int tcode = GetTypeCode(inst->ty);
    Symbol reg;
    //PRINT_DEBUG_INFO(("%s",GetAsmCodeName(X86_X87_POP)));
    if (tcode == F4 || tcode == F8) {
        EmitX87Move(inst, tcode);
        return;
    }

    if (tcode == B) {
        EmitMoveBlock(inst);
        return;
    }

    switch (tcode) {
        case I1:
        case U1:
            /**
                char a,b;
                int main(){
                    a = b;
                    a = '3';
                    return 0;
                }

                ----------------------------------------
                movb b, %al
                movb %al, a

                movb $51, a		// SK_Constant

             */
            if (SRC1->symbol_kind == SK_Constant) {
                // TEMPLATE(X86_MOVI1,    "movb %1, %0")
                Move(X86_MOVI1, DST, SRC1);
            } else {
                reg = GetByteReg();
                Move(X86_MOVI1, reg, SRC1);
                Move(X86_MOVI1, DST, reg);
            }
            break;

        case I2:
        case U2:
            if (SRC1->symbol_kind == SK_Constant) {
                Move(X86_MOVI2, DST, SRC1);
            } else {
                reg = GetWordReg();
                Move(X86_MOVI2, reg, SRC1);
                Move(X86_MOVI2, DST, reg);
            }
            break;

        case I4:
        case U4:
            if (SRC1->symbol_kind == SK_Constant) {
                Move(X86_MOVI4, DST, SRC1);
            } else {
                /**
                    we try to reuse the temporary value in register.
                 */
                AllocateReg(inst, 1);
                AllocateReg(inst, 0);
                if (SRC1->reg == NULL && DST->reg == NULL) {
                    /**
                        On X86, we can't move from mem1 to mem2.
                        So we have to move from mem1 to register , and
                        then from register to mem2.
                     **/
                    reg = GetReg();
                    Move(X86_MOVI4, reg, SRC1);
                    Move(X86_MOVI4, DST, reg);
                } else {
                    Move(X86_MOVI4, DST, SRC1);
                }
            }
            ModifyVar(DST);
            break;

        default:
            assert(0);
    }
}

/**
 * Put the variable in register
 */
static Symbol PutInReg(Symbol p) {
    Symbol reg;

    if (p->reg != NULL) {
        assert(p->symbol_kind == SK_Temp);
        return p->reg;
    }
    reg = GetReg();
    // TEMPLATE(X86_MOVI4,    "movl %1, %0")
    Move(X86_MOVI4, reg, p);
    return reg;
}

/**
 * Emit assembly code for indirect move
 */
static void EmitIndirectMove(IRInst inst) {
    Symbol reg;

    /// indirect move is the same as move, except using register indirect address
    /// mode for destination operand
    reg = PutInReg(DST);
    inst->opcode = MOV;
    DST = reg->next;
    EmitMove(inst);
}

/**
 * Emit assembly code for assign
 */
//	a+b,		a-b,	a*b,a|b, a << 4
static void EmitAssign(IRInst inst) {
    int code;
    int tcode = GetTypeCode(inst->ty);


    assert(DST->symbol_kind == SK_Temp);

    if (tcode == F4 || tcode == F8) {
        EmitX87Assign(inst, tcode);
        return;
    }

    assert(tcode == I4 || tcode == U4);

    code = ASM_CODE(inst->opcode, tcode);

    switch (code) {
        case X86_DIVI4:
        case X86_DIVU4:
        case X86_MODI4:
        case X86_MODU4:
        case X86_MULU4:
            /**
                c = b / a;
                        movl b, %eax
                        cdq
                        idivl a
                        movl %eax, -4(%ebp)
                        movl -4(%ebp), %eax
                        movl %eax, c
             */
            if (SRC1->reg == X86Regs[EAX]) {
                SRC1->need_writeback = 0;
                SpillReg(X86Regs[EAX]);
            } else {
                SpillReg(X86Regs[EAX]);
                Move(X86_MOVI4, X86Regs[EAX], SRC1);
            }
            SpillReg(X86Regs[EDX]);
            UsedRegs = 1 << EAX | 1 << EDX;
            if (SRC2->symbol_kind == SK_Constant) {
                // Because "idivl $10" is illegal
                Symbol reg = GetReg();

                Move(X86_MOVI4, reg, SRC2);
                SRC2 = reg;
            } else {
                AllocateReg(inst, 2);
            }

            /**
                We are sure that dst is a temporary,
                its content is in EAX or EDX
             */
            PutASMCode(code, inst->opds);
            if (code == X86_MODI4 || code == X86_MODU4) {
                AddVarToReg(X86Regs[EDX], DST);
            } else {
                AddVarToReg(X86Regs[EAX], DST);
            }
            break;

        case X86_LSHI4:
        case X86_LSHU4:
        case X86_RSHI4:
        case X86_RSHU4:
            AllocateReg(inst, 1);
            if (SRC2->symbol_kind != SK_Constant) {
                if (SRC2->reg != X86Regs[ECX]) {
                    SpillReg(X86Regs[ECX]);
                    Move(X86_MOVI4, X86Regs[ECX], SRC2);
                }
                /**
                    The following assembly code is illegal:
                    Error: operand type mismatch for `shl'
                        shll %ecx, %eax
                    So we use %cl here.
                        shll %cl, %eax
                 */
                SRC2 = X86ByteRegs[ECX];
                UsedRegs = 1 << ECX;
            }
            goto put_code;

        case X86_NEGI4:
        case X86_NEGU4:
            AllocateReg(inst, 1);
            goto put_code;

        default:
            AllocateReg(inst, 1);
            AllocateReg(inst, 2);

        put_code:
            AllocateReg(inst, 0);

            assert(DST->reg != NULL);

            if (DST->reg != SRC1->reg) {
                Move(X86_MOVI4, DST, SRC1);
            }
            PutASMCode(code, inst->opds);
            break;
    }
    ModifyVar(DST);
}

/**
	 double c = 9.87;
	 double d;
	 int a = 50;char b = 'x';

	 int main(){
		 a = (int)c;
		 printf("%d \n",a);
		 a = (int)(c+1.23);
		 b = (char)(c+1.23);
		 printf("%d %d .\n",a,b);
		 d = c + 1.23;
		 c = (double)((float)(c+1.23))+(c+1.23);
		 printf("%f\n",c);
		 return 0;
	 }

 */
static void EmitCast(IRInst inst) {
    Symbol dst, reg;
    int code;

    dst = DST;

    //  this assertion fails, because TypeCast is not treated as common subexpression in UCC.
    // assert(DST->kind == SK_Temp);		//  See TryAddValue(..)

    reg = NULL;
    code = inst->opcode + X86_EXTI1 - EXTI1;
    switch (code) {
        case X86_EXTI1:
        case X86_EXTI2:
        case X86_EXTU1:
        case X86_EXTU2:
            assert(GetTypeCode(inst->ty) == I4);
            AllocateReg(inst, 0);
            if (DST->reg == NULL) {
                DST = GetReg();
            }
            PutASMCode(code, inst->opds);
            //	If dst != DST, then the original destination is not temporary,
            //	we mov the value in register to the named variable.
            if (dst != DST) {
                Move(X86_MOVI4, dst, DST);
            }
            break;

        case X86_TRUI1:        // truncate I4/U4 ---------->  I1
            assert(GetTypeCode(SRC1->symbol_type) == I4 || GetTypeCode(SRC1->symbol_type) == U4);

            if (SRC1->reg != NULL) {
                reg = X86ByteRegs[SRC1->reg->val.i[0]];
            }
            if (reg == NULL) {
                reg = GetByteReg();
                Move(X86_MOVI4, X86Regs[reg->val.i[0]], SRC1);
            }
            Move(X86_MOVI1, DST, reg);
            break;

        case X86_TRUI2:        // // truncate I4/U4 ---------->  I2

            assert(GetTypeCode(SRC1->symbol_type) == I4 || GetTypeCode(SRC1->symbol_type) == U4);

            if (SRC1->reg != NULL) {
                reg = X86WordRegs[SRC1->reg->val.i[0]];
            }
            if (reg == NULL) {
                reg = GetWordReg();
                Move(X86_MOVI4, X86Regs[reg->val.i[0]], SRC1);
            }
            Move(X86_MOVI2, DST, reg);
            break;
            /**
                Warning:
                    There is not X86_CVTI1F4/X86_CVTU1F4/X86_CVTI2F4/X86_CVTU2F4
                    Because the actual work is done by 2 steps:

                    For example:

                    char ch = 'a';
                    double d;
                    int main(){
                        d = (double)ch;
                        return 0;
                    }
                    -------------------------------
                    function main
                    t0 = (int)(char)ch;		-----------EXTI1
                    d = (double)(int)t0;	-----------CVTI4F8
                    return 0;
                    ret
                    -----------------------------------
                     movsbl ch, %eax		-----------EXTI1

                     pushl %eax			-----------CVTI4F8
                     fildl (%esp)
                     fstpl d

                     movl $0, %eax

             */
        case X86_CVTI4F4:
        case X86_CVTI4F8:
        case X86_CVTU4F4:
        case X86_CVTU4F8:
            assert(X87Top != DST);
            /**
                 TEMPLATE(X86_CVTI4F4,	"pushl %1;fildl (%%esp);fstps %0")
                TEMPLATE(X86_CVTI4F8,  "pushl %1;fildl (%%esp);fstpl %0")
                TEMPLATE(X86_CVTU4F4,  "pushl $0;pushl %1;fildq (%%esp);fstps %0")
                TEMPLATE(X86_CVTU4F8,  "pushl $0;pushl %1;fildq (%%esp);fstpl %0")
             */
            PutASMCode(code, inst->opds);
            break;

        default:
            /**
                Because the following PutASMCode(...) doesn't check X87 TOP,
                so we have to save it here.
             */
            SaveX87Top();
            if (code != X86_CVTF4 && code != X86_CVTF8) {
                /**
                    CVTF4I4,CVTF4U4,CVTF8I4,CVTF8U4
                    The assembly code in x86linux.tpl use EAX to store
                    the casted result.
                 */
                assert(code == X86_CVTF4I4 || code == X86_CVTF4U4
                       || code == X86_CVTF8I4 || code == X86_CVTF8U4);
                SpillReg(X86Regs[EAX]);
                AllocateReg(inst, 0);
                PutASMCode(code, inst->opds);
                /**
                    (DST->reg && DST->reg != X86Regs[EAX]) :
                        DST is a temporary and allocated in another register other than EAX,
                    DST->reg == NULL
                        DST is global/static/local variable
                 */
                if ((DST->reg && DST->reg != X86Regs[EAX]) || DST->reg == NULL) {
                    Move(X86_MOVI4, DST, X86Regs[EAX]);
                }
            } else {
                PutASMCode(code, inst->opds);
            }
            // we have touched the X87top in the last statement; mark it invalid explicitely.
            X87Top = NULL;
            break;
    }
    ModifyVar(dst);

}

//	a++	, float/double is also done here.
static void EmitInc(IRInst inst) {
    /**
         TEMPLATE(X86_INCI1,	"incb %0")
         TEMPLATE(X86_INCU1,	"incb %0")
         TEMPLATE(X86_INCI2,	"incw %0")
         TEMPLATE(X86_INCU2,	"incw %0")
         TEMPLATE(X86_INCI4,	"incl %0")
         TEMPLATE(X86_INCU4,	"incl %0")
         TEMPLATE(X86_INCF4,	"fld1;fadds %0;fstps %0")
     */
    PutASMCode(X86_INCI1 + GetTypeCode(inst->ty), inst->opds);
}

//	a--
static void EmitDec(IRInst inst) {
    //PRINT_CUR_IRInst(inst);
    PutASMCode(X86_DECI1 + GetTypeCode(inst->ty), inst->opds);
}

static void EmitBranch(IRInst inst) {
    int tcode = GetTypeCode(inst->ty);
    BBlock p = (BBlock) DST;
    /**
        We make the inst->opds[0] to a SK_Lable here.
     */
    DST = p->sym;
    if (tcode == F4 || tcode == F8) {
        //  see examples/cfg/crossBB.c
        ClearRegs();
        EmitX87Branch(inst, tcode);
        return;
    }

    assert(tcode >= I4);

    if (SRC2) {
        if (SRC2->symbol_kind != SK_Constant) {
            SRC1 = PutInReg(SRC1);
        }
    }

    SRC1->ref--;
    if (SRC1->reg != NULL)
        SRC1 = SRC1->reg;
    if (SRC2) {
        SRC2->ref--;
        if (SRC2->reg != NULL)
            SRC2 = SRC2->reg;
    }
    ClearRegs();
    PutASMCode(ASM_CODE(inst->opcode, tcode), inst->opds);
}

/**
	(1)	the target of Jump is a BBlock, not Variable.
		So no DST->ref -- here.
	(2)  X87 top is saved in EmitBlock(),
		because Jump must be the last IL in a basic
		block.
 */
static void EmitJump(IRInst inst) {
    BBlock p = (BBlock) DST;

    DST = p->sym;
    assert(DST->symbol_kind == SK_Label);
    ClearRegs();
    PutASMCode(X86_JMP, inst->opds);
}

/**
	 switch(a){
		 case 1:
			 a = 100;
			 break;
		 case 2:
			 a = 200;
			 break;
		 case 3:
			 a = 300;
			 break;
	 }

	IRinst	goto (BB0,BB1,BB2,)[t0];	 ---------------  ijmp	 21

			SRC1--------------t0
			DST	--------------[BB0,BB1,BB2,NULL]
	 ----------------------------------------------------------------
	 .data
	 swtchTable1:	 .long	 .BB0	----------	DefineAddress()
				 .long	 .BB1
				 .long	 .BB2

	 .text

		 jmp *swtchTable1(,%eax,4)

 */
static void EmitIndirectJump(IRInst inst) {
    BBlock *p;
    Symbol swtch;
    int len;
    Symbol reg;


    SRC1->ref--;
    p = (BBlock *) DST;
    reg = PutInReg(SRC1);

    PutString("\n");
    Segment(DATA);

    CALLOC(swtch);
    swtch->symbol_kind = SK_Variable;
    swtch->symbol_type = T(POINTER);
    swtch->name = FormatName("swtchTable%d", SwitchTableNum++);
    swtch->storage_class = TK_STATIC;
    swtch->level = 0;
    DefineGlobal(swtch);

    DST = swtch;
    len = strlen(DST->access_name);
    //PRINT_DEBUG_INFO(("%s ",DST->aname));
    while (*p != NULL) {
        DefineAddress((*p)->sym);
        PutString("\n");
        LeftAlign(ASMFile, len);
        PutString("\t");
        p++;
    }
    PutString("\n");

    Segment(CODE);

    SRC1 = reg;
    ClearRegs();
    // TEMPLATE(X86_IJMP,     "jmp *%0(,%1,4)")
    PutASMCode(X86_IJMP, inst->opds);
}

/**
	See TranslateReturnStatement()
	(1) The actual return action is done by Jumping to exitBB.
	(2) EmitReturn() here is just preparing the return value.
 */
static void EmitReturn(IRInst inst) {
    Type ty = inst->ty;

    if (IsFPType(ty)) {
        if (X87Top != DST) {
            int tcode = GetTypeCode(ty);
            SaveX87Top();
            // TEMPLATE(X86_LDF4,	   "flds %0")
            // When X87Top is NULL, X87TCode could be 0.
            PutASMCode(X86_LDF4 + tcode - F4, inst->opds);
        }
        X87Top = NULL;
        return;
    }
    /**
        see EmitFunction() and EmitCall()
     */
    if (IsRecordType(ty) && IsNormalRecord(ty)) {
        inst->opcode = IMOV;
        SRC1 = DST;
        // see  EmitFunction()
        DST = FSYM->params;
        // The actual work is done in EmitMoveBlock()
        EmitIndirectMove(inst);
        return;
    }
    /**
        We don't need to  call SpillReg(...) for EAX and EDX here.
        because in UCC, register only contains value for temporaries.
        When we return from function,these temporaries is not needed
        any more.
     */
    switch (ty->size) {
        case 1:
            Move(X86_MOVI1, X86ByteRegs[EAX], DST);
            break;

        case 2:
            Move(X86_MOVI2, X86WordRegs[EAX], DST);
            break;

        case 4:
            if (DST->reg != X86Regs[EAX]) {
                Move(X86_MOVI4, X86Regs[EAX], DST);
            }
            break;

        case 8:
            /**
                 typedef struct{
                     int arr[2];
                 }Data;
                 ---------------------------------
                 Data GetData(void){
                    Data dt;
                    return dt;
                }
                -------------------------------------
                 GetData:
                    ........
                     subl $8, %esp
                     movl -8(%ebp), %eax
                     movl -4(%ebp), %edx

             */
            Move(X86_MOVI4, X86Regs[EAX], DST);
            Move(X86_MOVI4, X86Regs[EDX], CreateOffset(T(INT), DST, 4, DST->pcoord));
            break;

        default:
            assert(0);
    }
}

static void PushArgument(Symbol p, Type ty) {
    int tcode = GetTypeCode(ty);
    /**
        We call SaveX87Top() in EmitBBlock() when it is "EmitCall()".
        In fact, when "p == X87Top",
        we can generate more efficient code here.
     */
    if (tcode == F4) {
        PutASMCode(X86_PUSHF4, &p);
    } else if (tcode == F8) {
        PutASMCode(X86_PUSHF8, &p);
    } else if (tcode == B) {
        Symbol opds[3];

        SpillReg(X86Regs[ESI]);
        SpillReg(X86Regs[EDI]);
        SpillReg(X86Regs[ECX]);
        opds[0] = p;
        opds[1] = IntConstant(ty->size);
        opds[2] = IntConstant(ALIGN(ty->size, STACK_ALIGN_SIZE));
        PutASMCode(X86_PUSHB, opds);
    } else {
        PutASMCode(X86_PUSH, &p);
    }
}

/**
	DST:
			return value
	SRC1:
			function name
	SRC2:
			arguments_list
*/
static void EmitCall(IRInst inst) {
    Vector args;
    ILArg arg;
    Type rty;
    int i, stksize;

    args = (Vector) SRC2;
    stksize = 0;
    rty = inst->ty;

    for (i = GET_VECTOR_SIZE(args) - 1; i >= 0; --i) {
        arg = GET_VECTOR_ITEM(args, i);
        PushArgument(arg->sym, arg->ty);
        if (arg->sym->symbol_kind != SK_Function) arg->sym->ref--;
        stksize += ALIGN(arg->ty->size, STACK_ALIGN_SIZE);
    }
    /**
         We don't have to call ClearRegs() in EmitCall(),
         Because ESI/EDI/EBX are saved in EmitPrologue().
     */
    SpillReg(X86Regs[EAX]);
    SpillReg(X86Regs[ECX]);
    SpillReg(X86Regs[EDX]);
    if (IsRecordType(rty) && IsNormalRecord(rty)) {
        Symbol opds[2];

        opds[0] = GetReg();
        opds[1] = DST;
        PutASMCode(X86_ADDR, opds);
        PutASMCode(X86_PUSH, opds);
        stksize += 4;
        DST = NULL;

    }

    PutASMCode(SRC1->symbol_kind == SK_Function ? X86_CALL : X86_ICALL, inst->opds);
    if (stksize != 0) {
        Symbol p;
        p = IntConstant(stksize);
        PutASMCode(X86_REDUCEF, &p);
    }


    if (DST != NULL)
        DST->ref--;
    if (SRC1->symbol_kind != SK_Function) SRC1->ref--;

    if (DST == NULL) {
        /**
            We have set X87Top to NULL in EmitReturn()
         */
        if (IsFPType(rty)) {
            PutASMCode(X86_X87_POP, inst->opds);
        }
        return;
    }


    if (IsFPType(rty)) {
        // 		TEMPLATE(X86_STF4,	   "fstps %0")
        PutASMCode(X86_STF4 + (rty->category != FLOAT), inst->opds);
        return;
    }

    switch (rty->size) {
        case 1:
            Move(X86_MOVI1, DST, X86ByteRegs[EAX]);
            break;

        case 2:
            Move(X86_MOVI2, DST, X86WordRegs[EAX]);
            break;

        case 4:
            AllocateReg(inst, 0);
            if (DST->reg != X86Regs[EAX]) {
                Move(X86_MOVI4, DST, X86Regs[EAX]);
            }
            ModifyVar(DST);
            break;

        case 8:
            Move(X86_MOVI4, DST, X86Regs[EAX]);
            Move(X86_MOVI4, CreateOffset(T(INT), DST, 4, DST->pcoord), X86Regs[EDX]);
            break;

        default:
            assert(0);
    }
}

static void EmitAddress(IRInst inst) {
    assert(DST->symbol_kind == SK_Temp && SRC1->symbol_kind != SK_Temp);
    AllocateReg(inst, 0);
    PutASMCode(X86_ADDR, inst->opds);
    ModifyVar(DST);
}

/**

	 int a;
	 int *ptr;
	 int main(){
		 ptr = &a;
		 a = *ptr;
		 return 0;
	 }
	 -----------------------
	function main
		t0 = &a;
		ptr = t0;
		t1 = *ptr;	-----------EmitDeref(...)
		a = t1;
		return 0;
		ret
	--------------------------

	 movl %eax, ptr
	 //	EmitDeref(t1 = *ptr;)
	 movl ptr, %ecx	---------------	PutInReg(ptr);
	 movl (%ecx), %edx

	 movl %edx, a

 */
static void EmitDeref(IRInst inst) {
    Symbol reg;

    reg = PutInReg(SRC1);
    inst->opcode = MOV;
    SRC1 = reg->next;
    assert(SRC1->symbol_kind == SK_IRegister);
    EmitMove(inst);
    ModifyVar(DST);
    return;
}

/**
	 int gArr[10] = {100};		--------  Not Need to EmitClear during runtime
	 int main(){
		 int lArr[20] = {200};	---------	EmitClear() to clear dynamic stack memory at run time
		 return 0;
	 }
 */
static void EmitClear(IRInst inst) {
    int size = SRC1->val.i[0];
    Symbol p = IntConstant(0);

    switch (size) {
        case 1:
            Move(X86_MOVI1, DST, p);
            break;

        case 2:
            Move(X86_MOVI2, DST, p);
            break;

        case 4:
            Move(X86_MOVI4, DST, p);
            break;

        default:
            SpillReg(X86Regs[EAX]);
            PutASMCode(X86_CLEAR, inst->opds);
            break;
    }
}

static void EmitNOP(IRInst inst) {
    assert(0);
}

/**
	OPCODE(IJMP,    "ijmp",                 IndirectJump)
	OPCODE(INC,     "++",                   Inc)
	OPCODE(DEC,     "--",                   Dec)
	OPCODE(ADDR,    "&",                    Address)
	OPCODE(DEREF,   "*",                    Deref)
	OPCODE(EXTI1,   "(int)(char)",          Cast)
	OPCODE(EXTU1,   "(int)(unsigned char)", Cast)
 */
static void (*Emitter[])(IRInst inst) =
        {
#define OPRCODE(code, name, func) Emit##func,

#include "oprcode.h"

#undef OPRCODE
        };

static void EmitIRInst(IRInst inst) {
    struct irinst instc = *inst;

    (*Emitter[inst->opcode])(&instc);
    return;
}

static void EmitBBlock(BBlock bb) {
    IRInst inst = bb->insth.next;

    while (inst != &bb->insth) {
        UsedRegs = 0;
        /**
            This bug is found by testing "Livermore loops".
            See	Line 1567 in cflops.c
              for (k=1 ; k<=n ; k++) {
                ox_1(k)= abs( x_1(k) - stat_1(7));
              }
            (1) see EmitX87Branch()
            (2) ClearRegs() are called before generating assembly jumping
                in EmitBranch()/EmitJump()/EmitIndirectJump()/EmitCall().
            (3) SaveX87Top() is called here to keep it simple.
                For conditional-expr, temporaries are used across basic blocks.
                see examples/cfg/crossBB.c
         */
        if ((inst->opcode >= JZ && inst->opcode <= IJMP) || (inst->opcode == CALL)) {
            SaveX87Top();
        }
        //  the kernel part of emit ASM from IR.
        EmitIRInst(inst);
        /**
            ref is used as a factor in spilling register
                static int SelectSpillReg(int endr)
        */
        if (!(inst->opcode >= JZ && inst->opcode <= IJMP) &&
            inst->opcode != CALL) {
            DST->ref--;
            if (SRC1 && SRC1->symbol_kind != SK_Function) SRC1->ref--;
            if (SRC2 && SRC2->symbol_kind != SK_Function) SRC2->ref--;
        }
        inst = inst->next;
    }
    ClearRegs();
    SaveX87Top();
}

/**
	function(parameter1, parameter2, ....)

	...............
	parameter2
	parameter1 _________	20(%ebp)
	return address			16(%ebp)
	old ebp				12(%ebp)
	ebx					8(%ebp)
	esi					4(%ebp)
	edi ______________	0(%ebp)
	local variables and temporaries		-4(%ebp)
									....

 */
static int LayoutFrame(FunctionSymbol fsym, int fstParamPos) {
    Symbol p;
    int offset;
    /**
        #include <stdio.h>

         void f(int a, int b){	-------		a, b are parameters
             int c = a + b;		-------		c is local variables.

             printf("%d \n",c);
         }
         int main(){
             f(3,4);
             return 0;
         }
            movl 20(%ebp), %eax		--------  a is 20(%ebp)
            addl 24(%ebp), %eax		--------  b is 24(%ebp)
            movl %eax, -4(%ebp)		--------  c is -4(%ebp)
     */
    offset = fstParamPos * STACK_ALIGN_SIZE;
    p = fsym->params;
    while (p) {
        AsVar(p)->offset = offset;
        if (p->symbol_type->size == 0) {
            //	empty struct or array of empty struct to be of 1 byte size
            offset += ALIGN(EMPTY_OBJECT_SIZE, STACK_ALIGN_SIZE);
        } else {
            offset += ALIGN(p->symbol_type->size, STACK_ALIGN_SIZE);
        }
        p = p->next;
    }

    offset = 0;
    /**
        SK_Temp/SK_Variable are in fsym->locals.
        In fact, some SK_Temp are allocated to register,
        but UCC always keep their stack position when the
        function is active. Of course , this cause a waste of
        stack memory, but it made the management of temporaries
        simple.
     */
    p = fsym->locals;
    while (p) {
        if (p->ref == 0)
            goto next;


        // for empty struct object or array of empty struct object
        if (p->symbol_type->size == 0) {
            offset += ALIGN(EMPTY_OBJECT_SIZE, STACK_ALIGN_SIZE);
        } else {
            offset += ALIGN(p->symbol_type->size, STACK_ALIGN_SIZE);
        }
        AsVar(p)->offset = -offset;
        // PRINT_DEBUG_INFO((" offset = %d, name = %s ",AsVar(p)->offset,AsVar(p)->name));
        next:
        p = p->next;
    }

    return offset;
}

static void EmitPrologue(int stksize) {
    /**
     * regardless of the actual register usage, always save the preserve registers.
     * on x86 platform, they are ebp, ebx, esi and edi
     */
    PutASMCode(X86_PROLOGUE, NULL);
    if (stksize != 0) {
        Symbol sym = IntConstant(stksize);
        // TEMPLATE(X86_EXPANDF,  "subl %0, %%esp")
        PutASMCode(X86_EXPANDF, &sym);
    }
}

static void EmitEpilogue(int stksize) {
    PutASMCode(X86_EPILOGUE, NULL);
}

void EmitFunction(FunctionSymbol fsym) {
    BBlock bb;
    Type rty;
    int stksize;

    FSYM = fsym;
    if (fsym->storage_class != TK_STATIC) {
        Export((Symbol) fsym);
    }
    DefineLabel((Symbol) fsym);

    rty = fsym->symbol_type->base_type;
    /**
         typedef struct{
             int arr[10];	// or char arr[3];
         }Data;


         Data GetData(void){	-------------->  void GetData(Date * implicit)
             Data dt;
             return dt;
         }
         see	EmitReturn(IRInst inst)
         Here:
             add a implicite T(POINTER) parameter.
     */
    if (IsRecordType(rty) && IsNormalRecord(rty)) {
        VariableSymbol p;
        /**
            We just add an formal parameter to act as placeholder,
            in order to let LayoutFrame allocate stack space for it.
            The actual work is done by EmitCall()
         */
        CALLOC(p);
        p->symbol_kind = SK_Variable;
        p->name = "recvaddr";
        p->symbol_type = T(POINTER);
        p->level = 1;
        p->storage_class = TK_AUTO;

        p->next = fsym->params;
        fsym->params = (Symbol) p;
    }

    stksize = LayoutFrame(fsym, PRESERVE_REGS + 1);
    /**
        main:
        pushl %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
        movl %esp, %ebp
        subl $32, %esp
     */
    EmitPrologue(stksize);

    bb = fsym->entryBB;
    while (bb != NULL) {
        // to show all basic blocks
        DefineLabel(bb->sym);
        EmitBBlock(bb);
        bb = bb->next;
    }
    /**
        movl %ebp, %esp
        popl %edi
        popl %esi
        popl %ebx
        popl %ebp
        ret
     */
    EmitEpilogue(stksize);
    PutString("\n");
}

//  store register value to variable
void StoreVar(Symbol reg, Symbol v) {
    // WIN32:
    // TEMPLATE(X86_MOVI4,     "mov %d0, %d1")
    // Linux:
    // TEMPLATE(X86_MOVI4,    "movl %1, %0")
    Move(X86_MOVI4, v, reg);
}

/**
	see X86win32.c:
		 * When defining a symbol, it shall be in its alignment boundary.
		 * In order to avoid too many align directives, we maintain a global
		 * address counter ORG, only when ORG is not in symbol's alignment
		 * boundary, write align directive
	char a = '1';
	double d = 1.23;
	int b = 30;
	double e = 1.23;

	.globl	a
	a:	.byte	49

	.align 8
	.globl	d

	d:	.long	2061584302
	.long	1072934420

	.globl	b
	b:	.long	30

	.align 8
	.globl	e

	e:	.long	2061584302
	.long	1072934420

		This function outputs ".align .." when necessary
 */

static void Align(Symbol p) {
    int align = p->symbol_type->align;

    if (ORG % align != 0) {
        Print(".align %d\n", align);
        ORG = ALIGN(ORG, align);
    }
    ORG += p->symbol_type->size;
}

/**
	return a symbol @p 's name to be used in assembly code.
		4		---->	$4
		str0		---->	.str0
 */
static char *GetAccessName(Symbol p) {
    if (p->access_name != NULL)
        return p->access_name;
    switch (p->symbol_kind) {
        case SK_Constant:
            // movl $4, -4(%ebp)
            p->access_name = FormatName("$%s", p->name);
            break;

        case SK_String:
        case SK_Label:
            // .str0:	.string	"%d \012"
            p->access_name = FormatName(".%s", p->name);
            break;

        case SK_Variable:
        case SK_Temp:
            if (p->level == 0 || p->storage_class == TK_EXTERN) {
                p->access_name = p->name;
            } else if (p->storage_class == TK_STATIC) {
                /**
                    int main(){
                        static int c = 5;
                        ...
                    }
                    c.0:	.long	5

                    Because it is illegal to declare a global var in C language as following:
                        int c.0;
                    So there is no conflict to generate a name 'c.0' for static variable in
                    assembly output.
                 */
                p->access_name = FormatName("%s.%d", p->name, TempNum++);
            } else {

                /**
                    typedef struct{
                        int arr[10];
                    }Data;
                    int main(){
                        Data dt ;
                        dt.arr[5] = 100;
                    }
                 */
                // movl 20(%ebp), %eax
                p->access_name = FormatName("%d(%%ebp)", AsVar(p)->offset);
            }
            break;

        case SK_Function:
            /**
                .globl	f
                f:
            **/
            p->access_name = p->name;
            break;

        case SK_Offset: {
            Symbol base = p->link;
            int n = AsVar(p)->offset;
            /**
                typedef struct{
                    int a;
                    int b;
                    int c;
                    int d;
                }Data;
                Data dt2;
                int main(){
                    Data dt1;
                    dt1.c = 100;
                    dt2.d = 200;
                    return 0;
                }


                .lcomm	__va_arg_tmp,4
                .comm	dt2,16

                .text
                .globl	main

                main:
                    pushl %ebp
                    pushl %ebx
                    pushl %esi
                    pushl %edi
                    movl %esp, %ebp
                    subl $24, %esp
                    leal -16(%ebp), %eax
                    movl $100, -8(%ebp)	----------- -8(%ebp)
                    leal dt2, %ecx
                    movl $200, dt2+12		-----------dt2 + 12
                    movl $0, %eax
                    jmp .BB0
            */
            if (base->level == 0 || base->storage_class == TK_STATIC || base->storage_class == TK_EXTERN) {
                p->access_name = FormatName("%s%s%d", GetAccessName(base), n >= 0 ? "+" : "", n);
            } else {
                /**
                    typedef struct{
                        int arr[10];
                    }Data;
                    int main(){
                        Data dt ;				----	dt is SK_Variable
                        dt.arr[5] = 100;		----  dt.arr[5] is SK_Offset
                    }
                 */
                n += AsVar(base)->offset;
                p->access_name = FormatName("%d(%%ebp)", n);
            }
        }
            break;

        default:
            assert(0);
    }

    return p->access_name;
}


void SetupRegisters(void) {
    /**
        ESP, EBP are used for stack,
        we don't have to CreateReg for them.
     */
    X86Regs[EAX] = CreateReg("%eax", "(%eax)", EAX);
    X86Regs[EBX] = CreateReg("%ebx", "(%ebx)", EBX);
    X86Regs[ECX] = CreateReg("%ecx", "(%ecx)", ECX);
    X86Regs[EDX] = CreateReg("%edx", "(%edx)", EDX);
    X86Regs[ESI] = CreateReg("%esi", "(%esi)", ESI);
    X86Regs[EDI] = CreateReg("%edi", "(%edi)", EDI);

    X86WordRegs[EAX] = CreateReg("%ax", NULL, EAX);
    X86WordRegs[EBX] = CreateReg("%bx", NULL, EBX);
    X86WordRegs[ECX] = CreateReg("%cx", NULL, ECX);
    X86WordRegs[EDX] = CreateReg("%dx", NULL, EDX);
    X86WordRegs[ESI] = CreateReg("%si", NULL, ESI);
    X86WordRegs[EDI] = CreateReg("%di", NULL, EDI);

    X86ByteRegs[EAX] = CreateReg("%al", NULL, EAX);
    X86ByteRegs[EBX] = CreateReg("%bl", NULL, EBX);
    X86ByteRegs[ECX] = CreateReg("%cl", NULL, ECX);
    X86ByteRegs[EDX] = CreateReg("%dl", NULL, EDX);
}

void PutASMCode(int code, Symbol opds[]) {
    /**
        For example:
            TEMPLATE(X86_JMP,      "jmp %0")
        If code is X86_JMP,
            fmt is "jmp %0"
     */
    char *fmt = ASMTemplate[code];
    int i;

    PutChar('\t');
    while (*fmt) {
        switch (*fmt) {
            case ';':
                PutString("\n\t");
                break;

            case '%':
                // 	Linux:
                // TEMPLATE(X86_MOVI4,    "movl %1, %0")
                fmt++;
                if (*fmt == '%') {
                    PutChar('%');
                } else {
                    i = *fmt - '0';
                    if (opds[i]->reg != NULL) {
                        PutString(opds[i]->reg->name);
                    } else {
                        PutString(GetAccessName(opds[i]));
                    }
                }
                break;

            default:
                PutChar(*fmt);
                break;
        }
        fmt++;
    }
    PutChar('\n');
}

void BeginProgram(void) {
    int i;

    ORG = 0;
    FloatNum = TempNum = 0;
    for (i = EAX; i <= EDI; ++i) {
        // Initialize register symbols to
        // make sure that no register contains data from variables.
        if (X86Regs[i] != NULL) {
            X86Regs[i]->link = NULL;
        }
    }

    PutString("# Code auto-generated by SCC\n\n");
}

void Segment(int seg) {
    if (seg == DATA) {
        PutString(".data\n\n");
    } else if (seg == CODE) {
        PutString(".text\n\n");
    }
}

/**
	 .globl  f

	 f:

 */
void Export(Symbol p) {
    Print(".globl\t%s\n\n", GetAccessName(p));
}

/**
	 .str0:  .string "%d \012"
	 .str1:  .string "a + b + c + d = %d.\012"

 */
void DefineString(String str, int size) {
    int i = 0;

    if (str->content[size - 1] == 0) {
        PutString(".string\t\"");
        size--;
    } else {
        /**
            char str[3] = "abc";
         */
        // PRINT_DEBUG_INFO(("%s",str->chs));
        PutString(".ascii\t\"");
    }
    while (i < size) {
        // if it is not printable.	ASCII value  <= 0x20 ?
        if (!isprint(str->content[i])) {
            /**
                printf("\\%03o", (unsigned char)12);
                    \014o
             */
            Print("\\%03o", (unsigned char) str->content[i]);
        } else {
            if (str->content[i] == '"') {

                //  \"
                PutString("\\\"");
            } else if (str->content[i] == '\\') {
                /**
                        When "make test",  Ugly Bug.
                        //	\\
                        PutString("\\\\");
                  */
                PutString("\\\\");
            } else {
                PutChar(str->content[i]);
            }
        }
        i++;
    }
    PutString("\"\n");
}

void DefineFloatConstant(Symbol p) {
    //	.flt1		.flt2
    p->access_name = FormatName(".flt%d", FloatNum++);

    Align(p);
    Print("%s:\t", p->access_name);
    DefineValue(p->symbol_type, p->val);
}


void DefineGlobal(Symbol p) {
    Align(p);
    if (p->storage_class != TK_STATIC) {
        // ".globl\t%s\n\n"
        // "PUBLIC %s\n\n"
        Export(p);
    }
    Print("%s:\t", GetAccessName(p));
}

void DefineCommData(Symbol p) {
    GetAccessName(p);
    if (p->storage_class == TK_STATIC) {
        /**
            #include <stdio.h>
            int a = 3;
            static int b;
         */
        Print(".lcomm\t%s,%d\n", p->access_name, p->symbol_type->size);
    } else {
        Print(".comm\t%s,%d\n", p->access_name, p->symbol_type->size);
    }
}

void DefineAddress(Symbol p) {
    Print(".long\t%s", GetAccessName(p));
}

/**
	 .data



	 .lcomm  __va_arg_tmp,4

	 .globl  ch
	 ch: .byte	 97

	 .align 2
	 .globl  s

	 s:  .word	 100

	 .globl  i
	 i:  .long	 300

	 .globl  lg
	 lg: .long	 500

	 .globl  f
	 f:  .long	 1065353216

	 .globl  d
	 d:  .long	 0
	 .long	 1074266112

*/
void DefineValue(Type ty, union value val) {
    int tcode = GetTypeCode(ty);

    switch (tcode) {
        case I1:
        case U1:
            Print(".byte\t%d\n", val.i[0] & 0xff);
            break;

        case I2:
        case U2:
            Print(".word\t%d\n", val.i[0] & 0xffff);
            break;

        case I4:
        case U4:
            // a:	.long	3
            Print(".long\t%d\n", val.i[0]);
            break;

        case F4:
            Print(".long\t%d\n", *(unsigned *) &val.f);
            break;

        case F8: {
            unsigned *p = (unsigned *) &val.d;
            Print(".long\t%d\n.long\t%d\n", p[0], p[1]);
            break;
        }

        default:
            assert(0);
    }
}

/**
	 typedef struct{
		 int a;
		 int b;
		 int c;
		 int d;
	 }Data;
	 Data dt2 = {100};


	 .data
	 .lcomm  __va_arg_tmp,4
	 .globl  dt2

	 dt2:	 .long	 100
		 .space  12

 */
void Space(int size) {
    Print(".space\t%d\n", size);
}

/**
		 jmp .BB0
	 .BB0:
		 movl %ebp, %esp
		 popl %edi
 */
void DefineLabel(Symbol p) {
    Print("%s:\n", GetAccessName(p));
}

void EndProgram(void) {
    Flush();
}