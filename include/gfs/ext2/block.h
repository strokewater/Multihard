#ifndef FS_EXT2_BLOCK_H
#define FS_ExT2_BLOCK_H

#define NR_MAX_ISTK_SIZE	20

#include "types.h"
#include "gfs/buffer.h"
#include "gfs/minode.h"
#include "gfs/ext2/ext2.h"
#include "gfs/ext2/super.h"

struct BufferHead *Ext2GetBlock(dev_no dev, off_t blk_no);
void Ext2DestoryBlock(struct BufferHead *bh);
void Ext2FreeBlock(struct BufferHead *p);
struct BufferHead *GetFreeDataBlock(struct MExt2SuperBlk *msb);
int FreeDataBlock(struct MExt2SuperBlk *msb, off_t data_blk_no);
off_t BMap(struct MInode *minode, off_t par_bno, int create);
int BMapInDirect(struct MExt2SuperBlk *msb, off_t blk_no, off_t par_bno, int level, int create);

#endif
