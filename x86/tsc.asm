[SECTION .data]
StartupTSC:			dq		0
TSCTailTimeNS:		dd		0
TSCFullTimeNS:		dd		0
CPUFrequencyMHZ:	dd		1

[SECTION .text]

global InitTSC
global GetTSCTailTime
global SetStartupTailTime
global Bits64MulBits16
global Bits128DivBits16

GetCPUFrequency:
	push	ebp
	mov		ebp, esp
	sub		esp, 8
%define OLD_TSC_LOW		(- 4)
%define OLD_TSC_HIGH	(- 8)

	rdtsc
	mov		[ebp + OLD_TSC_LOW], eax
	mov		[ebp + OLD_TSC_HIGH], edx
	
	mov		al, 0
	out		0x70, al
	in		al, 0x71
	mov		cl, al
.AlignTime:
	mov		al, 0
	out		0x70, al
	in		al, 0x71
	cmp		al, cl
	je		.AlignTime
	
	rdtsc
	mov		ebx, [ebp + OLD_TSC_HIGH]
	sub		edx, ebx
	mov		ebx, [ebp + OLD_TSC_LOW]
	sub		eax, ebx
	
	mov		ebx, 1000
	div 	ebx
	
	mov		[CPUFrequencyMHZ], eax

	add		esp, 8
	pop		ebp
	
	ret

InitTSC:
	rdtsc 
	mov		[StartupTSC], eax
	mov		[StartupTSC + 4], edx
	mov		eax, [esp + 4]
	
	mov		eax, [esp + 4]
	mov		[TSCTailTimeNS], eax
	
	mov		eax, [esp + 8]
	mov		[TSCFullTimeNS], eax
	
	call	GetCPUFrequency
	
	ret
	
Bits64MulBits16:
	; void Bits64MulBits32(bits64 mula_64, bits32 mulb_16, bits128 *result)
	push	ebp
	mov		ebp, esp
	sub		esp, 4

%define MUL_A_OFFSET  (4 + 4)
%define MUL_B_OFFSET  (4 + 4 + 8)
%define RESULT_OFFSET (4 + 4 + 8 + 4)
%define CARRY_OFFSET  (-4)
%define LOOP_CNT	  (64 / 16)

	mov		esi, 0
	mov 	edi, 0
	mov		ecx, LOOP_CNT
	mov		dword [ebp + CARRY_OFFSET], 0
.Calc:
	mov		ax, word [ebp + MUL_A_OFFSET + esi * 2]
	mov		ebx, [ebp + MUL_B_OFFSET]
	mul		bx
	add 	ax, word [ebp + CARRY_OFFSET]
	adc		dx, 0
	mov		word [ebp + CARRY_OFFSET], dx
	
	mov		ebx, [ebp + RESULT_OFFSET]
	mov		[ebx + edi * 2], ax
	
	inc		esi
	inc		edi
	loop 	.Calc
	
	mov		eax, [ebp + CARRY_OFFSET]
	mov		ebx, [ebp + RESULT_OFFSET]
	mov		[ebx + edi * 2], eax
	
	add		esp, 4
	pop		ebp
	ret

%undef LOOP_CNT
%undef CARRY
%undef MUL_A_OFFSET
%undef MUL_B_OFFSET
%undef RESULT_OFFSET

Bits128DivBits16:
	; void Bits128DivBits32(bits128 dividend, bits32 divisor, bits64 *quotient, bits32 *remainder)
	
	push	ebp
	mov		ebp, esp
	
	sub		esp, 4
%define DIVIDEND_OFFSET			8
%define DIVISOR_OFFSET			(8 + 16)
%define QUOTIENT_OFFSET			(8 + 16 + 4)
%define REMAINDER_RET_OFFSET	(8 + 16 + 4 + 4)
%define REMAINDER_OFFSET		(-4)

%define LOOP_CNT	(128 / 16)
	
	mov		ecx, LOOP_CNT
	mov		esi, LOOP_CNT - 1
	mov		edi, LOOP_CNT - 1
	mov		dword [ebp + REMAINDER_OFFSET], 0
