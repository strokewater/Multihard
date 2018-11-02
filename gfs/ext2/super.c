#include "stdlib.h"
#include "printk.h"
#include "mm/varea.h"
#include "drivers/drivers.h"
#include "gfs/buffer.h"
#include "gfs/gfs.h"
#include "gfs/gfslevel.h"
#include "gfs/mtable.h"
#include "gfs/ext2/super.h"
#include "gfs/ext2/ext2.h"
#include "gfs/ext2/block.h"


struct MExt2SuperBlk Supers[NR_MAX_SUPERS];

struct MExt2SuperBlk *Ext2GetSuperBlk(dev_no dev)
{
	int i;
	for (i = 0; i < NR_MAX_SUPERS; ++i)
	{
		if (Supers[i].buf->dev_no == dev)
		{
			WaitOnBuffer(Supers[i].buf);
			return Supers + i;
		}
	}
	
	return NULL;
}

int Ext2Mounted(dev_no dev, struct MInode *mdir, int flags)
{
	struct ext2_super_block *super;
	size_t blk_size;
	struct BufferHead *bh_super;
	off_t offset;
	int i;
	struct MinorDevice *md;
	struct MountItem mi;
	
	md = GetMinorDevice(dev);
	super = KMalloc(sizeof(*super));
	if (!super)
		return 0;
			
	DRead(dev, super, 2, 1);
	if (super->s_magic != EXT2_SUPER_MAGIC)
		return 0;
	

	blk_size = 1 << (super->s_log_block_size + 10);
	md->fs_blk_size = blk_size;
	if (blk_size == 1024)
	{
		bh_super = Ext2GetBlock(dev, 1);
		offset = 0;
	}
	else
	{
		bh_super = Ext2GetBlock(dev, 0);	// (Ext2FreeBlock in umounted.)
		offset = 1024;
	}
	
	if (bh_super == NULL)
		return 0;
		
	for (i = 0; i < NR_MAX_SUPERS; ++i)
		if (Supers[i].buf == NULL)
			break;
	
	if (i == NR_MAX_SUPERS)
		return 0;
	
	bh_super->count++;
	Ext2FreeBlock(bh_super);
	Supers[i].buf = bh_super;
	Supers[i].super = (struct ext2_super_block *)((char *)(bh_super->data) + offset);
	Supers[i].dev = dev;
	Supers[i].rd_only = flags & MF_RDONLY;
	Supers[i].block_size = blk_size;
	
	md->fs_type = Ext2FSTypeNo;
	md->root = GetMInode(dev, EXT2_ROOT_INO);
	
	mi.mount_dir = mdir;
	mi.dev_root_dir = md->root;
	mi.flags = flags;
	mi.dev = dev;

	LockMInode(mdir);
	mdir->mount = md->root->mount = AddToMTable(&mi);
	mdir->dirt = 1;
	UnLockMInode(mdir);
	
	debug_printk("[FS] SysMount: mdir(%x) count = %d", mdir, mdir->count);
	
	return 1;
}

int Ext2Mount(struct MInode *mdev, struct MInode *mdir, int fs_type, int flags)
{
	dev_no mounted_dev_no;
	int ret;
	
	mounted_dev_no = mdev->zone[0];
	
	mdir->dirt = 1;
	ret = GFSMounted(mounted_dev_no, mdir, flags, fs_type);
	if (ret == 0)
		return 0;
	else
		return 1;
}

int Ext2UMounted(struct MInode *mdir)
{
	struct MExt2SuperBlk *sb;
	struct MinorDevice *md;
	struct MountItem *mi;
	
	mi = GetMItemFromMTable(mdir->mount);
	
	SysSync(mi->dev);	// sync all, including super block and data block.
									// next free super block.
	
	if (IsMInodeOfDevVaildExist(mi->dev))
		// some minodes of this device whose count > 0 are still exist.
		return 0;	// an error.
	
	sb = Ext2GetSuperBlk(mi->dev);
	sb->buf->count--;
	Ext2DestoryBlock(sb->buf);
	sb->buf = NULL;
	sb->super = NULL;
	sb->rd_only = 0;
	sb->dev = 0;
	
	md = GetMinorDevice(mi->dev);
	md->fs_type = FS_TYPE_RAW;
	md->fs_blk_size = 0;
	md->root->mount = 0;
	FreeMInode(md->root);
	
	return 1;
	
}

int Ext2UMount(struct MInode *mdir)
{
	GFSUMounted(mdir);
	RemoveFromMTable(mdir->mount);
	mdir->mount = 0;
	
	return 1;
}
