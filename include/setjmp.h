#ifndef LIB_SETJMP_H
#define LIB_SETJMP_H

#include "types.h"

typedef struct s_jmp_buf
{
	u32 eip;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
} jmp_buf;

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#endif
