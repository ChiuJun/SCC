//
// Created by LJChi on 2021/5/23.
//

#ifndef SCC_REG_H
#define SCC_REG_H

enum { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI };
//  indirect addressing   register,  [eax] or (%eax)
#define SK_IRegister (SK_Register + 1)
//  no register is satisfied
#define NO_REG -1

void StoreVar(Symbol reg, Symbol v);
void SpillReg(Symbol reg);
void ClearRegs(void);
Symbol CreateReg(char *name, char *iname, int no);
Symbol GetByteReg(void);
Symbol GetWordReg(void);
Symbol GetReg(void);

extern Symbol X86Regs[];
extern Symbol X86ByteRegs[];
extern Symbol X86WordRegs[];
// bit mask for register use
extern int UsedRegs;

#endif //SCC_REG_H
