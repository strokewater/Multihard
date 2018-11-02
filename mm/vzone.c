#include "string.h"
#include "stdlib.h"
#include "elf.h"
#include "exception.h"
#include "printk.h"
#include "types.h"
#include "asm/asm.h"
#include "mm/vzone.h"
#include "mm/varea.h"
#include "mm/mm.h"
#include "mm/mhash.h"
#include "gfs/gfslevel.h"
#include "gfs/gfs_utils.h"


void WaitOnVZone(struct VZone *vz)
{
	while (vz->lock)
		;
	ASMCli();
	vz->lock = 1;
	ASMSti();
}
void LockOnVZone(struct VZone *vz)
{
	ASMCli();
	vz->lock = 1;
	ASMSti();
}
void UnLockVZone(struct VZone *vz)
{
	ASMCli();
	vz->lock = 0;
	ASMSti();
}

static int GetAttrFromType(int type)
{
	int attr = PG_US;
	
	if (!(type & VZONE_TYPE_READ))
	{
		attr = -1;
		return attr;	// in x86 leagcy page mode, page is all readable.
	}
	if (type & VZONE_TYPE_WRITE)
		attr |= PG_RW;
	// ignore VZONE_TYPE_EXEC, x86 leagcy page mode not support it.
	return attr;
}

struct VZone * AllocVZone(void *vm_start, size_t size, int type,
						  struct MInode *mapped_file, off_t mapped_file_off, size_t mapped_file_size)
{
	struct VZone *ret;
	
	if (size == 0)
		return NULL;
	if ((type & VZONE_TYPE_GROW_DOWN) && (
		mapped_file != VZONE_MAPPED_FILE_ANONYMOUSE_MEM && 
		mapped_file != VZONE_MAPPED_FILE_PHYSICAL_MEM
	))
		return NULL;

	ret = KMalloc(sizeof(*ret));
	ret->lock = 0;
	LockOnVZone(ret);
	
	ret->vm_start = vm_start;
	if (type & VZONE_TYPE_GROW_DOWN)
		ret->vm_end = vm_start - (size - 1);
	else
		ret->vm_end = vm_start + (size - 1);
	ret->size = size;
	ret->origin_size = size;
	ret->type = type;
	
	ret->mapped_file = mapped_file;
	ret->mapped_file_offset = mapped_file_off;
	ret->mapped_size = mapped_file_size;
	ret->mapped_file->count++;
	
	int ngrp = (size + VZONE_PGGRP_SIZE - 1) / VZONE_PGGRP_SIZE;
	ret->mpages = KMalloc(ngrp * sizeof(ret->mpages[0]));
	for (int i = 0; i < ngrp; ++i)
	{
		ret->mpages[i] = KMalloc(sizeof(ret->mpages[0][0]) * VZONE_NR_PAGE_PER_GRP);
		memset(ret->mpages[i], 0, VZONE_NR_PAGE_PER_GRP * sizeof(ret->mpages[0][0]));
	}
	ret->attacher_head = ret->attacher_last = NULL;
	debug_printk("[MM] AllocVZone(vm_start = %x, vm_end = %x, size = %d", ret->vm_start, ret->vm_end, ret->size);
	UnLockVZone(ret);
	return ret;
}

