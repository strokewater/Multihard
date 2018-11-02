#include "types.h"
#include "stdlib.h"
#include "mm/varea.h"
#include "gfs/mtable.h"

struct MountItem *MItems[NR_MITEMS];

int AddToMTable(struct MountItem *mi)
{
	int i;
	struct MountItem *p;
	for (i = 1; i < NR_MITEMS; ++i)
	{
		if (MItems[i] == NULL)
		{
			p = KMalloc(sizeof(*p));
			*p = *mi;
			p->mount_dir->count++;
			p->dev_root_dir->count++;
			MItems[i] = p;
			return i;
		}
	}
	return 0;
}

int RemoveFromMTable(int index)
{
	FreeMInode(MItems[index]->mount_dir);
	FreeMInode(MItems[index]->dev_root_dir);
	MItems[index] = NULL;
	return 0;
}

struct MountItem *GetMItemFromMTable(int index)
{
	return MItems[index];
}
