%include "asm/int_content.inc"

[SECTION .text]
global KeyboardInt
extern DoWithScanCode
extern debug_printk
KeyboardInt:
	IntoInt

	xor	eax, eax
	in	al, 0x60
	push	eax
	call	DoWithScanCode
	add	esp, 4

	in	al, 0x61

	or	al, 0x80
	out	0x61, al
	nop
	nop
	and	al, 0x7f
	out	0x61, al

	mov	al, 0x20
	out	0x20, al

	OutOfInt

	iret
