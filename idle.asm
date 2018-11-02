%include "asm/pm.inc"
%include "syscall.inc"
global IdleText
extern ASyncKernelPart
extern InitC

[SECTION .data]
%define PAGE_SIZE	4096

InitFileName		db	"/usr/sbin/init", 0
InitArgvs		dd	InitFileName, 0
InitEnvps		dd	0

[SECTION .text]
global IdleText

ALIGN	PAGE_SIZE
IdleText:
	xor	eax, eax
	mov	ax, SelectorFlat3
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax

	mov	eax, SYS_CALL_NO_FORK
	int	0x80

	cmp	eax, 0
	je	.Init
.Idle:
	nop
	nop
	jmp	.Idle
.Init:
	mov	eax, SYS_CALL_NO_SETUP
	int	0x80	; sys_setup()
	
	mov	eax, SYS_CALL_NO_EXECV
	mov	ebx, InitFileName
	mov 	ecx, InitArgvs
	mov 	edx, InitEnvps
	int	0x80

	jmp	$
