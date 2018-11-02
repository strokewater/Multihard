#ifndef ASM_H
#define ASM_H

#include "sched.h"
#include "types.h"

void ASMLGDT(void *gdt_ptr);
void ASMLIDT(void *idt_ptr);
int ASMInByte(int port);
void ASMOutByte(int port, int data);
int ASMInWord(int port);
void ASMOutWord(int port, int data);
void ASMInvlPg(void *va);
void SwitchTo(struct Process *p);
void ClockHandle();
void MoveToIdle();

#define ASMCli()	{__asm__("cli;":::);}
#define ASMSti()	{__asm__("sti;":::);}

#endif
