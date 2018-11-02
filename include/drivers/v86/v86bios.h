#ifndef V86_BIOS_H
#define V86_BIOS_H

struct V86BIOSIntReg
{
	int eax;
	int ebx;
	int ecx;
	int edx;
	int esi;
	int edi;
	int es;
};

void V86BIOSInt(struct V86BIOSIntReg *reg);
void V86BIOSInit();

#endif
