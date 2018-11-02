%include "asm/int_content.inc"
%include "drivers/terminal/rs232.inc"


[SECTION .data]
JmpTable:	dd		ModemStatus, WriteChar, ReadChar, LineStatus

[SECTION .text]
global RS1IntHandle
global RS2IntHandle

RS1IntHandle:
	IntoInt
	push	dword RS232_CHANNEL1_BASE
	jmp		RSIntHandle
RS2IntHandle:
	IntoInt
	push	dword RS232_CHANNEL2_BASE
		
RSIntHandle:
	mov		edx, [esp]
	add		edx, 2
	xor		eax, eax
	in		al, dx
	
	test	al, 1
	jne		End
	cmp		al, 6
	ja		End
	call	[JmpTable + eax * 2]
	jmp		RSIntHandle

End:
	mov		al, 0x20
	out		0x20, al
	
	add		esp, 4
	
	OutOfInt
	iret
	
ALIGN	2
ModemStatus:
	mov		edx, [esp + 4]
	add		edx, 6
	in		al, dx
	ret
	
ALIGN	2
LineStatus:
	mov		edx, [esp + 4]
	add		edx, 5
	in		al, dx
	ret

ALIGN	2
ReadChar:
	mov		edx, [esp + 4]
	push	edx
	call	RS232ReadCharC
	add		esp, 4
	
	ret

ALIGN	2
WriteChar:
	mov		edx, [esp + 4]
	push	edx
	call	RS232WriteCharC
	add		esp, 4
	
	ret
