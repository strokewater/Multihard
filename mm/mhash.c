#include "mm/mm.h"
#include "mm/mhash.h"
#include "mm/mpgs.h"
#include "assert.h"
#include "stdlib.h"
#include "string.h"
#include "asm/asm.h"

static int Last = -1;

void *MHashPage(void * pa)
{
	int i;
	int left, right;
	addr va;
	PAGE *dest = NULL;
	
	i = Last == -1 ? NMAP_PAGES / 2 : Last;
	for (left = i - 1, right = i + 1; left >= 0 || right < NMAP_PAGES; left--, right++)
	{
		if (left >= 0 && !((MapPages[left]) & PG_P))
		{
			dest = MapPages + left;
			i = left;
			break;
		}
		else if(right < NMAP_PAGES && !((MapPages[right]) & PG_P))
		{
			dest = MapPages + right;
			i = right;
			break;
		}
	}
	
	if (dest == NULL)
		return NULL;
	else
	{
		*dest = (addr)pa | PG_P;
		va = CORE_SPACE_MAP_LOWER + i * PAGE_SIZE;
		ASMInvlPg((void *)va);
		Last = i;
		
		return (void *)va;
	}
}

void UnHashPage(void * va)
{
	int i;
	i = ((addr)va - CORE_SPACE_MAP_LOWER) / PAGE_SIZE;
	// so, it doesn't matter even va was not at PAGE_SIZE bound.(some offset is permitted.).
	
	assert(i >= 0 && i < NMAP_PAGES, "UnHashPage(unfixable): va is out of range.");
	MapPages[i] = 0;
	ASMInvlPg(va);
	Last = i;
	
	Last = i;
}
