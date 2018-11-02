#include "assert.h"
#include "stdlib.h"
#include "string.h"
#include "printk.h"
#include "mm/varea.h"
#include "drivers/drivers.h"
#include "gfs/buffer.h"
#include "gfs/ext2/block.h"
#include "gfs/ext2/ext2.h"


struct BufferHead *Ext2GetBlock(dev_no dev, off_t blk_no)
{
	return GetBlkLoaded(dev, blk_no);
}

void Ext2FreeBlock(struct BufferHead *bh)
{
	PutBlk(bh);
}

void Ext2DestoryBlock(struct BufferHead *bh)
{
	int ret;
	size_t n;
	struct MinorDevice *minor;
		
	minor = GetMinorDevice(bh->dev_no);
	
	n = bh->data_size / minor->block_size;
	
	if (bh->uptodate)
	{
		ret = DWrite(bh->dev_no, bh->data, bh->block_no * n, n);
		assert(ret == n, "Ext2FreeBlock: Dwrite buffer failed.");
	}
	
	KFree(bh->data, bh->data_size);
	bh->data = NULL;
	bh->data_size = 0;
	bh->dev_no = 0;
	
	PutBlk(bh);
}

struct BufferHead *GetFreeDataBlock(struct MExt2SuperBlk *msb)
{
	struct ext2_group_desc *gd;
	struct BufferHead *gd_bh, *bitmap_bh;
	int i;
	int k, l;
	off_t bitmap_block_no;
	off_t ret_blk_no = 0;
	struct BufferHead *bh_ret;
	s8 *bitmap;
	
	debug_printk("[FS] GetFreeDataBlock, device = %d", msb->dev);
	
	LockBuffer(msb->buf);
	int n_gds = (msb->super->s_blocks_count - msb->super->s_first_data_block - 1) 
			/ msb->super->s_blocks_per_group + 1;
	int num_gd_per_blk = msb->block_size / sizeof(struct ext2_group_desc);

	int gds_blkno = msb->super->s_first_data_block + 1;
	int free_blk_gd_no = -1;
	int blk_idx_of_gds = 0;
	
	for (int cnt_gds = 0; cnt_gds < n_gds; cnt_gds += num_gd_per_blk)
	{
		gd_bh = Ext2GetBlock(msb->dev, gds_blkno);
	
		gd = (struct ext2_group_desc *)gd_bh->data;
		for (i = 0; i < n_gds && gd[i].bg_free_blocks_count == 0; ++i)
			;
		if (i != n_gds)
			break;

		Ext2FreeBlock(gd_bh);
		blk_idx_of_gds++;
		gds_blkno++;
	}
	
	if (i == n_gds)
	{
		UnLockBuffer(msb->buf);
		Ext2FreeBlock(gd_bh);
		return 0;
	}

	free_blk_gd_no = i + blk_idx_of_gds * n_gds;
	
	bitmap_block_no = gd[i].bg_block_bitmap;
	
	bitmap_bh = Ext2GetBlock(msb->dev, bitmap_block_no);
	bitmap = bitmap_bh->data;
	for (k = 0; k < msb->block_size; ++k)
	{
		if (bitmap[k] == 0xff)
			continue;
		for (l = 0; l < sizeof(s8) * 8 && bitmap[k] & (1 << l); ++l)
			;
		if (l < sizeof(s8) * 8)
		{
			bitmap[k] |= (1 << l);
			bitmap_bh->dirt = 1;
			msb->super->s_free_blocks_count--;
			msb->buf->dirt = 1;
			gd[i].bg_free_blocks_count--;
			gd_bh->dirt = 1;
			ret_blk_no = free_blk_gd_no * msb->super->s_blocks_per_group + k * 8 + l + msb->super->s_first_data_block;
			break;
		}
	}
	if (ret_blk_no == 0)
		bh_ret = NULL;
	else
		bh_ret = GetBlkLoaded(msb->dev, ret_blk_no);
		
	Ext2FreeBlock(gd_bh);
	Ext2FreeBlock(bitmap_bh);
	UnLockBuffer(msb->buf);
	return bh_ret;
}

