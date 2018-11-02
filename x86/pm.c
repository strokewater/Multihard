#include "types.h"
#include "assert.h"
#include "process.h"
#include "asm/pm.h"
#include "asm/asm.h"
#include "drivers/i8259a/i8259a.h"


static struct Descriptor GDT[NR_GDT_DESCS];
static struct GdtPtr	  	GDTPtr;

static struct Gate 		IDT[NR_IDT_GATES];
static struct IdtPtr	  	IDTPtr;

struct
{
	struct TSS Tss;
	u32 TssIntBitmap[8];
	u8 IOBitmap[128];
	u8 IOEndSign;
} TSSArea;

void SetupIDTGate(int index, u32 selector, addr offset)
{
	assert(index >= 0 && index < NR_IDT_GATES, "SetupIDTGate: index out of range.");
	
	IDT[index].Offset1 = offset & 0xffff;
	IDT[index].Selector = selector;
	IDT[index].ParaCount = 0;
	IDT[index].Reserved = 0;
	IDT[index].Type = DESC_TYPE_SYS_386IGate;
	IDT[index].S = DESC_S_SYS;
	IDT[index].DPL = 3;
	IDT[index].P = 1;
	IDT[index].Offset2 = (offset >> 16) & 0xffff;	
}

void InitUnsetIDTGate();

static void FillGDT()
{
	GDT[0].P = 0;

	// Segment for Flat
	GDT[1].Limit1 = (u32)0xffffffff & 0xffff;
	GDT[1].Base1 = (addr)0 & 0x00ffffff;
	GDT[1].Type = DESC_TYPE_NO_SYS_RW;
	GDT[1].S = DESC_S_DC;
	GDT[1].DPL = 0;
	GDT[1].P = 1;
	GDT[1].Limit2 = ((u32)(0xffffffff) & 0xf0000) >> 16;
	GDT[1].AVL = 0;
	GDT[1].Reserved = 0;
	GDT[1].DB = DESC_D_32;
	GDT[1].G = DESC_G_4K;
	GDT[1].Base2 = (addr)0x0 >> 24;

	// Segment of FlatC
	GDT[2].Limit1 = (u32)0xffffffff & 0xffff;
	GDT[2].Base1 = (addr)0 & 0x00ffffff;
	GDT[2].Type = DESC_TYPE_NO_SYS_C;
	GDT[2].S = DESC_S_DC;
	GDT[2].DPL = 0;
	GDT[2].P = 1;
	GDT[2].Limit2 = ((u32)(0xffffffff) & 0xf0000) >> 16;
	GDT[2].AVL = 0;
	GDT[2].Reserved = 0;
	GDT[2].DB = DESC_D_32;
	GDT[2].G = DESC_G_4K;
	GDT[2].Base2 = (addr)0x0 >> 24;

	// Segment for Flat3
	GDT[3].Limit1 = (u32)0xffffffff & 0xffff;
	GDT[3].Base1 = (addr)0 & 0x00ffffff;
	GDT[3].Type = DESC_TYPE_NO_SYS_RW;
	GDT[3].S = DESC_S_DC;
	GDT[3].DPL = 3;
	GDT[3].P = 1;
	GDT[3].Limit2 = ((u32)(0xffffffff) & 0xf0000) >> 16;
	GDT[3].AVL = 0;
	GDT[3].Reserved = 0;
	GDT[3].DB = DESC_D_32;
	GDT[3].G = DESC_G_4K;
	GDT[3].Base2 = (addr)0x0 >> 24;

	// Segment of FlatC3
	GDT[4].Limit1 = (u32)0xffffffff & 0xffff;
	GDT[4].Base1 = (addr)0 & 0x00ffffff;
	GDT[4].Type = DESC_TYPE_NO_SYS_C;
	GDT[4].S = DESC_S_DC;
	GDT[4].DPL = 3;
	GDT[4].P = 1;
	GDT[4].Limit2 = ((u32)(0xffffffff) & 0xf0000) >> 16;
	GDT[4].AVL = 0;
	GDT[4].Reserved = 0;
	GDT[4].DB = DESC_D_32;
	GDT[4].G = DESC_G_4K;
	GDT[4].Base2 = (addr)0x0 >> 24;

	// Segment for TSS
	GDT[5].Limit1 = (u32)(sizeof(TSSArea) - 1) & 0xffff;
	GDT[5].Base1 = (addr)(&(TSSArea.Tss)) & 0x00ffffff;
	GDT[5].Type = DESC_TYPE_SYS_TSS;
	GDT[5].S = DESC_S_SYS;
	GDT[5].DPL = 0;
	GDT[5].P = 1;
	GDT[5].Limit2 = (u32)(sizeof(TSSArea) - 1) & 0xf0000;
	GDT[5].AVL = 0;
	GDT[5].Reserved = 0;
	GDT[5].DB = DESC_D_32;
	GDT[5].G = 0;
	GDT[5].Base2 = (addr)(&(TSSArea.Tss)) >> 24;
}

void TSSInit()
{
	int i;
	for (i = 0; i < 8; ++i)
		TSSArea.TssIntBitmap[i] = 0xffffffff;
	TSSArea.TssIntBitmap[0] = 0x0;
	for (i = 0; i < 128; ++i)
		TSSArea.IOBitmap[i] = 0;
	TSSArea.IOEndSign = 0xff;
	TSSArea.Tss.IOMap = (void *)&(TSSArea.IOBitmap) - (void *)&(TSSArea.Tss);
	TSSArea.Tss.ESP0 = PROCESS_STK0_TOP - 4;
	TSSArea.Tss.SS0 = SelectorFlat;
}

void PmInit()
{
	// half init, because page will be init later
	TSSInit();
	FillGDT();
	InitUnsetIDTGate();
	Init8259A();
	
	GDTPtr.Base = (addr)&GDT;
	GDTPtr.Limit = sizeof(GDT[0]) * NR_GDT_DESCS - 1;
	IDTPtr.Base = (addr)&IDT;
	IDTPtr.Limit = sizeof(IDT[0]) * NR_IDT_GATES - 1;
	ASMLGDT(&GDTPtr);
	__asm__("ljmp %0,$setup_selector_flat;" \
			"setup_selector_flat:" \
			"mov %%bx, %%ds;" \
			"mov %%bx, %%es;" \
			"mov %%bx, %%ss;" \
			"mov %%bx, %%fs;" \
			"mov %%bx, %%gs;" : :"O"(SelectorFlatC), "b"(SelectorFlat) :);
	ASMLIDT(&IDTPtr);
	
}
