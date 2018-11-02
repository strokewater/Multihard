%include "sched.inc"
%include "asm/pm.inc"
%include "idle.inc"
%include "asm/int_content.inc"

extern Current
extern Schedule
extern Tss
extern scr3
extern Idle
extern jiffies
extern CheckTimer
extern debug_printk

global ASMLGDT
global ASMLIDT
global SwitchTo
global MoveToIdle
global ClockHandle
global ASMInByte
global ASMOutByte
global ASMInWord
global ASMOutWord
global ASMInvlPg

global SystemCall
global RetFromSysCall
global ClockHandle

extern DoSignal
extern SysCallTable

[SECTION .data]
SysCallMsg:		db	"[SYS] SysCallNo == %d", 0

[SECTION .text]


ASMLGDT:
	mov	eax, [esp+4]
	lgdt	[eax]
	ret
ASMLIDT:
	mov	eax, [esp+4]
	lidt	[eax]
	ret
ASMOutByte:
	xor	edx, edx
	xor	eax, eax
	mov	edx, [esp+4]
	mov	eax, [esp+8]
	out	dx, al
	ret

ASMInByte:
	xor	edx, edx
	xor	eax, eax
	mov	edx, [esp+4]
	in	al, dx
	ret
	
ASMInWord:
	xor	edx, edx
	xor	eax, eax
	mov	edx, [esp+4]
	in	ax, dx
	ret

ASMOutWord:
	xor	eax, eax
	xor	edx, edx
	mov	edx, [esp+4]
	mov	eax, [esp+8]
	out	dx, ax
	ret
	
ASMInvlPg:
	mov	eax, [esp+4]
	invlpg	[eax]
	ret

SystemCall:
	IntoInt
	shl	eax, 2
	add	eax, SysCallTable
SysCallCommon:
	push	edi
	push	esi
	push	edx
	push	ecx
	push	ebx
	
	call	[eax]
	add	esp, 20
	mov	[esp+INT_CONTENT_OFFSET_EAX], eax
	mov	eax, Current
	jmp	RetFromSysCall
ClockHandle:
	IntoInt
	mov	al, 0x20
	out	0x20, al	;OCW2 to 0x20 port, send EOI

	inc	dword [jiffies]
	call	CheckTimer

	mov	ebx, Current
	mov	ebx, [ebx]
	mov	eax, [esp + INT_CONTENT_OFFSET_CS]
	cmp	eax, SelectorFlatC
	jne	.ring3	; ring0 schedule itself.
.ring0:
	inc	dword [ebx + PROCESS_OFFSET_STIME]
	jmp	RetFromSysCall
.ring3:
	inc	dword [ebx + PROCESS_OFFSET_UTIME]
	cmp 	ebx, Idle
	je 	.1
	dec 	dword [ebx + PROCESS_OFFSET_COUNTER]
	mov	eax, [ebx + PROCESS_OFFSET_COUNTER]
	cmp	eax, 0
	jne	.Out
	mov 	eax, [ebx + PROCESS_OFFSET_TOTAL_COUNTER]
	mov 	[ebx + PROCESS_OFFSET_COUNTER], eax
.1:
	call	Schedule
	jmp	RetFromSysCall
.Out:
RetFromSysCall:
	mov	eax, Current
	mov 	eax, dword [eax]
	mov	ebx, [PROCESS_OFFSET_SIGNAL+eax]
	mov	ecx, [PROCESS_OFFSET_BLOCKED+eax]
	not	ecx
	and	ecx, ebx
	bsf	ecx, ecx

	je	.1
	btr	ebx, ecx
	mov	[PROCESS_OFFSET_SIGNAL+eax], ebx
	inc	ecx
	push	ecx
	call	DoSignal
	pop	eax
.1:
	OutOfInt
	iretd

SwitchTo:
; case cr3
;  jmp to
; save eflags
;  save esp
;  save ebp
	mov	ebx, [esp+4]
	mov 	eax, [Current]
	cmp	ebx, eax
	je	.Out

	;cli
	mov	ecx, [CurIntContent]
	mov	[eax + PROCESS_OFFSET_INT_CONTENT], ecx
	mov	ecx, [ebx + PROCESS_OFFSET_INT_CONTENT]
	mov	[CurIntContent], ecx
	;sti

	mov	[eax + PROCESS_OFFSET_EBP], ebp

	pushfd
	pop	ecx
	mov	[eax + PROCESS_OFFSET_EFLAGS], ecx

	mov	ecx, esp
	mov	[eax + PROCESS_OFFSET_ESP], ecx

	mov	eax, [ebx + PROCESS_OFFSET_CR3]
	mov	cr3, eax
	mov 	[scr3], eax

	mov	eax, [ebx + PROCESS_OFFSET_ESP]
	mov	esp, eax

	mov	eax, [ebx + PROCESS_OFFSET_EFLAGS]
	push	eax
	popfd

	mov	eax, [ebx + PROCESS_OFFSET_EBP]
	mov	ebp, eax
.Out:
	mov [Current], ebx
	ret

MoveToIdle:
	mov	eax, SelectorTSS
	ltr	ax
	push	SelectorFlat3 | SA_RPL3
	push	IDLE_STK_TOP
	push	0x200 ; only set IF
	push	SelectorFlatC3 | SA_RPL3
	push	IDLE_TEXT_BASE
	sti
	iret