int FreeDataBlock(struct MExt2SuperBlk *msb, off_t data_blk_no)
{
	struct ext2_group_desc *gd;
	struct BufferHead *gd_bh, *bitmap_bh;
	s8 *bitmap;
	int i, j, k, l;
	off_t blk_no;
	
	LockBuffer(msb->buf);

	int num_gd_per_blk = msb->block_size / sizeof(struct ext2_group_desc);
	
	data_blk_no -= msb->super->s_first_data_block;
	i = data_blk_no / msb->super->s_blocks_per_group;
	j = data_blk_no % msb->super->s_blocks_per_group;
	k = j / 8;
	l = j % 8;
	
	gd_bh = Ext2GetBlock(msb->dev, msb->super->s_first_data_block + 1 + i / num_gd_per_blk);
	gd = (struct ext2_group_desc *)gd_bh->data;
	blk_no = gd[i].bg_block_bitmap;
	
	bitmap_bh = Ext2GetBlock(msb->dev, blk_no);
	bitmap = bitmap_bh->data;
	
	bitmap[k] &= ~(1 << l);
	gd[i % num_gd_per_blk].bg_free_blocks_count++;
	msb->super->s_free_blocks_count++;
	
	bitmap_bh->dirt = 1;
	gd_bh->dirt = 1;
	msb->buf->dirt = 1;
	
	Ext2FreeBlock(gd_bh);
	Ext2FreeBlock(bitmap_bh);
	UnLockBuffer(msb->buf);
	return 0;
}


int BMapInDirect(struct MExt2SuperBlk *msb, off_t blk_no, off_t par_bno, int level, int create)
{
	struct BufferHead *bh, *bhtmp;
	int n = msb->block_size / sizeof(off_t);
	off_t *off_lst;
	off_t ret;
	int i;
	
	bh = Ext2GetBlock(msb->dev, blk_no);
	off_lst = (off_t *)bh->data;
	if (level == 1)
	{
		i = par_bno;
		if (off_lst[i] == 0 && create)
		{
			bhtmp = GetFreeDataBlock(msb);
			LockBuffer(bh);
			off_lst[i] = bhtmp->block_no;
			bh->dirt = 1;
			UnLockBuffer(bh);
			Ext2FreeBlock(bhtmp);
		} else if(off_lst[i] == 0 && create == 0)
		{
			Ext2FreeBlock(bh);
			return 0;
		}
		ret = off_lst[i];
	} else
	{
		i = par_bno / n;
		if (off_lst[i] == 0 && create)
		{
			bhtmp = GetFreeDataBlock(msb);
			LockBuffer(bh);
			off_lst[i] = bhtmp->block_no;
			bh->dirt = 1;
			UnLockBuffer(bh);
		} else if(off_lst[i] == 0 && create == 0)
		{
			Ext2FreeBlock(bh);
			return 0;
		}	
		ret = BMapInDirect(msb, off_lst[i], par_bno % n, level - 1, create);
	}

	Ext2FreeBlock(bh);
	return ret;
}

off_t BMap(struct MInode *minode, off_t par_bno, int create)
{
	struct MExt2SuperBlk *msb;
	off_t ret;
	struct BufferHead *bh;
	int n;
	
	msb = Ext2GetSuperBlk(minode->dev);
	n = msb->block_size / sizeof(off_t);
	
	if (par_bno < 12)
	{
		ret = minode->zone[par_bno];
		if (ret == 0 && create)
		{
			bh = GetFreeDataBlock(msb);
			minode->zone[par_bno] = bh->block_no;
			PutBlk(bh);
			return minode->zone[par_bno];
		} else
			return ret;
	}else if(par_bno < 12 + n)
	{
		if (minode->zone[12] == 0)
		{
			if (create)
			{
				bh = GetFreeDataBlock(msb);
				minode->zone[12] = bh->block_no;
				minode->dirt = 1;
				PutBlk(bh);
			} else
				return 0;
		}
		return BMapInDirect(msb, minode->zone[12], par_bno - 12, 1, create);
	}else if(par_bno < 12 + n + n * n)
	{
		if (minode->zone[13] == 0)
		{
			if (create)
			{
				bh = GetFreeDataBlock(msb);
				minode->zone[13] = bh->block_no;
				minode->dirt = 1;
				PutBlk(bh);
			} else
				return 0;
		}
		return BMapInDirect(msb, minode->zone[13], par_bno - (12 + n), 2, create);
	}
	else if(par_bno < 12 + n + n * n + n * n * n)
	{
		if (minode->zone[14] == 0)
		{
			if (create)
			{
				bh = GetFreeDataBlock(msb);
				minode->zone[14] = bh->block_no;
				minode->dirt = 1;
				PutBlk(bh);
			} else
				return 0;
		}
		return BMapInDirect(msb, minode->zone[14], par_bno - (12 + n + n * n), 3, create);
	}

	panic("BMap reache here error.");
	return -1;
}

