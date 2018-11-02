#include "types.h"
#include "gfs/minode.h"
#include "gfs/buffer.h"
#include "gfs/ext2/truncate.h"
#include "gfs/ext2/super.h"
#include "gfs/ext2/block.h"


void FreeIndirectBlocksAtOffset(struct MExt2SuperBlk *msb, off_t direct_blks_no, off_t blk_start_off, int level);
void FreeIndirectBlock(struct MExt2SuperBlk *msb, off_t bno, int level);

int Ext2Truncate(struct MInode *minode, size_t size)
{
	int sized_pbno;
	int n;
	struct MExt2SuperBlk *msb;
	int i;
	int level;
	int grands_no, uncles_no, father_no, kids_no;
	off_t kids_bno;
	
	LockMInode(minode);
	msb = Ext2GetSuperBlk(minode->dev);
	sized_pbno = size / msb->block_size;
	n = msb->block_size / sizeof(off_t);
	
	if (size >= minode->size)
		goto finish;
	
	if (sized_pbno < 12)
	{
		for (level = 1; level <= 3; ++level)
		{
			FreeIndirectBlocksAtOffset(msb, minode->zone[12 + level - 1], 0, level);
			minode->zone[12 + level - 1] = 0;
		}
		if (sized_pbno == 0)
			i = 0;
		else
			i = sized_pbno + 1;
			
		for (i = sized_pbno == 0 ? 0 : sized_pbno + 1; i < 12; ++i)
		{
			FreeDataBlock(msb, minode->zone[i]);
			minode->zone[i] = 0;
		}
	}else if(sized_pbno < 12 + n)
	{
		sized_pbno -= 12;
		if (minode->zone[12] != 0)
			FreeIndirectBlocksAtOffset(msb, minode->zone[12], sized_pbno + 1, 1);
		
		for (level = 2; level <= 3; ++level)
		{
			if (minode->zone[12 + level - 1] != 0)
			{
				FreeIndirectBlocksAtOffset(msb, minode->zone[12 + level - 1], 0, level);
				minode->zone[12 + level - 1] = 0;
			}
		}
	}else if (sized_pbno < 12 + n + n * n)
	{
		sized_pbno -= (12 + n);
		uncles_no = sized_pbno / n;
		kids_no = sized_pbno % n;
		father_no = uncles_no;
		
		FreeIndirectBlocksAtOffset(msb, minode->zone[13], uncles_no + 1, 2);
		kids_bno = BMapInDirect(msb, minode->zone[14], father_no, 1, 0);
		FreeIndirectBlocksAtOffset(msb, kids_bno, kids_no + 1, 0);
	}else
	{
		sized_pbno -= (12 + n + n * n);
		grands_no = sized_pbno / n;
		sized_pbno %= n;
		father_no = sized_pbno / n;
		kids_no = sized_pbno % n;
		
		FreeIndirectBlocksAtOffset(msb, minode->zone[14], grands_no + 1, 3);
		kids_bno = BMapInDirect(msb, minode->zone[14],grands_no * n + father_no, 2, 0);
		FreeIndirectBlocksAtOffset(msb, kids_bno, kids_no + 1, 1);
	}
finish:
	minode->size = size;
	minode->dirt = 1;
	UnLockMInode(minode);
	
	return 0;
}


void FreeIndirectBlocksAtOffset(struct MExt2SuperBlk *msb, off_t direct_blks_no, off_t blk_start_off, int level)
{
	struct BufferHead *bh;
	int n = msb->block_size / sizeof(off_t);
	off_t *lst;
	
	if (direct_blks_no == 0)
		return;
	
	bh = Ext2GetBlock(msb->dev, direct_blks_no);
	lst = (off_t *)bh->data;
	
	for (; blk_start_off < n; ++blk_start_off)
	{
		if (lst[blk_start_off] != 0)
			FreeIndirectBlock(msb, lst[blk_start_off], level - 1);
		lst[blk_start_off] = 0;
	}
	Ext2FreeBlock(bh);		
}

void FreeIndirectBlock(struct MExt2SuperBlk *msb, off_t bno, int level)
{
	struct BufferHead *bh;
	int i;
	int n = msb->block_size / sizeof(off_t);
	off_t *lst;
	
	if (bno == 0)
		return;
	
	if (level == 0)
	{
		FreeDataBlock(msb, bno);
		return;
	}
	
	bh = Ext2GetBlock(msb->dev, bno);
	lst = (off_t *)bh->data;
	for (i = 0; i < n; ++i)
	{
		if (lst[i] != 0)
		{
			if (level == 1)
				FreeDataBlock(msb, lst[i]);
			else
				FreeIndirectBlock(msb, lst[i], level - 1);
		}
		lst[i] = 0;
	}
	Ext2FreeBlock(bh);
}