int IsVZoneCross(struct VZone *v1, struct VZone *v2)
{
	addr v1_lower, v1_upper;
	addr v2_lower, v2_upper;
	
	v1_lower = (addr)(v1->type & VZONE_TYPE_GROW_DOWN ? v1->vm_end : v1->vm_start);
	v1_upper = (addr)(v1->type & VZONE_TYPE_GROW_DOWN ? v1->vm_start : v1->vm_end);
	
	v2_lower = (addr)(v2->type & VZONE_TYPE_GROW_DOWN ? v2->vm_end : v2->vm_start);
	v2_upper = (addr)(v2->type & VZONE_TYPE_GROW_DOWN ? v2->vm_start : v2->vm_end);

	return v1_upper < v2_lower || v1_lower > v2_upper ? 0 : 1;
}
void AttachVZone(struct Process *process, struct VZone *vz)
{
	struct VZoneAttcherLst *p;

	void *va;
	int i, j;
	
	for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
	{
		if (IsVZoneCross(process->vzones[i], vz))
			panic("VZone cross.");
	}
	
	LockOnVZone(vz);
	if (!(vz->type & VZONE_TYPE_PROCESS_PUBLIC) && vz->attacher_head != NULL)
		die("Cannot attach another process to vzone writeable.\n");
	p = KMalloc(sizeof(*p));
	if (p == NULL)
		; // THROW(ERR_KMALLOC_FAILED)
	p->attacher = process;
	p->next = NULL;
	if (vz->attacher_last)
		vz->attacher_last->next = p;
	vz->attacher_last = p;
	if (vz->attacher_head == NULL)
		vz->attacher_head = p;
		
	for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
	{
		if (process->vzones[i] == NULL)
		{
			process->vzones[i] = vz;
			break;
		}
	}

	if (process == Current)
	{
		int step;
		if (vz->type & VZONE_TYPE_GROW_DOWN)
		{
			va = (void *)(PAGE_ALIGN((addr)(vz->vm_end)));
			step = -PAGE_SIZE;
		}else
		{
			va = (void *)(PAGE_ALIGN((addr)(vz->vm_start)));
			step = PAGE_SIZE;
		}
		
		int n = ((vz->size + PAGE_SIZE - 1) / PAGE_SIZE + VZONE_NR_PAGE_PER_GRP - 1) / VZONE_NR_PAGE_PER_GRP;
		for (i = 0; i < n ; ++i)
		{
			if (vz->mpages[i] == NULL)
				continue;
			for (j = 0; j < VZONE_NR_PAGE_PER_GRP; ++j)
			{
				ASMInvlPg(va);
				va += step;
			}
		}
	}
	
	UnLockVZone(vz);
}

void GrowVZone(struct VZone *vz, size_t size)
{
	int i, j, k;
	addr tmp_cr3;
	
	LockOnVZone(vz);
	
	void *old_vm_end = vz->vm_end;
	
	if (vz->size + size < vz->origin_size)
		size = vz->size - vz->origin_size;
	
	if (size > 0)
	{
		// cross a group.
		if ((vz->size + VZONE_PGGRP_SIZE - 1 )/ VZONE_PGGRP_SIZE  < 
			(vz->size + size + VZONE_PGGRP_SIZE - 1) / VZONE_PGGRP_SIZE)
		{
			i = vz->size / VZONE_PGGRP_SIZE + 1;
			for (; i <= (vz->size + size) / VZONE_PGGRP_SIZE; ++i)
			{
				vz->mpages[i] = KMalloc(sizeof(vz->mpages[0]));
				memset(vz->mpages[i], 0, sizeof(vz->mpages[0]));
			}
		}
	}else if(size < 0)
	{
		// cross a group
		if (vz->size / VZONE_PGGRP_SIZE  > (vz->size + size) / VZONE_PGGRP_SIZE)
		{
			int n = 0;
			for (i = vz->size / VZONE_PGGRP_SIZE; i > (vz->size + size) / VZONE_PGGRP_SIZE; --i)
			{
				KFree(vz->mpages[i], sizeof(PAGE) * VZONE_NR_PAGE_PER_GRP);
				vz->mpages[i] = NULL;
				
				n += VZONE_PGGRP_SIZE;
			}
			
			vz->size -= n;
			size += n;
		}
		
		j = (vz->size + size) / VZONE_NR_PAGE_PER_GRP;
		for (i = (vz->size + size) / PAGE_SIZE; i <= (vz->size) / PAGE_SIZE; ++i)
		{
			k = i % VZONE_NR_PAGE_PER_GRP;;
			vz->mpages[j][k] = (PAGE)NULL;
		}
		
		void *va_start, *va_end;
		if (vz->type & VZONE_TYPE_GROW_DOWN)
		{
			va_start = (void *)PAGE_ALIGN((addr)(old_vm_end));
			va_end = (void *)PAGE_ALIGN((addr)(vz->vm_end));
		}else
		{
			va_start = (void *)PAGE_ALIGN((addr)(vz->vm_end));
			va_end = (void *)PAGE_ALIGN((addr)(old_vm_end));
		}

		tmp_cr3 = scr3;
		for (struct VZoneAttcherLst *plst = vz->attacher_head; plst; plst = plst->next)
		{
			scr3 = plst->attacher->cr3;
			for (void *va = va_start; va <= va_end; va += PAGE_SIZE)
			{
				PAGE *mpg = GetMPage(va);
				if (*mpg & PG_P)
					FreePhyPage((void *)(PAGE_GET_PA(*mpg)));
				*mpg = 0;
				FreeMPage(mpg);
				
				if (plst->attacher == Current)
					ASMInvlPg(va);
			}
		}
		scr3 = tmp_cr3;
	}
			
	vz->vm_end += size;
	vz->size += size;
	UnLockVZone(vz);
}

