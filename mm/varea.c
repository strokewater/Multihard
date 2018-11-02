#include "string.h"
#include "stdlib.h"
#include "exception.h"
#include "mm/varea.h"
#include "mm/mhash.h"
#include "mm/mm.h"
#include "gfs/gfs.h"
#include "gfs/gfslevel.h"

static struct VAreaAddressDesc *FreeAddress;
static struct VAreaAddressDesc *AddressDescAllocatePool;

static void * AllocAddresses(size_t nr_page)
{
	void *ret;
	struct VAreaAddressDesc *i, *j;
	for (i = FreeAddress, j = NULL; i != NULL; i = i->next, j = i)
	{
		if (i->nr_pg >= nr_page)
		{
			ret = i->va;
			i->nr_pg -= nr_page;
			i->va += nr_page * PAGE_SIZE;
			if (i->nr_pg == 0)
			{
				if (j != NULL)
					j->next = i->next;
				else
					FreeAddress = i->next;

				i->next = AddressDescAllocatePool;
				AddressDescAllocatePool = i;
			}
			return ret;
		}
	}
	
	return NULL;
}

static void FreeAddresses(void * ad, size_t nr_page)
{
	struct VAreaAddressDesc *i, *j;
	struct VAreaAddressDesc *dest;
	for (i = FreeAddress, j = NULL; i != NULL && (addr)ad > (addr)i->va; i = i->next, j = i)
		;
	
	dest = AddressDescAllocatePool;
	AddressDescAllocatePool = AddressDescAllocatePool->next;
	dest->va = ad;
	dest->nr_pg = nr_page;
	dest->next = i;
	
	if (j)
	{
		if (j->va + j->nr_pg * PAGE_SIZE == dest->va)
		{
			j->nr_pg += nr_page;
			dest->next = AddressDescAllocatePool;
			AddressDescAllocatePool = dest;

			dest = j;
		}else
		{
			j->next = dest;
			dest->next = i;
		}
	}

	if (i)
	{
		if (dest->va + dest->nr_pg * PAGE_SIZE == i->va)
		{
			dest->nr_pg += i->nr_pg;
			dest->next = i->next;

			i->next = AddressDescAllocatePool;
			AddressDescAllocatePool = i;
			if (FreeAddress == i)
				FreeAddress = dest;
		}
	}
}

static void * LMems[VAREA_LMEM_MOST_PAGES_AT_ONCE];
// so, lmem a time, you can alloc VAREA_LMEM_MOST_PAGES_AT_ONCE * PAGE_SIZE at most

void *AllocLMems(size_t npage)
{
	int i;
	void *va;
	void *ret;
	
	for (i = 0; i < npage; ++i)
	{
		va = GetPhyPage();
		if (va)
			LMems[i] = va;
		else
			break;
	}
	
	if (i == npage)
	{
		ret = AllocAddresses(npage);
		for (i = 0; i < npage; ++i)
			SetMPage(ret + i * PAGE_SIZE, (s32)LMems[i] | PG_P | PG_RW);
		return ret;
	}else
	{
		for (--i; i >= 0; i--)
			FreePhyPage(LMems[i]);
		return NULL;
	}
	
}

void FreeVPages(void *va, size_t npage)
{
	PAGE *pg;
	while (npage--)
	{
		pg = GetMPage(va);
		FreePhyPage((void *)(PAGE_GET_PA(*pg)));
		*pg = 0;
		FreeMPage(pg);
		va += PAGE_SIZE;
	}
}

void FreeLMems(void * va, size_t npage)
{
	FreeVPages(va, npage);
	FreeAddresses(va, npage);
}

static void * MallocFreeAddress[NR_MALLOC_FREE_ADDRESS] = {NULL};
static struct BucketDir BucketDirs[] = {
	{16, NULL},
	{32, NULL},
	{64, NULL},
	{128, NULL},
	{256, NULL},
	{512, NULL},
	{1024, NULL},
	{2048, NULL},
	{4096, NULL},
	{0, NULL}
};
static struct BucketDesc *FreeBucketDesc = NULL;

static void *GetMallocFreeAddress(void)
{
	int i;
	void *va;
	for (i = 0; i < NR_MALLOC_FREE_ADDRESS && \
				!MallocFreeAddress[i]; ++i)
		;
	if (i == NR_MALLOC_FREE_ADDRESS)
	{
		// no free
		va = AllocAddresses(NR_MALLOC_FREE_ADDRESS);
		if (va == NULL)
			die("No free address for malloc.");
		for (i = 0; i < NR_MALLOC_FREE_ADDRESS; ++i)
			MallocFreeAddress[i] = (void *)((int)va + i * PAGE_SIZE);
		i = 0;
	}
	va = MallocFreeAddress[i];
	MallocFreeAddress[i] = NULL;
	return va;
}
static void FreeMallocFreeAddress(void * ad)
{
	int i;
	for (i = 0; i < NR_MALLOC_FREE_ADDRESS && MallocFreeAddress[i]; ++i)
		;
	if (i == NR_MALLOC_FREE_ADDRESS)
		FreeAddresses(ad, 1);
	else
		MallocFreeAddress[i] = ad;
}

