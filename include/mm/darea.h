#ifndef MM_DAREA_H
#define MM_DAREA_H

#include "mm/mm.h"
#include "types.h"
#include "stdlib.h"

#define NPAGES_DAREA		((CORE_SPACE_DAREA_UPPER + 1 - CORE_SPACE_DAREA_LOWER) / PAGE_SIZE)

void *DMalloc(s32 size);
void DFree(void *pa, s32 size);
void *GetDAreaPa(void *va);

#endif