void FreeVZone(struct VZone *vz)
{
	struct VZoneAttcherLst *p, *pnext;
	int i;
		
	int ngrp = (vz->size + VZONE_PGGRP_SIZE - 1) / VZONE_PGGRP_SIZE;
	for (i = 0; i < ngrp; ++i)
		KFree(vz->mpages[i], sizeof(PAGE) * VZONE_NR_PAGE_PER_GRP);
	KFree(vz->mpages, sizeof(vz->mpages[0]) * ngrp);
		
	if (vz->mapped_file != VZONE_MAPPED_FILE_ANONYMOUSE_MEM && 
		vz->mapped_file != VZONE_MAPPED_FILE_PHYSICAL_MEM)
		FreeMInode(vz->mapped_file);
	
	for (p = vz->attacher_head; p != NULL; p = pnext)
	{
		pnext = p->next;
		KFree(p, sizeof(*p)); 
	}
}

void DetachVZone(struct Process *process, struct VZone *vz)
{
	void *va;
	void *va_start, *va_end;
	PAGE *mpg;
	struct VZoneAttcherLst *p, *pprev;
	int i;
	
	LockOnVZone(vz);
	
	if (vz->type & VZONE_TYPE_GROW_DOWN)
	{
		va_start = (void *)PAGE_ALIGN((addr)(vz->vm_end));
		va_end = (void *)PAGE_ALIGN((addr)(vz->vm_start));
	}else
	{
		va_start = (void *)PAGE_ALIGN((addr)(vz->vm_start));
		va_end = (void *)PAGE_ALIGN((addr)(vz->vm_end));
	}
	
	for (va = va_start; va <= va_end; va += PAGE_SIZE)
	{
		mpg = GetMPage(va);
		if (*mpg & PG_P)
		{
			FreePhyPage((void *)(PAGE_GET_PA(*mpg)));
			*mpg = 0;
			
			if (vz->type & VZONE_TYPE_PROCESS_PUBLIC)
			{
				int j, k;
				if (vz->type & VZONE_TYPE_GROW_DOWN)
					i = (PAGE_ALIGN((addr)vz->vm_start) - PAGE_ALIGN((addr)va)) / PAGE_SIZE;
				else
					i = ((PAGE_ALIGN((addr)va)) - PAGE_ALIGN((addr)vz->vm_start)) / PAGE_SIZE;
	
				j = i / VZONE_NR_PAGE_PER_GRP;
				k = i % VZONE_NR_PAGE_PER_GRP;
				
				vz->mpages[j][k] = 0;
			}
			
			if (process == Current)
				ASMInvlPg(va);
		}
		FreeMPage(mpg);
	}
	
	for (pprev = NULL, p = vz->attacher_head; p != NULL; pprev = p, p = p->next)
	{
		if (p->attacher == process)
		{
			if (pprev)
			{
				pprev->next = p->next;
				if (vz->attacher_last == p)
					vz->attacher_last = pprev;
				KFree(p, sizeof(*p));
			}else
			{
				KFree(p, sizeof(*p));
				if (vz->attacher_head == vz->attacher_last)
					vz->attacher_last = NULL;
				vz->attacher_head = p->next;
			}
			break;
		}
	}
	
	for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
	{
		if (process->vzones[i] == vz)
		{
			process->vzones[i] = NULL;
			break;
		}
	}
	
	if (vz->attacher_head == NULL)
		FreeVZone(vz);
}

