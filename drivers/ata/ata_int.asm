%include "asm/int_content.inc"

[SECTION .text]
global AtaDevIRQIntASM
extern AtaDevIRQIntC

AtaDevIRQIntASM:
	IntoInt

	mov	al, 0x20
	out	0x20, al
	out	0xA0, al

	call	AtaDevIRQIntC

	OutOfInt
	iret