.Calc:
	mov		ax, word [ebp + DIVIDEND_OFFSET + esi * 2]
	mov		dx, [ebp + REMAINDER_OFFSET]
	mov		bx, [ebp + DIVISOR_OFFSET]
	div		bx
	
	mov		[ebp + REMAINDER_OFFSET], dx
	
	mov		ebx, [ebp + QUOTIENT_OFFSET]
	cmp		ebx, 0
	jne		.StoreQuoient
	jmp		.FinishQuoient
.StoreQuoient:
	mov		word [ebx + edi * 2], ax
.FinishQuoient:

	dec		esi
	dec		edi
	loop	.Calc
	
	mov		ebx, [ebp + REMAINDER_RET_OFFSET]
	cmp		ebx, 0
	jne		.StoreRemainder
	jmp		.FinishRemainder
.StoreRemainder:
	mov		eax, [ebp + REMAINDER_OFFSET]
	mov		[ebx], eax
.FinishRemainder:

	add		esp, 4
	pop		ebp
	ret

	
GetTSCTailTime:
	push	ebp
	mov		ebp, esp
	
	sub		esp, 16
%define CalcTempOffset		(-16)

	rdtsc
	sub		eax, [StartupTSC]
	sub		edx, [StartupTSC + 4]
	
	; 计算已经过去的时间(纳秒) Time = Diff * (1 / (Freq * 10^6)) * 10^9
	;							= (Diff * 10^3) / Freq
	;	10^6是因为CPU主频单位为MHZ, 10^9是因为时间单位为纳秒
	;	Diff为TSC差值
	
	lea		ebx, [ebp + CalcTempOffset]
	push	ebx
	mov		ebx, 1000
	push	ebx
	push	edx
	push	eax
	call	Bits64MulBits16
	add		esp, 16
	
	push	dword 0			; 不需要余数
	lea		ebx, [ebp + CalcTempOffset]
	push	ebx				; 计算结果存在CalcTemp
	mov		ebx, [CPUFrequencyMHZ]
	push	ebx				; 除数为CPU主频
	
	; 将CalcTemp压入栈
	mov		ebx, [ebp + CalcTempOffset + 12]
	push	ebx
	mov		ebx, [ebp + CalcTempOffset + 8]
	push	ebx
	mov		ebx, [ebp + CalcTempOffset + 4]
	push	ebx
	mov		ebx, [ebp + CalcTempOffset]
	push	ebx
	
	call	Bits128DivBits16
	add		esp, 28
	
	mov		edx, [ebp + CalcTempOffset + 4]
	mov		eax, [ebp + CalcTempOffset]
	; 最终Time存在(EDX: EAX)
	
	; VaildTime = Time % TSCFullTimeNS
	; TSCFullTimeNS 不会超过32位，因此ValidTime只需要32位就OK
	; 但除数很有可能大于32位因而导致除法异常
	
	lea		ebx, [ebp + CalcTempOffset]
	push	ebx	; 求余数
	push	dword 0 ; 商不需要
	mov		ebx, [TSCFullTimeNS]
	push	ebx ; 除数
	
	push	dword 0
	push	dword 0
	push	edx
	push	eax	; 被除数
	
	call	Bits128DivBits16
	add		esp, 28
	mov		eax, [ebp + CalcTempOffset] ; 取出余数
	
	; Tail = VaildTime / TSCTailTimeNS
	xor		edx, edx
	mov		ebx, [TSCTailTimeNS]
	div		ebx
	
	add	esp, 16
	pop	ebp
	
	ret

SetStartupTailTime:
	; = 10^6 * CpuFrequency * TailTime * 10^-9
	; = CpuFrequency * TailTime * 10^-3
	mov		eax, [esp + 4]
	mov		ebx, [CPUFrequencyMHZ]
	mul		ebx
	mov		ebx, 1000
	div		ebx
	
	mov		[StartupTSC], eax
	mov		dword [StartupTSC + 4], 0
	
	ret
