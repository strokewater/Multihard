#ifndef PM_H
#define PM_H

#include "types.h"

#define DESC_D_32		1	
// 32 位段
#define DESC_G_4K		1	
// 段界限粒度为 4K 字节

/*
 存储段描述符类型值说明
*/
#define DESC_S_SYS		0
// 段的类型是系统段，使用DESC_TYPE_SYS_系列子类型

#define DESC_S_DC		1
// 段的类型是数据段或代码段, 使用DESC_TYPE_NO_SYS_系列子类型

#define DESC_TYPE_NO_SYS_R	0	
// 存在的只读数据段类型值

#define DESC_TYPE_NO_SYS_RW	2	
// 存在的可读写数据段属性值

#define DESC_TYPE_NO_SYS_RWA	3	
// 存在的已访问可读写数据段类型值

#define DESC_TYPE_NO_SYS_C	8	
// 存在的只执行代码段属性值

#define DESC_TYPE_NO_SYS_CR	0xA	
// 存在的可执行可读代码段属性值

#define DESC_TYPE_NO_SYS_CCO	0xC	
// 存在的只执行一致代码段属性值

#define DESC_TYPE_NO_SYS_CCOR	0xE
// 存在的可执行可读一致代码段属性值

/* ----------------------------------------------------------------------------
  系统段描述符类型值说明
---------------------------------------------------------------------------- */

#define DESC_TYPE_SYS_LDT		2
// 局部描述符表段类型值

#define DESC_TYPE_SYS_TASK	5	
// 任务门类型值

#define DESC_TYPE_SYS_TSS	 	9
// 可用 386 任务状态段类型值

#define DESC_TYPE_SYS_386CGate	0xC	
// 386 调用门类型值

#define DESC_TYPE_SYS_386IGate	0xE	
// 386 中断门类型值

#define DESC_TYPE_SYS_386TGate	0xF	
// 386 陷阱门类型值


/* 
选择子类型值说明
 其中:
      SA_  : Selector Attribute
*/

#define SA_TIG	0
// 选择子的索引是在GDT中的索引

#define SA_TIL	4
// 选择子的索引是在LDT中的索引

#pragma pack(1)

struct Descriptor
{
	u16 Limit1: 16;
	u32  Base1: 24;
	u8 Type: 4;
	u8 S:1;
	u8 DPL:2;
	u8 P:1;
	u8 Limit2: 4;
	u8 AVL: 1;
	u8 Reserved: 1;		// must be 0
	u8 DB: 1;
	u8 G:1;
	u8 Base2:8;
};

struct Gate
{
	u16 Offset1;
	u16 Selector;
	u16 ParaCount:5;
	u16 Reserved: 3;
	u16 Type:4;
	u16 S:1;
	u16 DPL:2;
	u16 P:1;
	u16 Offset2;
};

struct TSS
{
	u32 PTT;
	u32 ESP0;
	u32 SS0;
	u32 ESP1;
	u32 SS1;
	u32 ESP2;
	u32 SS2;
	u32 CR3;
	u32 EIP;
	u32 EFlags;
	u32 EAX;
	u32 ECX;
	u32 EDX;
	u32 EBX;
	u32 ESP;
	u32 EBP;
	u32 ESI;
	u32 EDI;
	u32 ES;
	u32 CS;
	u32 SS;
	u32 DS;
	u32 FS;
	u32 GS;
	u32 LDTSelector;
	u16 Trap;
	u16 IOMap;
};

struct GdtPtr
{
	u16 Limit;
	u32 Base;
};

struct IdtPtr
{
	u16 Limit;
	u32 Base;
};

#pragma pack()

#define SelectorFlat		(1 << 3)
#define SelectorFlatC		(2 << 3)
#define SelectorFlat3		(3 << 3)
#define SelectorFlatC3		(4 << 3)
#define SelectorTSS			(5 << 3)

#define SA_RPL0			0
#define SA_RPL1			1
#define SA_RPL2			2
#define SA_RPL3			3

#define NR_GDT_DESCS		6
#define NR_IDT_GATES		255	

void SetupIDTGate(int index, u32 selector, addr offset);
void PmInit();
extern struct TSS Tss;

#endif
