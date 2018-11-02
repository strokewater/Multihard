#include "asm/fpu_sse.h"
#include "stdlib.h"
#include "sched.h"
#include "mm/varea.h"

static int CurFPUSSEEnviron;
struct Process *LastProcessUseMath = NULL;

int GetTS()
{
	int cr0;
	__asm__("movl %%cr0, %%eax" : "=a"(cr0) : :);
	return cr0 & (1 << 3);	
}

int GetEM()
{
	int cr0;
	__asm__("movl %%cr0, %%eax" : "=a"(cr0) : :);
	return cr0 & (1 << 2);	
}

int GetOSFXSR()
{
	int cr4;
	__asm__("movl %%cr4, %%eax" : "=a"(cr4) : :);
	return cr4 & (1 << 9);	
}

void DoLaterCaseForFPUSSE()
{
	int cr0;
	__asm__("movl %%cr0, %%eax" : "=a"(cr0) : :);
	cr0 = cr0 | (1 << 3);	// SET TS
	__asm__("movl %%eax, %%cr0" : : "a"(cr0) :);
}

static void FinishLaterCaseForFPUSSE()
{
	int cr0;
	__asm__("movl %%cr0, %%eax" : "=a"(cr0) : :);
	cr0 = cr0 & ~(1 << 3);	// CLEAR TS
	__asm__("movl %%eax, %%cr0" : : "a"(cr0) :);
}

void CaseFPUSEEInfo()
{
	FinishLaterCaseForFPUSSE();
	if (LastProcessUseMath == Current)
		return;
	if (CurFPUSSEEnviron == FPUSSE_ENVIRON_FPU)
	{
		__asm__("fwait");
		if (LastProcessUseMath)	
			__asm__("fnsave %0" : : "m"(*(LastProcessUseMath->fpu_sse)) : );
		LastProcessUseMath = Current;
		
		if (Current->fpu_sse)
			__asm__("frstor %0"::"m"(*(Current->fpu_sse)):);
		else
		{
			__asm__("fninit":::);
			Current->fpu_sse = KMalloc(FPU_IMAGE_SIZE);
		}
	} else if (CurFPUSSEEnviron == FPUSSE_ENVIRON_SSE)
	{
		if (LastProcessUseMath)	
			__asm__("fxsave %0" : : "m"(*(LastProcessUseMath->fpu_sse)) : );
		LastProcessUseMath = Current;
		
		if (Current->fpu_sse)
			__asm__("fxrstor %0"::"m"(*(Current->fpu_sse)):);
		else
		{
			__asm__("fninit":::);	// for fpu
			Current->fpu_sse = KMalloc(SSE_IMAGE_SIZE);
		}		
	}
}

void FPUSSEInit()
{
	int cr0;
	__asm__("movl %%cr0, %%eax" :"=a"(cr0) : :);
	if ((cr0 >> 4) & 0x1)
	{
		CurFPUSSEEnviron = FPUSSE_ENVIRON_FPU;
		cr0 = cr0 | ( 1 << 5);	// enable NE
		cr0 = cr0 & ~(1 << 2);	// disable EM
		cr0 = cr0 | ( 1 << 1);	// enable MP
		cr0 = cr0 & ~(1 << 3);	// disable TS
		
		__asm__("movl %%eax, %%cr0" : :"a"(cr0) :);
	} else
	{
		debug_printk("not fpu and kernel do not include software fpu.");
		CurFPUSSEEnviron = FPUSSE_ENVIRON_NOTHING;
		return;
	}


	int cpuid_leaf;
	__asm__("cpuid" : "=d"(cpuid_leaf) : "a"(1) :);
	if ((cpuid_leaf >> 25) & 0x1)
	{
		CurFPUSSEEnviron = FPUSSE_ENVIRON_SSE;
		int cr4;
		__asm__("movl %%cr4, %%eax" :"=a"(cr4) : :);
		cr4 = cr4 | (1 << 9);	// enable OSFXSR
		cr4 = cr4 | (1 << 10);	// enable OSXMMEXCEPT
		__asm__("movl %%eax, %%cr4" : : "a"(cr4) : );
	} 

}
