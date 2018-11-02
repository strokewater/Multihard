%ifndef INT_ENV_INC
%define INT_ENV_INC

%include "asm/pm.inc"

%ifndef __EXTERN_CUR_INT_CNT__
extern CurIntContent;
%define __EXTERN_CUR_INT_CNT__
%endif

%macro IntoInt 0
	push	esi
	push	edi
	push	ebp
	push	eax
	push	ebx
	push	ecx
	push	edx
	push	ds
	push	es
	push	fs

	push	eax
	mov	eax, SelectorFlat
	mov	ds, ax
	mov	es, ax
	mov	fs, ax

	mov	eax, [CurIntContent]
	xchg	eax, [esp]
	mov	[CurIntContent], esp


%endmacro

%macro OutOfInt 0
	pop	eax
	mov	[CurIntContent], eax

	pop	fs
	pop	es
	pop	ds
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	pop	ebp
	pop	edi
	pop	esi

%endmacro

%define INT_CONTENT_OFFSET_PREVIOUS	0
%define INT_CONTENT_OFFSET_FS		4
%define INT_CONTENT_OFFSET_ES		8
%define INT_CONTENT_OFFSET_DS		12
%define INT_CONTENT_OFFSET_EDX		16
%define INT_CONTENT_OFFSET_ECX		20
%define INT_CONTENT_OFFSET_EBX		24
%define INT_CONTENT_OFFSET_EAX		28
%define INT_CONTENT_OFFSET_EBP		32
%define INT_CONTENT_OFFSET_EDI		36
%define INT_CONTENT_OFFSET_ESI		40
%define INT_CONTENT_OFFSET_EIP		44
%define INT_CONTENT_OFFSET_CS		48
%define INT_CONTENT_OFFSET_EFLAGS	52
%define INT_CONTENT_OFFSET_ESP		56
%define INT_CONTENT_OFFSET_SS		60

%endif