static struct BucketDesc *GetFreeDesc()
{
	struct BucketDesc *ret, *i;
	void * pa;
	u32 n;
	if (FreeBucketDesc == NULL)
	{
		FreeBucketDesc = AllocAddresses(1);
		pa = GetPhyPage();
		if (!pa)
			die("malloc: no memory for get_free_desc.");
		SetMPage(FreeBucketDesc, (s32)pa | PG_P | PG_RW);
		
		for (i = FreeBucketDesc, n = PAGE_SIZE/sizeof(struct BucketDesc)
					; n > 1; 
					n--, i++)
			i->next = i + 1;
		i->next = NULL;
	}
	ret = FreeBucketDesc;
	FreeBucketDesc = FreeBucketDesc->next;
	ret->next = NULL;
	return ret;
}

static void FreeDesc(struct BucketDesc *desc)
{
	desc->next = FreeBucketDesc;
	FreeBucketDesc = desc;
}

void *KMalloc(size_t len)
{
	size_t npage;
	void *ret;
	struct BucketDir *bdir;
	struct BucketDesc *bdesc;
	char *cp;
	int i;
	void *va, *pa;
	
	for (bdir = BucketDirs; bdir->size; bdir++)
	{
		if (bdir->size >= len)
			break;
	}
		;
	if (bdir->size == 0)
	{
		if (len % PAGE_SIZE)
			npage = len / PAGE_SIZE + 1;
		else
			npage = len / PAGE_SIZE;
		ret = AllocLMems(npage);
		return ret;
	}else
	{
		for (bdesc = bdir->chain; bdesc && !bdesc->freeptr; bdesc = bdesc->next)
			;
		if (bdesc == NULL)
		{
			bdesc = GetFreeDesc();
			
			if (bdesc == NULL)
				return NULL;
			
			bdesc->bucket_size = bdir->size;
			bdesc->next = NULL;
			bdesc->refcnt = 0;
			
			bdesc->page = bdesc->freeptr = va = GetMallocFreeAddress();
			if (va == NULL)
				die("malloc: no more memory space in malloc.");
			pa = GetPhyPage();
			if (pa == NULL)
				die("malloc: out of memory in malloc.");
			SetMPage(va, (s32)pa | PG_P | PG_RW);
			cp = va;
			
			// second error occur here..
			*cp = 0;
			for (i = 0; i < PAGE_SIZE / bdesc->bucket_size - 1; i++)
			{
				*((char **)cp) = cp + bdir->size;
				cp += bdir->size;
			}
			*((char **)cp) = NULL;
			bdesc->next = bdir->chain;
			bdir->chain = bdesc;	
		}
		ret = bdesc->freeptr;
		bdesc->freeptr = *(void * *)bdesc->freeptr;
		bdesc->refcnt++;
		
		return ret;
	}
}

void KFree(void * obj, size_t size)
{
	void *page = (void *)(PAGE_MASK_OFFSET((addr)obj));
	struct BucketDesc *bdesc = NULL, *bprev;
	struct BucketDir *bdir;
	int founded = 0;
	
	if (!IS_VAREA_MEM(obj))
	{
		debug_printk("[MM] KFree Pass address %x, size = %x", obj, size);
		die("kfree: Bad address passed to free.");
	}
	
	for (bdir = BucketDirs; bdir->size;bdir++)
	{
		if (bdir->size >= size)
		{
			founded = 0;
			for (bdesc = bdir->chain, bprev = NULL; bdesc; bprev = bdesc, bdesc = bdesc->next)
			{
				if (bdesc->page == page)
				{
					founded = 1;
					break;
				}
			}
			if (founded)
				break;
		}
	}
	
	if (founded)
	{
		// found
		memset(obj, 0, bdesc->bucket_size);
		*((void * *)obj) = bdesc->freeptr;
		bdesc->freeptr = obj;
		bdesc->refcnt--;
		
		if (bdesc->refcnt == 0)
		{
			if (bprev)
				bprev->next = bdesc->next;
			else
				bdir->chain = bdesc->next;
			FreeVPages(bdesc->page, 1);
			FreeMallocFreeAddress(bdesc->page);
			FreeDesc(bdesc);
		}
	}else
	{
		if (size == 0)
			panic("Free LMEM must require size!");
		FreeLMems(obj, (size + PAGE_SIZE - 1) / PAGE_SIZE);
	}
	
}

void VAreaInit(void)
{
	addr va;
	struct VAreaAddressDesc *i;
	
	for (va = VAREA_ADDRESS_MAN_LOWER; va <= VAREA_ADDRESS_MAN_UPPER; va += PAGE_SIZE)
		SetMPage((void *)va, (s32)GetPhyPage() | PG_P | PG_RW | PG_US);
	
	FreeAddress = (struct VAreaAddressDesc *) VAREA_ADDRESS_MAN_LOWER;
	AddressDescAllocatePool = (struct VAreaAddressDesc *) (VAREA_ADDRESS_MAN_LOWER) + 1;
	
	FreeAddress->va = (void *)VAREA_ADDRESSED_LOWER;
	FreeAddress->nr_pg = (VAREA_ADDRESSED_UPPER - VAREA_ADDRESSED_LOWER + 1) / PAGE_SIZE;
	FreeAddress->next = NULL;
	
	for (i = AddressDescAllocatePool; (addr)i + sizeof(*i) - 1 <= VAREA_ADDRESS_MAN_UPPER; i++)
		i->next = i + 1;
	(--i)->next = NULL;
}
