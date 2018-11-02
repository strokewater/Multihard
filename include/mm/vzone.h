#ifndef MM_VZONE_H
#define MM_VZONE_H

#include "types.h"
#include "sched.h"
#include "elf.h"
#include "mm/mm.h"

#define VZONE_CTYPE_TEXT	(VZONE_TYPE_READ | VZONE_TYPE_EXEC | VZONE_TYPE_PROCESS_PUBLIC)
#define VZONE_CTYPE_RODATA	(VZONE_TYPE_READ | VZONE_TYPE_PROCESS_PUBLIC)
#define VZONE_CTYPE_DATA	(VZONE_TYPE_READ | VZONE_TYPE_WRITE)
#define VZONE_CTYPE_STACK	(VZONE_CTYPE_DATA | VZONE_TYPE_GROW_DOWN)

#define VZONE_TYPE_UNKNOWN				(-1)

#define VZONE_TYPE_READ					1
#define VZONE_TYPE_WRITE				2
#define VZONE_TYPE_EXEC					4

// opposite it is "VZONE_TYPE_PROCESS_PRIVATE", but this macro is not defined.
// type & VZONE_TYPE_PROCESS_PRIVATE should be !(type & VZONE_TYPE_PROCESS_PUBLIC)
#define VZONE_TYPE_PROCESS_PUBLIC		8

// opposite it is "VZONE_TYPE_GROW_UP", but this macro is not defined.			
#define VZONE_TYPE_GROW_DOWN			16


#define VZONE_MAPPED_FILE_PHYSICAL_MEM		((struct MInode *)0)
#define VZONE_MAPPED_FILE_ANONYMOUSE_MEM	((struct MInode *)1)


#define VZONE_NR_PAGE_PER_GRP	1024
#define VZONE_PGGRP_SIZE		(1024 * PAGE_SIZE)

struct VZoneAttcherLst
{
	struct Process *attacher;
	struct VZoneAttcherLst *next;
};

struct VZone
{
	void *vm_start;		// mpages对应的第一个虚拟地址
	void *vm_end;		// mpages对应的最后一个虚拟地址
	size_t origin_size;	// 初始大小，GrowVzone调整后的size不能小于该值
	size_t size;		// 当前大小

	int lock;			// 互斥锁

	int type;
	PAGE **mpages;		// 对应页表结构

	struct MInode *mapped_file;
	off_t mapped_file_offset;
	off_t mapped_size;

	struct VZoneAttcherLst *attacher_head;
	struct VZoneAttcherLst *attacher_last;
};

/*
 * VZONE 当size为0时mpages未零，表示未从设备中的可执行文件加载进入内存。
 */

struct VZone * AllocVZone(void *vm_start, size_t size, int type,
						  struct MInode *mapped_file, off_t mapped_file_off, size_t mapped_file_size);
void AttachVZone(struct Process *process, struct VZone *vz);
void GrowVZone(struct VZone *vz, size_t size);
void FreeVZone(struct VZone *vz);
void DetachVZone(struct Process *process, struct VZone *vz);
struct VZone *DupVZone(struct VZone *vz);

void DoNoPage(u32 error_code, void *va);
void DoWpPage(u32 error_code, void *va);

int SysBrk(void *addr);


#endif
