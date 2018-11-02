#ifndef MM_H
#define MM_H

#include "types.h"

/* ---------------------------------------------------------------------------------------------------------------------------------------------
 * | reserved | device mem | reserved | core | reserved | mm man | reserved | map pages |      varea	       |
 * ---------------------------------------------------------------------------------------------------------------------------------------------
 */

#define CORE_SPACE_ONE_M_LOWER		0x0
#define CORE_SPACE_ONE_M_UPPER		0xfffff
#define CORE_SPACE_DAREA_LOWER		0x8000
#define CORE_SPACE_DAREA_UPPER		0x9FC00
#define CORE_SPACE_VIDEO_LOWER		0xB8000
#define CORE_SPACE_VIDEO_UPPER		0xC7fff
#define CORE_SPACE_CORE_LOWER		0xc0100000
#define CORE_SPACE_CORE_UPPER		0xc01fffff
#define CORE_SPACE_MM_LOWER			0xc0200000
#define CORE_SPACE_MM_UPPER			0xc03fffff
#define CORE_SPACE_MAP_LOWER		0xc0400000
#define CORE_SPACE_MAP_UPPER		0xc0bfffff
#define CORE_SPACE_VAREA_LOWER		0xc0c02000
#define CORE_SPACE_VAREA_UPPER		0xc1ffffff
#define CORE_SPACE_STK0_LOWER		0xc6000000
#define CORE_SPACE_STK0_UPPER		0xc6001fff

#define CORE_SPACE_ONE_M_LOWER_PA	0x0
#define CORE_SPACE_ONE_M_UPPER_PA	0xfffff
#define CORE_SPACE_DAREA_LOWER_PA	0x8000
#define CORE_SPACE_DAREA_UPPER_PA	0x9FC00
#define CORE_SPACE_VIDEO_LOWER_PA	0xB8000
#define CORE_SPACE_VIDEO_UPPER_PA	0xC7FFF
#define CORE_SPACE_CORE_LOWER_PA	0x100000
#define CORE_SPACE_CORE_UPPER_PA	0x1fffff
#define CORE_SPACE_MM_LOWER_PA		0x200000
#define CORE_SPACE_MM_UPPER_PA		0x3fffff

#define CORE_VA_TO_PA(va)			(((addr)va) - CORE_SPACE_LOWER)

#define USER_SPACE_LOWER			0x0
#define USER_SPACE_UPPER			0xbfffffff
#define CORE_SPACE_LOWER			0xc0000000
#define CORE_SPACE_UPPER			0xffbfffff

// for int type virtual address overflow.

#define CORE_TO_USER_DIFF			(CORE_SPACE_LOWER - USER_SPACE_LOWER)

#define NR_PDIRS_PER_PROCESS		1024
#define NR_PDIRS_STATIC				8
#define NR_MAPS_PER_DIR				2
#define NR_PAGES_PER_DIR			1024

typedef int PDIR;
typedef int PAGE;

#define PAGE_SIZE				4096
#define PDIR_SIZE				0x400000
#define PAGE_NO(va)				(((va) >> 12) & 0x3ff)
#define PDIR_NO(va)				((va) >> 22)
#define PAGE_GET_PA(pg) 		((pg) & ~0xfff)
#define PDIR_GET_PA(pdir)		((pdir) & ~0xfff)
#define PAGE_ALIGN(pg)			((pg) & ~0xfff)

#define PAGE_ATTR_MASK			0xfff
#define PDIR_ATTR_MASK			0xfff

#define PAGE_GET_FLAG(pg)		((pg) & PAGE_ATTR_MASK)
#define PAGE_MASK_OFFSET(va)	((va) & ~PAGE_ATTR_MASK)
#define PDIR_GET_FLAG(pd)		((pd) & PDIR_ATTR_MASK)
#define PDIR_MASK_OFFSET(pd)	(	(pd) & ~PDIR_ATTR_MASK)

#define PAGE_GET_OFFSET(va)		((va) & PAGE_ATTR_MASK)

#define PG_P						1
#define PG_US						4
#define PG_RW						2
#define PG_A						32

// the first page and the last page is forbidden.
// for short value
#define SET_STATIC_PG(va, pg) (KerPDIR[PDIR_NO((va)) - PDIR_NO(CORE_SPACE_LOWER)][PAGE_NO(((va)))] = (pg))

extern PDIR KerGlobalPages[NR_PDIRS_PER_PROCESS];
extern PAGE KerPDIR[NR_PDIRS_STATIC][NR_PAGES_PER_DIR];
extern PAGE OneMPDIR[NR_PAGES_PER_DIR];
void MMInit();

void *GetPhyPage();
void FreePhyPage(void *pa);
void IncPhyCounter(void *pa);
int GetPhyCounter(void *pa);
int IsDynmaicAllocPhy(void *pa);

PAGE *GetMPage(void *pa);
void FreeMPage(PAGE *va);
PDIR *GetMPDir(void *pa);
void FreeMPDir(PDIR *va);

void SetMPage(void *va, PAGE page);

void VerifyArea(void *va, size_t n);
void DebugPage(void *va);

extern addr scr3;

#endif
