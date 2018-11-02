#include "gfs/ext2/rw.h"
#include "gfs/buffer.h"
#include "gfs/ext2/super.h"
#include "gfs/ext2/block.h"
#include "gfs/gfs.h"
#include "types.h"
#include "string.h"
#include "time.h"
#include "errno.h"
#include "gfs/stat.h"

ssize_t Ext2ReadFile(struct File *fp, void *buf, size_t count)
{
	size_t nchars;
	off_t offset;
	size_t blk_size;
	struct MInode *minode;
	struct BufferHead *bh;
	struct MExt2SuperBlk *msb;
	size_t n;
	off_t blk_no;
	
	// ()count + fp->pos - 1 > inode->size - 1
	
	minode = fp->inode;
	
	if (count  > minode->size - fp->pos)
		count = minode->size - fp->pos;
	if (count <= 0)
		return 0;
	

	msb = Ext2GetSuperBlk(minode->dev);
	LockMInode(minode);
	blk_size = msb->block_size;
	
	if (minode->size == fp->pos)
		return 0;// file was at end;
	if (minode->size < fp->pos)
		return -ERROR;// illegal.
	
	n = 0;
	
	while (count > 0)
	{
		if (minode->size == fp->pos)
			break;
			
		offset = fp->pos % blk_size;
		nchars = blk_size - offset;
		if (nchars > count)
			nchars = count;
			
		blk_no = BMap(minode, fp->pos / blk_size, 0);
		bh = Ext2GetBlock(msb->dev, blk_no);

		memcpy(buf, bh->data + offset, nchars);
		Ext2FreeBlock(bh);
		
		buf = (s8 *)buf + nchars;
		count -= nchars;
		n += nchars;
		fp->pos += nchars;
	}
	
	UnLockMInode(minode);
	
	return n;
}

ssize_t Ext2WriteFile(struct File *fp, void *buf, size_t count)
{
	size_t n;
	size_t nchars;
	off_t offset;
	size_t blk_size;
	off_t blk_no;
	struct MInode *minode;
	struct MExt2SuperBlk *msb;
	struct BufferHead *bh;
	
	minode = fp->inode;
	msb = Ext2GetSuperBlk(minode->dev);
	LockMInode(minode);
	blk_size = msb->block_size;
	
	n = 0;
	while (count > 0)
	{
		offset = fp->pos % blk_size;
		nchars = blk_size - offset;
		if (nchars > count)
			nchars = count;
		
		blk_no = BMap(minode, fp->pos / blk_size, 0);
		if (blk_no == 0)
		{
			blk_no = BMap(minode, fp->pos / blk_size, 1);
			bh = Ext2GetBlock(msb->dev, blk_no);
			memset(bh->data, 0, msb->block_size);
			bh->dirt = 1;
			minode->blocks += 2;
			minode->ctime = CURRENT_TIME;
		}else
			bh = Ext2GetBlock(msb->dev, blk_no);
			
		bh->dirt = 1;
		memcpy(bh->data + offset, buf, nchars);
		
		buf = (s8 *)buf + nchars;
		count -= nchars;
		n += nchars;
		fp->pos += nchars;
		PutBlk(bh);
	}
	
	minode->mtime = CURRENT_TIME;
	if (fp->pos >= minode->size)
	{
		minode->dirt = 1;
		if (S_ISDIR(minode->mode))
			minode->size = ((fp->pos + blk_size - 1) / blk_size) * blk_size;
		else
			minode->size = fp->pos;
		minode->blocks = (minode->size + 512 - 1) / 512;
	}
	
	UnLockMInode(minode);
	
	return n;
}
