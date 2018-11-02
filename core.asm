%include "mm/mm.inc"

[SECTION .data]
global MemLower
global MemUpper

DEP_VERSION			equ	1

MULTI_INFO_MEM_LOWER		equ	4
MULTI_INFO_MEM_UPPER		equ	8
MULTI_INFO_CMDLINE		equ	16

MemLower:	dd		0
MemUpper:	dd		0

CoreStack:	times	PAGE_SIZE * 2	db	0
CoreStackTop	equ	CoreStack + PAGE_SIZE * 2 - 4

[SECTION .text]

global _start
extern Main
extern GetConfig

_start:

	mov	ebx, [esp+4]
	mov	eax, dword [ebx + MULTI_INFO_MEM_LOWER]
	mov	[MemLower], eax
	mov	eax, dword [ebx + MULTI_INFO_MEM_UPPER]
	mov	[MemUpper], eax
	
	mov  	eax, dword [ebx + MULTI_INFO_CMDLINE]
	mov		esp, CoreStackTop
	push 	eax
	call 	GetConfig
	add	esp, 4
	
	jmp	Main
	jmp	$
