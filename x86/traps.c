#include "stdlib.h"
#include "types.h"
#include "printk.h"
#include "sched.h"
#include "exception.h"
#include "asm/pm.h"
#include "asm/traps.h"
#include "asm/fpu_sse.h"

void DoDoubleFaultError(u32 esp, u32 error_code)
{
	die("double fault.");
}

void DoGeneralProtectionError(u32 esp, u32 error_code)
{
	printk("Error Code = %d", error_code);
	printk("ESP0 = %x\n", esp);
	printk("Old EFlags = %x\n", *((int *)esp + 2));
	die("general protection.");
}

void DoDivideError(u32 esp, u32 error_code)
{
	die("divide error.");
}

void DoInt3Error(u32 esp, u32 error_code)
{
	die("int3 error. unsupported now.");
}

void DoNMIError(u32 esp, u32 error_code)
{
	die("NMI error.");
}

void DoDebugError(u32 esp, u32 error_code)
{
	die("debug error.");
}

void DoOverFlowError(u32 esp, u32 error_code)
{
	die("overflow error.");
}

void DoBoundsError(u32 esp, u32 error_code)
{
	die("bounds error.");
}

void DoInvalidOPError(u32 esp, u32 error_code)
{			
	die("invalid operand error.");
}

void DoDeviceNotAvailableError(u32 esp, u32 error_code)
{
	CaseFPUSEEInfo();
}

void DoCoprocessorSegmentOverRunError(u32 esp, u32 error_code)
{
	die("coprocessor segment overrun error.");
}

void DoInvalidTSSError(u32 esp, u32 error_code)
{
	die("invalid TSS error.");
}

void DoSegmentNotPresentError(u32 esp, u32 error_code)
{
	die("segment not present error.");
}

void DoStackSegmentError(u32 esp, u32 error_code)
{
	die("stack segment error.");
}

void DoCoProcessorError(u32 esp, u32 error_code)
{
	die("coprocessor error.");
}

void DoReserved(u32 esp, u32 error_code)
{
	die("reserved (15, 17-47) error.");
}

void DoUnImpError(u32 esp, u32 error_code)
{
	die("unimplement int.");
}

void DoPageFaultError(u32 esp, u32 error_code)
{
	die("page fault.");
}

void DoAlignCheckError(u32 esp, u32 error_code)
{
	die("alignment check.");
}

void DoMachineCheckError(u32 esp, u32 error_code)
{
	die("machine check.");
}

void DoSIMDExceptionError(u32 esp, u32 error_code)
{
	die("SIMD exception.");
}

void TrapsInit()
{
	SetupIDTGate(0, SelectorFlatC, (u32)&DivideError);
	SetupIDTGate(1, SelectorFlatC, (u32)&DebugError);
	SetupIDTGate(2, SelectorFlatC, (u32)&NMIError);
	SetupIDTGate(3, SelectorFlatC, (u32)&Int3Error);
	SetupIDTGate(4, SelectorFlatC, (u32)&OverFlowError);
	SetupIDTGate(5, SelectorFlatC, (u32)&BoundsError);
	SetupIDTGate(6, SelectorFlatC, (u32)&InvalidOPError);
	SetupIDTGate(7, SelectorFlatC, (u32)&DeviceNotAvailableError);
	SetupIDTGate(8, SelectorFlatC, (u32)&DoubleFaultError);
	SetupIDTGate(9, SelectorFlatC, (u32)&CoprocessorSegmentOverRunError);
	SetupIDTGate(10, SelectorFlatC, (u32)&InvalidTSSError);
	SetupIDTGate(11, SelectorFlatC, (u32)&SegmentNotPresentError);
	SetupIDTGate(12, SelectorFlatC, (u32)&StackSegmentError);
	SetupIDTGate(13, SelectorFlatC, (u32)&GeneralProtectionError);
	SetupIDTGate(14, SelectorFlatC, (u32)&PageFaultError);
	SetupIDTGate(16, SelectorFlatC, (u32)&CoprocessorError);
	SetupIDTGate(17, SelectorFlatC, (u32)&AlignCheckError);
	SetupIDTGate(18, SelectorFlatC, (u32)&MachineCheckError);
	SetupIDTGate(19, SelectorFlatC, (u32)&SIMDExceptionError);
}
