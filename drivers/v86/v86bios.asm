%include "asm/pm.inc"
%include "process.inc"

[SECTION .v86_text]
ALIGN	4096
%define NPAGE_V86_REAL	2
V86Entry:
; eax, ebx, ecx, edx, esi, edi
	int	0x10	; Call V86 BIOS Int.
	int	0x90	; Generate #GP
V86CodeLen	equ	$ - V86Entry

[SECTION .tmp_stk0]
TMP_STK0:	times	512	db	0
TMP_STK0_TOP	equ	$ - 4

[SECTION .data]
V86RealBaseVA		dd	0
V86RealBasePA		dd	0
V86EntrySeg		dd	0
V86EntryOffset		dd	0
OldEBP			dd	0
OldESP			dd	0
OldEFlags		dd	0

V86EIP			dd	0
V86CS			dd	0
V86EFlag		dd	0
TempStk0Top		dd	0
[SECTION .text]
global V86BIOSInt
global V86BIOSInit

extern DMalloc
extern GetDAreaPa
extern SetupIDTGate
extern TSSArea

V86BIOSInit:
	push	dword 4096 * NPAGE_V86_REAL
	call	DMalloc
	add	esp, 4
	mov	[V86RealBaseVA], eax
	push	eax
	call	GetDAreaPa
	add	esp, 4
	mov	[V86RealBasePA], eax

	add	eax, (NPAGE_V86_REAL- 1) * 4096 - 4
	mov	[TempStk0Top], eax

	mov	eax, [V86RealBasePA]
	shr	eax, 4
	mov	[V86EntrySeg], eax
	mov	dword [V86EntryOffset], 0

	mov	esi, V86Entry
	mov	edi, [V86RealBaseVA]
	mov	ecx, V86CodeLen
	mov	ax, ds
	mov	es, ax
	cld
	rep	movsb

	push	V86ExitV86Program
	push	SelectorFlatC
	push	0x90
	call	SetupIDTGate
	add	esp, 12

	mov	eax, cr4
	or	eax, 1
	mov	cr4, eax

	ret
V86BIOSInt:
; struct Reg
; void V86BIOSInt(struct V86BIOSIntReg *reg);
; During DO BIOS INT, IF is clear until begining to return.
	mov	[OldEBP], ebp
	mov	[OldESP], esp
	pushfd
	pop	eax
	mov	[OldEFlags], eax

	mov	eax, [TempStk0Top]
	mov	dword [TSSArea + 4], eax

	mov	ebx, [esp + 4]
	
	mov	eax, [ebx + 0]
	mov	ecx, [ebx + 8]
	mov	edx, [ebx + 12]
	mov	esi, [ebx + 16]
	mov	edi, [ebx + 20]
	mov	ebx, [ebx + 4]
	mov	ebp, [ebx + 24]

	push	dword 0		 	 ; GS
	push	dword 0		 	 ; FS
	push	dword [V86EntrySeg]	 ; DS
	push	dword ebp	 	 ; ES
	push	dword [V86EntrySeg]	 ; SS
	push	dword NPAGE_V86_REAL * 4096 - 4	 ; ESP
	push	dword 0x23000		 ; EFlags, set VM
	push	dword [V86EntrySeg]	 ; CS
	push	dword [V86EntryOffset] 	 ; EIP
	iretd

FinishV86BIOSInt:
	sti
	ret

V86ExitV86Program:
	mov	ebp, SelectorFlat
	mov	ds, bp
	mov	es, bp
	mov	ss, bp
	
	mov	dword [TSSArea + 4], PROCESS_STK0_TOP - 4
	mov	esp, [OldESP]
	
	mov	ebp, [esp + 4]
	
	mov	[ebp + 0], eax
	mov	[ebp + 4], ebx
	mov	[ebp + 8], ecx
	mov	[ebp + 12], edx
	mov	[ebp + 16], esi
	mov	[ebp + 20], edi

	mov	ebp, [OldEFlags]
	push 	ebp	; False int information for iret:eflags.
	mov	ebp, [OldEBP]
	push	dword SelectorFlatC
	push	dword FinishV86BIOSInt
	iret

