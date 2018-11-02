%include "mm/mm.inc"
%include "asm/pm.inc"

[SECTION .data]
global MemLower
global MemUpper

MULTI_MAGIC			equ	0x1BADB002
MULTI_FLAGS			equ	3
MULTI_LOADED_MAGIC		equ	0x2BADB002

MulitbootInfo	dd	0
Stack:	times	PAGE_SIZE	db	0
StackTop	equ		Stack + PAGE_SIZE - 4

ALIGN PAGE_SIZE
global PDIRS
PDIRS:		times NR_PDIRS_PER_PROCESS	dd	0

[SECTION .gdt]
ALIGN PAGE_SIZE
LOADER_GDT_NULL:       Descriptor 0,              0, 0                      ; 空描述符
LOADER_GDT_FLAT_C:  	Descriptor 0,        0fffffh, DA_CR|DA_32|DA_LIMIT_4K ; 0~4G
LOADER_GDT_FLAT: 		Descriptor 0,        0fffffh, DA_DRW|DA_32|DA_LIMIT_4K      ; 0~4G

LoaderGdtLen		equ	$ - LOADER_GDT_NULL	; GDT长度
LoaderGdtPtr		dw	LoaderGdtLen - 1		; GDT界限
					dd	LOADER_GDT_NULL			; GDT基地址

LoaderSelectorFlatC	equ		8
LoaderSelectorFlat	equ		16

[SECTION .text]

global _start
extern Main

Multiboot:
Magic:		dd		MULTI_MAGIC
Flags:		dd		MULTI_FLAGS
Checksum:	dd		-(MULTI_MAGIC + MULTI_FLAGS)

_start:
	mov	esp, StackTop

	cmp	eax, MULTI_LOADED_MAGIC
	je	.2
	jmp	$
.2:

	mov	eax, LoaderGdtPtr
	lgdt [eax]
	jmp LoaderSelectorFlatC:SetupSelectorFlat
SetupSelectorFlat:
	mov	ax, LoaderSelectorFlat
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	gs, ax
	mov	fs, ax

	push	ebx
	call	Main
	jmp	$
