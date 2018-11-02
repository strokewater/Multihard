#ifndef FS_EXT2_TRUNC
#define FS_EXT2_TRUNC

#include "types.h"
#include "gfs/minode.h"

int Ext2Truncate(struct MInode *minode, size_t size);

#endif
