#include "stdlib.h"
#include "string.h"
#include "types.h"
#include "exception.h"
#include "core.h"
#include "sched.h"
#include "printk.h"
#include "asm/pm.h"
#include "asm/traps.h"
#include "asm/asm.h"
#include "mm/mm.h"
#include "mm/mhash.h"
#include "mm/varea.h"
#include "mm/vzone.h"
#include "mm/mpgs.h"


static u16 *MMMan  = (u16 *)(CORE_SPACE_MM_LOWER);
static addr MMBase;
static size_t NMMManed;
static int MMLastPos = 0;

addr scr3;

PDIR KerGlobalPages[NR_PDIRS_PER_PROCESS] __attribute__ ((aligned (PAGE_SIZE)));
void DoWpPage(u32 error_code, void *va);

void PageFault();

void * GetPhyPage()
{
	int left, right;
	left = MMLastPos - 1;
	right = MMLastPos;
	
	while (left >= 0 || right < NMMManed)
	{
		if (left >= 0 && !MMMan[left])
		{
			MMMan[left]++;
			MMLastPos = left;
			return (void *)(MMBase + left * PAGE_SIZE);
		}else if (right >= 0 && !MMMan[right])
		{
			MMMan[right]++;
			MMLastPos = right;
			return (void *)(MMBase + right * PAGE_SIZE);
		}
		left--;
		right++;
	}
	
	panic("No Memory!");
	return NULL;
}

void FreePhyPage(void * pa)
{
	addr offset;
	
	offset = ((addr)pa - MMBase) / PAGE_SIZE;

	if (offset == 139)
		debug_printk("[MM] PID = %d free physical memory %x(count = %d)", Current->pid, pa, MMMan[offset]);

	if (offset < 0 || offset >= NMMManed)
		panic("memory man: physical is out of page manned.");
	if (MMMan[offset])
	{
		MMMan[offset]--;
		MMLastPos = offset;
	}
	else
		die("memory man: free free physical page.");
}

int GetPhyCounter(void *pa)
{
	addr offset;
	
	offset = ((addr)pa - MMBase) / PAGE_SIZE;
	if (offset < 0 || offset >= NMMManed)
		die("memory man: physical is out of page manned.");
	
	return MMMan[offset];
}

void IncPhyCounter(void *pa)
{
	addr offset;
	
	offset = ((addr)pa - MMBase) / PAGE_SIZE;
	if (offset < 0 || offset >= NMMManed)
		die("memory man: physical is out of page manned.");
	if (MMMan[offset])
	{
		MMMan[offset]++;
		MMLastPos = offset;
	}
	else
		die("memory man: free free physical page.");	
}

int IsDynmaicAllocPhy(void *pa)
{
	return (addr)pa >= MMBase;
}

PDIR *GetMPDir(void *va)
{
	PDIR *dir;
	int dir_no;
	
	dir_no = PDIR_NO((addr)va);
	dir = MHashPage((void *)scr3);
	return dir + dir_no;
}

void FreeMPDir(PDIR *dir)
{
	UnHashPage(dir);
}

PAGE *GetMPage(void *va)
{
	void *pa;
	PAGE *page;
	PDIR *dir;
	int page_no;
	
	dir = GetMPDir(va);
	page_no = PAGE_NO(((addr)va));
	if (!(*dir & PG_P))
	{
		pa = GetPhyPage();
		void *tva = MHashPage(pa);
		memset(tva, 0, PAGE_SIZE);
		UnHashPage(tva);
		*dir = (addr)pa | PG_P | PG_US | PG_RW;
	}
	pa = (void *)PDIR_GET_PA(*dir);
	page = MHashPage(pa);
	FreeMPDir(dir);
	return page + page_no;
}

void FreeMPage(PAGE *page)
{
	UnHashPage(page);
}

int GetMPageForGDB(void *va)
{
	int ret;
	PAGE *t;
	t = GetMPage(va);
	ret = *t;
	FreeMPage(t);
	return ret;
}

void SetMPage(void *va, PAGE page)
{
	PAGE *pg;
	
	pg = GetMPage(va);
	*pg = page;
	ASMInvlPg(va);
	FreeMPage(pg);
}

