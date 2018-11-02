#ifndef SUPER_H
#define SUPER_H

#include "types.h"
#include "gfs/minode.h"
#include "gfs/buffer.h"


struct MExt2SuperBlk
{
	struct BufferHead *buf;
	struct ext2_super_block *super;
	dev_no dev;
	size_t block_size;
	u8 rd_only;
};

struct MExt2SuperBlk *Ext2GetSuperBlk(dev_no dev);
int Ext2FreeSuperBlk(struct MExt2SuperBlk *sb);

int Ext2Mounted(dev_no dev, struct MInode *mdir, int flags);
int Ext2Mount(struct MInode *mdev, struct MInode *mdir, int fs_type, int flags);
int Ext2UMounted(struct MInode *mdir);
int Ext2UMount(struct MInode *mdir);


#define NR_MAX_SUPERS	10

#endif
