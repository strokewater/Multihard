#include "types.h"
#include "elf.h"
#include "string.h"
#include "mm/mm.h"

struct MBInfo
{
	int flags;
	int mem_lower;
	int mem_upper;
	int boot_device;
	char *cmdline;
	int mods_count;
	int mods_addr;
	char syms[16];
	int mmap_length;
	int mmap_addr;
	int drives_length;
	int drives_addr;
	int config_table;
	int boot_loader_name;
	int apm_table;
	int vbe_control_info;
	int vbe_mode_info;
	int vbe_mode;
	int vbe_interface_seg;
	int vbe_interface_off;
	int vbe_interface_len;
};

struct MBModuleInfo
{
	void *start;
	void *end;
	char *name;
	int reserved;
};

#define MODS_COUNT(info)	(*(int *)(info + 20))
#define MODS_ADDR(info)		(*(struct MBModuleInfo **)(info + 24))

void Dead()
{
	while (1);
}

extern PDIR PDIRS[NR_PDIRS_PER_PROCESS];

#define NR_MAX_PAGE_OF_LOADER		8
#define LOADER_LOWER				(CORE_SPACE_DAREA_LOWER_PA)
#define LOADER_UPPER				(LOADER_LOWER + NR_MAX_PAGE_OF_LOADER * PAGE_SIZE - 1)
#define MBINFO_BASE					(LOADER_UPPER + 1)
#define LOADER_DATA_POOL			(MBINFO_BASE + 2 * PAGE_SIZE)
#define LOADER_DATA_POOL_END		CORE_SPACE_DAREA_UPPER_PA

char *pool = (char *)LOADER_DATA_POOL;

void *PoolMalloc(int size, int align)
{
	if ((addr)pool & (align - 1))
		pool = (char *)(((addr)pool & ~(align - 1)) + align);
	void *ret = pool;
	pool += size;
	return ret;
}

void SetMapPage(void *va, void *pa, int attr)
{
	int dir_no = PDIR_NO((addr)va);
	if (PDIRS[dir_no] == 0)
	{
		PDIRS[dir_no] = (addr)PoolMalloc(PAGE_SIZE, PAGE_SIZE) | PG_P | PG_RW;
		memset((void *)PDIRS[dir_no], 0, PAGE_SIZE);
	}
	PAGE *page = (PAGE *)(PAGE_GET_PA((addr)PDIRS[dir_no]));
	page[PAGE_NO((addr)va)] = (addr)pa | attr;
}

struct MBInfo *CopyMBInfo(struct MBInfo *SRCMBInfo)
{
	struct MBInfo *ret = (struct MBInfo *)MBINFO_BASE;
	memcpy(ret, SRCMBInfo, sizeof(*SRCMBInfo));
	
	char *p = SRCMBInfo->cmdline;
	ret->cmdline = pool;
	while (*p)
		*pool++ = *p++;
	*pool++ = 0;
	
	return ret;
}

void Main(struct MBInfo *mbinfo)
{
	int nmods = mbinfo->mods_count;
	if (nmods != 1)
		Dead();
		
	struct MBModuleInfo *module = (struct MBModuleInfo *)(mbinfo->mods_addr);
	memmove((void *)0x200000, module->start, module->end - module->start + 1);
	int start = 0x200000;
	// arrive here, operation about elf has nothing to do wtih module.
	Elf32_Ehdr *head = (Elf32_Ehdr *)start;
	int entry = head->e_entry;
	Elf32_Phdr *phs = PoolMalloc(head->e_phnum * head->e_phentsize, 4);
	
	// later copy Program header may effect program header table
	memcpy(phs, (void *)(start + head->e_phoff), head->e_phnum * head->e_phentsize);	
	
	unsigned int base = ~0;
	int i;
	for (i = 0; i < head->e_phnum; ++i)
	{
		if (phs[i].p_type == PT_LOAD && phs[i].p_vaddr < base)
			base = phs[i].p_vaddr;
	}
	
	void *va;
	for (va = (void *)CORE_SPACE_DAREA_LOWER_PA; (addr)va <= CORE_SPACE_DAREA_UPPER_PA; va += PAGE_SIZE)
	{
		// for loader, and for page man struct itself.
		SetMapPage(va, va, PG_P | PG_RW);
	}
	for (va = (void *)CORE_SPACE_VIDEO_LOWER_PA; (addr)va <= CORE_SPACE_VIDEO_UPPER_PA; va += PAGE_SIZE)
	{
		// video kernel, later kernel may use it.
		SetMapPage(va, va, PG_P | PG_RW);
	}
	for (i = 0; i < head->e_phnum; ++i)
	{
		if (phs[i].p_type != PT_LOAD)
			continue;
		memmove((void *)(CORE_VA_TO_PA(phs[i].p_vaddr)) ,(void *)(start + phs[i].p_offset) ,phs[i].p_filesz);
		memset((void *)(CORE_VA_TO_PA(phs[i].p_vaddr) + phs[i].p_filesz), 0, phs[i].p_memsz - phs[i].p_filesz);	
		
		for (va = (void *)(phs[i].p_vaddr); (addr)va < phs[i].p_vaddr + phs[i].p_memsz; va += PAGE_SIZE)
			SetMapPage(va, (void *)CORE_VA_TO_PA(va), PG_P | PG_RW);
	}
	
	mbinfo = CopyMBInfo(mbinfo);
	__asm__("movl %%eax, %%cr3;" : : "a"((addr)PDIRS): );
	__asm__("movl %%cr0, %%eax;" \
			  "orl $0x80000000, %%eax;" \
			  "movl %%eax, %%cr0;" ::);
	((void(*)(void *))(entry))(mbinfo);
	
	while (1)
	;
}