struct VZone *DupVZone(struct VZone *vz)
{
	struct VZone *ret;
	int i, j, va;
	struct VZoneAttcherLst *p;
	
	if (vz->mapped_file == VZONE_MAPPED_FILE_PHYSICAL_MEM)
		return NULL;	// physical memory maybe not allocated by GetPhyPage.
	
	LockOnVZone(vz);
	debug_printk("[MM] DupVZone(type = %d, start = %x, end = %x", vz->type, vz->vm_start, vz->vm_end);
	
	ret = KMalloc(sizeof(*ret));
	*ret = *vz; // lock on ret.
	if (ret->mapped_file != VZONE_MAPPED_FILE_PHYSICAL_MEM &&
		ret->mapped_file != VZONE_MAPPED_FILE_ANONYMOUSE_MEM)
		ret->mapped_file->count++;
	ret->attacher_head = ret->attacher_last = 0;
	
	ret->mpages = KMalloc((vz->size + VZONE_PGGRP_SIZE - 1) / VZONE_PGGRP_SIZE * sizeof(vz->mpages[0]));
	memset(ret->mpages, 0, (vz->size + VZONE_PGGRP_SIZE - 1) / VZONE_PGGRP_SIZE * sizeof(vz->mpages[0]));
	
	int step;
	if (vz->type & VZONE_TYPE_GROW_DOWN)
		step = -PAGE_SIZE;
	else
		step = PAGE_SIZE;
		
	va = PAGE_ALIGN((addr)(vz->vm_start));
	
	for (i = 0; i < (vz->size + VZONE_PGGRP_SIZE - 1) / VZONE_PGGRP_SIZE; ++i)
	{
		ret->mpages[i] = KMalloc(VZONE_NR_PAGE_PER_GRP * sizeof(ret->mpages[0][0]));
		for (j = 0; j < VZONE_NR_PAGE_PER_GRP; ++j)
		{
			if (vz->mpages[i][j] & PG_P)
			{
				if (!(vz->type & VZONE_TYPE_PROCESS_PUBLIC) &&
					vz->type & VZONE_TYPE_WRITE)
				{
					vz->mpages[i][j] &= ~PG_RW;
					ret->mpages[i][j] = vz->mpages[i][j];
					IncPhyCounter((void *)PAGE_GET_PA(vz->mpages[i][j]));
					
					addr tmp_cr3 = scr3;
					for (p = vz->attacher_head; p != NULL; p = p->next)
					{
						scr3 = p->attacher->cr3;
							PAGE *mpg = GetMPage((void *)va);
						if (*mpg & PG_P)
							*mpg = vz->mpages[i][j];
						FreeMPage(mpg);
						if (p->attacher == Current)
							ASMInvlPg((void *)va);
					}
					scr3 = tmp_cr3;
				} else
					ret->mpages[i][j] = vz->mpages[i][j];
			} else
				ret->mpages[i][j] = 0;
			va += step;
		}
	}
	UnLockVZone(vz);
	UnLockVZone(ret);
	return ret;
}

