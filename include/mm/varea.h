#ifndef VAREA_H
#define VAREA_H

#include "mm/mm.h"
#include "types.h"

#define NR_PGS_VAREA_ADD_MAN		4
#define NR_PGS_VAREA_MALLOC			4000

#define VAREA_ADDRESS_MAN_LOWER		CORE_SPACE_VAREA_LOWER
#define VAREA_ADDRESS_MAN_UPPER		(CORE_SPACE_VAREA_LOWER + NR_PGS_VAREA_ADD_MAN * PAGE_SIZE - 1)
#define VAREA_MALLOC_ZONE_LOWER		VAREA_ADDRESS_MAN_UPPER + 1
#define VAREA_MALLOC_ZONE_UPPER		(VAREA_MALLOC_ZONE_LOWER + NR_PGS_VAREA_MALLOC * PAGE_SIZE - 1)
#define VAREA_LONG_ZONE_LOWER		VAREA_MALLOC_ZONE_UPPER + 1
#define VAREA_LONG_ZONE_UPPER		CORE_SPACE_VAREA_UPPER

#define VAREA_ADDRESSED_LOWER		VAREA_MALLOC_ZONE_LOWER
#define VAREA_ADDRESSED_UPPER		VAREA_LONG_ZONE_UPPER

#define VAREA_LMEM_MOST_PAGES_AT_ONCE	4096

#define NR_MALLOC_FREE_ADDRESS	10

#define IS_VAREA_MEM(va)	(((addr)va) >= VAREA_MALLOC_ZONE_LOWER && ((addr)va) <= VAREA_LONG_ZONE_UPPER)

struct VAreaAddressDesc
{
	void *va;
	size_t nr_pg;
	struct VAreaAddressDesc *next;
};

struct BucketDesc {
	void *page;
	void *freeptr;
	int refcnt;
	u16 bucket_size;
	struct BucketDesc *next;
};

struct BucketDir
{
	size_t size;
	struct BucketDesc *chain;
};

void VAreaInit();
void *KMalloc(size_t len);
void KFree(void * obj, size_t size);

#endif
