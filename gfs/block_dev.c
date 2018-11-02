#include "types.h"
#include "string.h"
#include "stdlib.h"
#include "printk.h"
#include "gfs/gfs.h"
#include "drivers/drivers.h"
#include "mm/varea.h"

ssize_t GFSReadBlockDev(struct File *fp, void *buf, size_t count)
{
	dev_no dev;
	size_t blk_size;
	struct MinorDevice *md;
	struct BufferHead *bh;
	
	size_t nchars;
	size_t n;
	off_t blk_no, offset;

	
	dev = fp->inode->zone[0];
	md = GetMinorDevice(dev);
	blk_size = md->block_size;
	
	if (md->type != DEV_TYPE_BLOCK)
	{
		debug_printk("Invaild Block Dev READ!!!");
		return -1;
	}
	LockMInode(fp->inode);
	n = 0;
	while (count > 0)
	{
		blk_no = fp->pos / blk_size;
		offset = fp->pos % blk_size;
		nchars = blk_size - offset;
		
		bh = GetBlkLoaded(dev, blk_no);
		
		memcpy(buf, bh->data + offset, nchars);
		PutBlk(bh);
		
		buf = (s8 *)buf + nchars;
		count -= nchars;
		n += nchars;
		fp->pos += nchars;
	}
	
	UnLockMInode(fp->inode);
	return n;
}

ssize_t GFSWriteBlockDev(struct File *fp, void *buf, size_t count)
{

	dev_no dev;
	size_t blk_size;
	struct MinorDevice *md;
	struct BufferHead *bh;
	
	size_t nchars;
	size_t n;
	off_t blk_no, offset;
	
	dev = fp->inode->zone[0];
	md = GetMinorDevice(dev);
	blk_size = md->block_size;
	
	if (md->type != DEV_TYPE_BLOCK)
	{
		debug_printk("Invaild Block Dev WRITE!!!");
		return -1;
	} else
		debug_printk("VAILD BLOCK DEV!");
	
	LockMInode(fp->inode);
	n = 0;
	while (count > 0)
	{
		blk_no = fp->pos / blk_size;
		offset = fp->pos % blk_size;
		nchars = blk_size - offset;
		
		bh = GetBlkLoaded(dev, blk_no);
		if (bh->data == NULL)
		{
			bh->data = KMalloc(blk_size);
			bh->data_size = blk_size;
		}
		memcpy(bh->data + offset, buf, nchars);
		bh->dirt = 1;
		PutBlk(bh);
		
		buf = (s8 *)buf + nchars;
		count -= nchars;
		n += nchars;
		fp->pos += nchars;
	}
	
	UnLockMInode(fp->inode);
	return n;	
}
