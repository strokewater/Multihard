%include "asm/pm.inc"

global DivideError
global DebugError
global NMIError
global Int3Error
global OverFlowError
global BoundsError
global InvalidOPError
global DeviceNotAvailableError
global DoubleFaultError
global CoprocessorSegmentOverRunError
global InvalidTSSError
global SegmentNotPresentError
global StackSegmentError
global GeneralProtectionError
global CoprocessorError
global PageFaultError
global AlignCheckError
global MachineCheckError
global SIMDExceptionError
global SystemCall
global RetFromSysCall

extern DoDivideError
extern DoDebugError
extern DoNMIError
extern DoInt3Error
extern DoOverFlowError
extern DoBoundsError
extern DoInvalidOPError
extern DoDeviceNotAvailableError
extern DoDoubleFaultError
extern DoCoprocessorSegmentOverRunError
extern DoInvalidTSSError
extern DoSegmentNotPresentError
extern DoStackSegmentError
extern DoGeneralProtectionError
extern DoCoprocessorError
extern DoUnImpError
extern DoPageFaultError
extern DoAlignCheckError
extern DoMachineCheckError
extern DoSIMDExceptionError

DivideError:
	push	DoDivideError
NoErrorCode:
	xchg	[esp], eax
	push	ebx
	push	ecx
	push	edx
	push	edi
	push	esi
	push	ebp
	push	ds
	push	es
	push	fs
	push	0
	lea	edx, [esp+44]
	push	edx

	mov	edx, SelectorFlat
	mov	ds, dx
	mov	es, dx
	mov	fs, dx
	call	eax
	add	esp, 8

	pop	fs
	pop	es
	pop	ds
	pop	ebp
	pop	esi
	pop	edi
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	iret
	
DebugError:
	push	DoDebugError
	jmp	NoErrorCode
NMIError:
	push	DoNMIError
	jmp	NoErrorCode
Int3Error:
	push	DoInt3Error
	jmp	NoErrorCode
OverFlowError:
	push	DoOverFlowError
	jmp	NoErrorCode
BoundsError:
	push	DoBoundsError
	jmp	NoErrorCode
InvalidOPError:
	push	DoInvalidOPError
	jmp	NoErrorCode
DeviceNotAvailableError:
	push	DoDeviceNotAvailableError
	jmp	NoErrorCode
CoprocessorError:
	push	DoUnImpError
	jmp	NoErrorCode
MachineCheckError:
	push	DoMachineCheckError
	jmp	NoErrorCode
SIMDExceptionError:
	push	DoSIMDExceptionError
	jmp	NoErrorCode

DoubleFaultError:
	push	DoDoubleFaultError
	jmp	ErrorCode
ErrorCode:
	xchg	[esp+4], eax
	xchg	[esp], ebx
	push	ecx
	push	edx
	push	edi
	push	esi
	push	ebp
	push	ds
	push	es
	push	fs
	push	eax
	lea	eax, [esp+44]
	push	eax

	mov	eax, SelectorFlat
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	call	ebx

	add	esp, 8
	pop	fs
	pop	es
	pop	ds
	pop	ebp
	pop	esi
	pop	edi
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	iret
CoprocessorSegmentOverRunError:
	push	DoCoprocessorSegmentOverRunError
	jmp	NoErrorCode
InvalidTSSError:
	push	DoInvalidTSSError
	jmp	ErrorCode
SegmentNotPresentError:
	push	DoSegmentNotPresentError
	jmp	ErrorCode
StackSegmentError:
	push	DoStackSegmentError
	jmp	ErrorCode
GeneralProtectionError:
	push	DoGeneralProtectionError
	jmp	ErrorCode
PageFaultError:
	push	DoPageFaultError
	jmp	ErrorCode
AlignCheckError:
	push	DoAlignCheckError
	jmp	ErrorCode