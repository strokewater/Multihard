#ifndef INT_CONTENT_H
#define INT_CONTENT_H

#include "types.h"

struct IntContent
{
	struct IntContent *previous;
	u32 fs;
	u32 es;
	u32 ds;
	u32 edx;
	u32 ecx;
	u32 ebx;
	u32 eax;
	u32 ebp;
	u32 edi;
	u32 esi;
	u32 eip;
	u32 cs;
	u32 eflags;
	u32 esp;
	u32 ss;
};

extern struct IntContent *CurIntContent;

#endif