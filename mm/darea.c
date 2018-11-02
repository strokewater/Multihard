#include "mm/darea.h"
#include "types.h"
#include "stdlib.h"

static s32 DAreaBitmap[NPAGES_DAREA / (sizeof(s32) * 8)];
static void *DMallocPage(s32 npage);

void *DMalloc(s32 size)
{
	if (size < 0)
		return NULL;
		
	if (size < PAGE_SIZE)
		return DMallocPage(1);
	else
		return DMallocPage((size + PAGE_SIZE - 1) / PAGE_SIZE);
}

void DFree(void *va, s32 size)
{
	s32 npage;
	s32 i;
	s32 j,k;
	
	if (size < PAGE_SIZE)
		npage = 1;
	else
		npage = size % PAGE_SIZE ? size / PAGE_SIZE + 1 : size / PAGE_SIZE;
	
	for (i = ((s32)va - CORE_SPACE_DAREA_LOWER) / PAGE_SIZE; npage > 0; --npage, ++i)
	{
		j = i / (sizeof(s32) * 8);
		k = i % (sizeof(s32) * 8);
		DAreaBitmap[j] &= ~(1 << k);
	}
}

static void *DMallocPage(s32 npage)
{
	s32 i, j, k;
	s32 n;
	s32 itmp;
	
	i = 0;
	while (i < NPAGES_DAREA)
	{
		j = i / (sizeof(s32) * 8);
		k = i % (sizeof(s32) * 8);
		
		if ((DAreaBitmap[j] & (1 << k)) == 0)
		{
			itmp = i;
			for (n = 1, ++i; n < npage; ++n)
			{
				j = i / (sizeof(s32) * 8);
				k = i % (sizeof(s32) * 8);
				if ((DAreaBitmap[j] & (1 << k)) != 0)
					break;
				else
					++i;
			}
			
			if (n == npage)
			{
				for (--i; i >= itmp; --i)
				{
					j = i / (sizeof(s32) * 8);
					k = i % (sizeof(s32) * 8);
					DAreaBitmap[j] |= (1 << k);
				}
				
				return (void *)CORE_SPACE_DAREA_LOWER + itmp * PAGE_SIZE;
			}
		}else
			++i;
	}
	
	return NULL;
}
void *GetDAreaPa(void *va)
{
	int i = ((s32)va - CORE_SPACE_DAREA_LOWER) / PAGE_SIZE;
	return CORE_SPACE_DAREA_LOWER_PA + i * PAGE_SIZE;
}