void DebugPage(void *va)
{
	PAGE *page = 0;
	PDIR *dirs = 0;
	int dir_no = PDIR_NO((addr)va);
	int page_no = PAGE_NO(((addr)va));
	
	dirs = MHashPage((void *)scr3);
	
	if (!(dirs[dir_no] & PG_P))
	{
		debug_printk("[MM] physical address not available for linear %x", va);
		goto resource_clean;
	} else
		debug_printk("[MM] PDIR(%x): %x (attr = %x)", va, PDIR_GET_PA(dirs[dir_no]), PDIR_GET_FLAG(dirs[dir_no]));

	
	page = MHashPage((void *)(PDIR_GET_PA(dirs[dir_no])));
	if (!(page[page_no] & PG_P))
	{
		debug_printk("[MM] physical address not available for linear %x", va);
		goto resource_clean;
	} else
		debug_printk("[MM] PAGE(%x): %x (attr = %x)", va, PAGE_GET_PA(page[page_no]), PAGE_GET_FLAG(page[page_no]));
		
resource_clean:
	if (dirs)
		UnHashPage(dirs);
	if (page)
		UnHashPage(page);
}

void VerifyArea(void *va_checked, size_t n)
{
	addr start = PAGE_ALIGN((addr) va_checked);
	addr end = PAGE_ALIGN((addr) va_checked + n - 1);
	addr va;
	PDIR *dirs;
	PAGE *pages;
	
	dirs = MHashPage((void *)scr3);
	if (dirs == NULL)
		die("MHashPage failed.");
	
	for (va = start; va <= end; va += PAGE_SIZE)
	{
		pages = MHashPage((void *)(PDIR_GET_PA(dirs[PDIR_NO(va)])));
		if (!(pages[PAGE_NO(va)] & PG_P))
			DoNoPage(0, (u32 *)va);
		if (!(pages[PAGE_NO(va)] & PG_RW))
			DoWpPage(0, (u32 *)va);
		UnHashPage(pages);
	}
	UnHashPage(dirs);
}


void MMInit()
{
	int t;
	addr va;
	int i;
	
	KerGlobalPages[PDIR_NO(CORE_SPACE_ONE_M_LOWER)] = CORE_VA_TO_PA((addr)OneMPDIR) | PG_P | PG_RW | PG_US;
	for (i = 0; i < NR_PDIRS_STATIC; ++i)
		KerGlobalPages[PDIR_NO(CORE_SPACE_LOWER) + i] = (CORE_VA_TO_PA((addr)KerPDIR[i])) | PG_P | PG_RW | PG_US;
	
	NMMManed = (MemUpper * 1024 - CORE_SPACE_MM_LOWER_PA) / (PAGE_SIZE + sizeof(MMMan[0]));
	if ((NMMManed * sizeof(MMMan[0])) % PAGE_SIZE)
		t = (NMMManed * sizeof(MMMan[0])) / PAGE_SIZE + 1;
	else
		t = (NMMManed * sizeof(MMMan[0])) / PAGE_SIZE;
	MMBase = CORE_SPACE_MM_LOWER_PA + t * PAGE_SIZE;
	
	for (va = CORE_SPACE_ONE_M_LOWER; va <= PAGE_MASK_OFFSET(CORE_SPACE_ONE_M_UPPER); va += PAGE_SIZE)
		OneMPDIR[PAGE_NO(va)] = (addr)va | PG_P | PG_RW | PG_US;
	for (va = CORE_SPACE_CORE_LOWER; va <= PAGE_MASK_OFFSET(CORE_SPACE_CORE_UPPER); va += PAGE_SIZE)
		SET_STATIC_PG(va, ((va - (CORE_SPACE_CORE_LOWER - CORE_SPACE_CORE_LOWER_PA)) | PG_P | PG_RW));
	for (va = CORE_SPACE_MM_LOWER; va <= PAGE_MASK_OFFSET(CORE_SPACE_MM_LOWER + t * PAGE_SIZE); va += PAGE_SIZE)
		SET_STATIC_PG(va, ((va - (CORE_SPACE_MM_LOWER - CORE_SPACE_MM_LOWER_PA)) | PG_P | PG_RW));
	for (va = CORE_SPACE_MAP_LOWER; va <= PAGE_MASK_OFFSET(CORE_SPACE_MAP_UPPER); va += PDIR_SIZE)
		KerGlobalPages[PDIR_NO(va)] = (CORE_VA_TO_PA((addr)MapPages) + (va - CORE_SPACE_MAP_LOWER + 1) / PDIR_SIZE * sizeof(PAGE) * 1024) | PG_P | PG_RW;
	
	__asm__("movl %%eax, %%cr3;" : : "a"(CORE_VA_TO_PA((addr)KerGlobalPages)): );
	scr3 = CORE_VA_TO_PA((addr)KerGlobalPages);
	
	SetupIDTGate(14, SelectorFlatC, (u32)&PageFault);	  
	VAreaInit();
	
	return;
}