void VZoneUpdateMPage(struct VZone *vz, int j, int k)
{
	struct VZoneAttcherLst *p;
	addr tmp_cr3;
	PAGE *pg;
	void *va;
	
	tmp_cr3 = scr3;
	
	if (vz->type & VZONE_TYPE_GROW_DOWN)
		va = vz->vm_end - (j * VZONE_NR_PAGE_PER_GRP + k) * PAGE_SIZE;
	else
		va = vz->vm_start + (j * VZONE_NR_PAGE_PER_GRP + k) * PAGE_SIZE;
	
	tmp_cr3 = scr3;
	
	for (p = vz->attacher_head; p; p = p->next)
	{
		scr3 = p->attacher->cr3;
		pg = GetMPage(va);
		*pg = vz->mpages[j][k];
		FreeMPage(pg);
	}
	
	scr3 = tmp_cr3;
}

void VZoneSetMPage(struct VZone *vz, void *va, PAGE page)
{
	int i, j, k;
	
	
	if (vz->type & VZONE_TYPE_GROW_DOWN)
		i = (((void *)PAGE_ALIGN((addr)vz->vm_end) - va)) / PAGE_SIZE;
	else
		i = (va - ((void *)PAGE_ALIGN((addr)vz->vm_start))) / PAGE_SIZE;
	
	j = i / VZONE_NR_PAGE_PER_GRP;
	k = i % VZONE_NR_PAGE_PER_GRP;
	
	vz->mpages[j][k] = page;
	VZoneUpdateMPage(vz, j, k);
}

#define IS_VA_IN_VZONE(vz, va)	((vz->type & VZONE_TYPE_GROW_DOWN) ? \
								 ((addr)va >= (addr)((vz)->vm_end) && (addr)va <= (addr)vz->vm_start) : \
								((addr)va >= (addr)((vz)->vm_start) && (addr)va <= (addr)(vz->vm_end)))

void FillEmptyPageToVA(struct VZone *vz, void *va, int j, int k)
{
	int attr;
	void *pa;
	PAGE *mpg;
		
	attr = 0;
	pa = GetPhyPage();
	attr = GetAttrFromType(vz->type);
	attr |= PG_P;
	vz->mpages[j][k] = (s32)pa | attr;
	mpg = GetMPage(va);
	*mpg = (s32)pa | attr;
	ASMInvlPg(va);
	FreeMPage(mpg);
	memset((void *)(PAGE_ALIGN((addr)va)), 0, PAGE_SIZE);
		
	debug_printk("[MM] FillEmptyPage(va = %x, pa = %x)", va, pa);
} 

