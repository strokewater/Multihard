#ifndef FS_MTABLE_H
#define FS_MTABLE_H

#include "types.h"
#include "gfs/minode.h"

#define NR_MITEMS 20

struct MountItem
{
	struct MInode *mount_dir;
	struct MInode *dev_root_dir;
	dev_no dev;
	int flags;
};

// return an index(non-zero).
int AddToMTable(struct MountItem *mi);
int RemoveFromMTable(int index); 
struct MountItem *GetMItemFromMTable(int index);

#endif
