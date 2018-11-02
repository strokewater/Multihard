#ifndef FPU_SSE_H
#define FPU_SSE_H

#define SSE_IMAGE_SIZE	512
#define FPU_IMAGE_SIZE	108

#define FPUSSE_ENVIRON_NOTHING	0
#define FPUSSE_ENVIRON_FPU		1
#define FPUSSE_ENVIRON_SSE		2

extern struct Process *LastProcessUseMath;

void CaseFPUSEEInfo();
void DoLaterCaseForFPUSSE();
void FPUSSEInit();

#endif