void DoNoPage(u32 error_code, void *va)
{
	int i;
	struct VZone *vz;
	PAGE *mpg;
	int j, k;
	
	debug_printk("[MM] #PF, Process = %s, PID = %d, address = %x", Current->name, Current->pid, va);
	
	for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
	{
		if (Current->vzones[i] && IS_VA_IN_VZONE(Current->vzones[i], va))
		{
			vz = Current->vzones[i];
			break;
		}
	}
	
	if (i == NR_VZONES_PER_PROCESS)
	{
		printk("DoNoPage not found page in vzones.: %x.\n", (addr)va);
		die("DoNoPage panic.");
	}
		
	if (vz->type & VZONE_TYPE_GROW_DOWN)
		i = (PAGE_ALIGN((addr)vz->vm_start) - PAGE_ALIGN((addr)va)) / PAGE_SIZE;
	else
		i = ((PAGE_ALIGN((addr)va)) - PAGE_ALIGN((addr)vz->vm_start)) / PAGE_SIZE;
	
	j = i / VZONE_NR_PAGE_PER_GRP;
	k = i % VZONE_NR_PAGE_PER_GRP;
	
	// vz->mpages, elf section do not loaded.
	
	if (vz->mpages[j][k] & PG_P)
	{
		mpg = GetMPage(va);
		*mpg = vz->mpages[j][k];
		IncPhyCounter((void *)PAGE_GET_PA(*mpg));
		ASMInvlPg(va);
		FreeMPage(mpg);
		debug_printk("[MM] Get Entry From VZone.mpages (va = %x, page=%x)", va, vz->mpages[j][k]); 
	}else
	{
		if (i > vz->origin_size / PAGE_SIZE)
		{
			// anonymouse memory.
			LockOnVZone(vz);
			FillEmptyPageToVA(vz, va, j, k);
			UnLockVZone(vz);
		} else
		{
			// mapped file
			if (vz->mapped_file == VZONE_MAPPED_FILE_PHYSICAL_MEM)
			{
				LockOnVZone(vz);
				if (vz->type & VZONE_TYPE_GROW_DOWN)
				{
					vz->mpages[j][k] = (PAGE_ALIGN(vz->mapped_file_offset) - i * PAGE_SIZE) | 
											GetAttrFromType(vz->type) | PG_P;
				} else
				{
					vz->mpages[j][k] = (PAGE_ALIGN(vz->mapped_file_offset) + i * PAGE_SIZE) | 
											GetAttrFromType(vz->type) | PG_P;
				}
				mpg = GetMPage(va);
				*mpg = vz->mpages[j][k];
				IncPhyCounter((void *)PAGE_GET_PA(*mpg));
				ASMInvlPg(va);
				FreeMPage(mpg);
				
				UnLockVZone(vz);
			} else if(vz->mapped_file == VZONE_MAPPED_FILE_ANONYMOUSE_MEM)
			{
				LockOnVZone(vz);
				FillEmptyPageToVA(vz, va, j, k);
				UnLockVZone(vz);
			} else
			{
				struct File *fp;
				size_t size;
				
				LockOnVZone(vz);

				// new page for va is ready.
				FillEmptyPageToVA(vz, va, j, k);
				
				fp = GFSOpenFileByMInode(vz->mapped_file, MAY_READ | MAY_EXEC);
				
				if ( (((addr)va) / PAGE_SIZE) != (((addr)vz->vm_start) / PAGE_SIZE) )
					va = (void *)PAGE_ALIGN((addr)va);
				else
					va = vz->vm_start;
					
				fp->pos = vz->mapped_file_offset + va - vz->vm_start;
					
				size = PAGE_ALIGN((addr)va) + PAGE_SIZE - (addr)va;
				if (size > (vz->vm_start + vz->origin_size - va))
					size = vz->vm_start + vz->origin_size - va;
								
				GFSRead(fp, va, size);
				GFSCloseFileByFp(fp);
				
				debug_printk("[MM] Load mapped file (virtual address = %x, limit = %x)", 
								va, 
								size);
				UnLockVZone(vz);
			}
		}
	}
	debug_printk("[MM] #PF, Process = %s, PID = %d, address = %x finish", Current->name, Current->pid, va);
}

