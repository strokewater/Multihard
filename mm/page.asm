%include "asm/pm.inc"
%include "asm/int_content.inc"

global PageFault
extern DoNoPage
extern DoWpPage

[SECTION .text]
PageFault:
	xchg	[esp], eax
	push	esi
	push	edi
	push	ebx
	push	ecx
	push	edx
	push	ds
	push	es
	push	fs
	
	mov	edx, SelectorFlat
	mov	ds, dx
	mov	es, dx
	mov	fs, dx

	mov	edx, cr2
	push	edx
	push	eax
	test eax, 1
	jne .inwp
	call DoNoPage
	jmp .out
.inwp:
	call DoWpPage
.out:
	add	esp, 8
	
	pop	fs
	pop	es
	pop	ds
	pop	edx
	pop	ecx
	pop	ebx
	pop	edi
	pop	esi
	pop	eax
	iret
