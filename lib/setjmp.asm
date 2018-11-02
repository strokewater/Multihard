[SECTION .text]
global setjmp
global longjmp

setjmp:
	cli
	
	mov ecx, [esp + 4]
	mov edx, [esp]
	mov [ecx], edx
	
	mov [ecx + 4], ebx
	mov [ecx + 8], esp
	mov [ecx + 0xc], ebp
	mov [ecx + 0x10], esi
	mov [ecx + 0x14], edi
	
	xor eax, eax
	
	sti
	ret
longjmp:
	cli
	
	mov eax, [esp + 4]
	
	mov ecx, [eax]
	mov ebx, [eax + 4]
	mov esp, [eax + 8]
	mov ebp, [eax + 0xc]
	mov esi, [eax + 0x10]
	mov edi, [eax + 0x14]
	
	mov eax, [eax]
	test eax, 0
	jne .out
	
	inc eax
.out:
	
	sti
	jmp	ecx