void DoWpPage(u32 error_code, void *va)
{
	int i;
	struct VZone *vz;
	PAGE *mpg;
	void *pa;
	int j, k;
	
	debug_printk("[MM] #WP, Process = %s, PID = %d, address = %x", Current->name, Current->pid, va);
	
	for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
	{
		if (Current->vzones[i] && IS_VA_IN_VZONE(Current->vzones[i], va))
		{
			vz = Current->vzones[i];
			break;
		}
	}
	
	if (i == NR_VZONES_PER_PROCESS || !(vz->type & VZONE_TYPE_WRITE))
	{
		printk("\nPage %x do wp page fail.(error code = %d)\n", (addr)va, error_code);
		die("DoWpPage not found page in vzones.");
	}
	if (vz->type & VZONE_TYPE_GROW_DOWN)
		i = (PAGE_ALIGN((addr)vz->vm_start) - PAGE_ALIGN((addr)va)) / PAGE_SIZE;
	else
		i = ((PAGE_ALIGN((addr)va)) - PAGE_ALIGN((addr)vz->vm_start)) / PAGE_SIZE;
	
	j = i / VZONE_NR_PAGE_PER_GRP;
	k = i % VZONE_NR_PAGE_PER_GRP;

	int cnt;
	if (vz->type & VZONE_TYPE_WRITE && !(vz->type & VZONE_TYPE_PROCESS_PUBLIC))
	{
		if ((vz->mpages[j][k] & PG_P) && ((cnt = GetPhyCounter((void *)(PAGE_GET_PA(vz->mpages[j][k])))) > 1))
		{
			pa = GetPhyPage();
			void *va_new_pg = MHashPage(pa);
			memcpy(va_new_pg, (void *)(PAGE_ALIGN((addr)va)), PAGE_SIZE);
			UnHashPage(va_new_pg);
			FreePhyPage((void *)(PAGE_GET_PA(vz->mpages[j][k])));
			vz->mpages[j][k] = (addr)pa | PAGE_GET_FLAG(vz->mpages[j][k]) | PG_RW;
		}else
			vz->mpages[j][k] |= PG_RW; // need not increase PhyCounter
			
		void *tba = (void *)(PAGE_GET_PA(vz->mpages[j][k]));
		debug_printk("[MM] #WP: VA = %x, PA = %x, Counter = %d", va, tba, cnt);
			
		mpg = GetMPage(va);
		*mpg = vz->mpages[j][k];
		ASMInvlPg(va);
		FreeMPage(mpg);
		
	}else
	{
		// error occur, un write able why do write page fault!
		die("Invaild #WP: varea is not copy on write.");
	}
}

int SysBrk(void *address)
{
	int i;
	int data_top_vzone = -1;
	struct VZone *vz;
	debug_printk("[MM] Do SysBrk(%x) start.", (addr)address);
	if (Current->brk_offset_in_vzones == -1)
	{
		for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
		{
			if (Current->vzones[i] == NULL)
				continue;
			if (Current->vzones[i]->type != VZONE_CTYPE_STACK)
			{
				if (data_top_vzone == -1)
					data_top_vzone = i;
				else if(Current->vzones[i]->vm_start > Current->vzones[data_top_vzone]->vm_start)
				{
					data_top_vzone = i;
					continue;
				}
			}
		}
		vz = AllocVZone((void *)(PAGE_ALIGN((addr)Current->vzones[data_top_vzone]->vm_end) + PAGE_SIZE), PAGE_SIZE, VZONE_CTYPE_DATA, 
								VZONE_MAPPED_FILE_ANONYMOUSE_MEM, 0, 0);
		AttachVZone(Current, vz);
		for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
		{
			if (Current->vzones[i]->vm_start == (void *)(PAGE_ALIGN((addr)Current->vzones[data_top_vzone]->vm_end) + PAGE_SIZE))
			{
				Current->brk_offset_in_vzones = i;
				break;
			}
		}
	}
	if (address == NULL)
	{
		debug_printk("[MM] Do SysBrk(%x) finish. heap(%x, %x)", (addr)address, Current->vzones[Current->brk_offset_in_vzones]->vm_start,
																			   Current->vzones[Current->brk_offset_in_vzones]->vm_end);
		return (addr)(Current->vzones[Current->brk_offset_in_vzones]->vm_end) + 1;
	}
	GrowVZone(Current->vzones[Current->brk_offset_in_vzones], address - 1 - Current->vzones[Current->brk_offset_in_vzones]->vm_end);
	debug_printk("[MM] Do SysBrk(%x) finish. heap(%x, %x)", (addr)address, Current->vzones[Current->brk_offset_in_vzones]->vm_start,
																		   Current->vzones[Current->brk_offset_in_vzones]->vm_end);
	return (addr)(Current->vzones[Current->brk_offset_in_vzones]->vm_end) + 1;
}
