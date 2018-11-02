#include "types.h"
#include "stdlib.h"
#include "assert.h"
#include "string.h"
#include "errno.h"
#include "mm/varea.h"
#include "drivers/drivers.h"
#include "gfs/buffer.h"
#include "gfs/minode.h"
#include "gfs/gfs.h"
#include "gfs/gfslevel.h"
#include "gfs/fcntl.h"
#include "gfs/mtable.h"
#include "gfs/gfs_utils.h"
#include "gfs/ext2/inode.h"
#include "gfs/ext2/block.h"
#include "gfs/ext2/super.h"
#include "gfs/ext2/bitmap.h"
#include "gfs/ext2/ext2.h"
#include "gfs/ext2/dir.h"


struct MInode *GetFreeInode(struct MExt2SuperBlk *msb)
{
	struct BufferHead *gd_bh, *bitmap_bh;
	struct ext2_group_desc *gd;
	int i, j;
	ino_t reti;
	struct MInode *mret;
	
	LockBuffer(msb->buf);

	int n_gds = (msb->super->s_blocks_count - msb->super->s_first_data_block - 1) 
			/ msb->super->s_blocks_per_group + 1;
	int num_gd_per_blk = msb->block_size / sizeof(struct ext2_group_desc);

	int gds_blkno = msb->super->s_first_data_block + 1;
	int free_inode_gd_no = -1;
	int blk_idx_of_gds = 0;
	
	for (int cnt_gds = 0; cnt_gds < n_gds; cnt_gds += num_gd_per_blk)
	{
		gd_bh = Ext2GetBlock(msb->dev, gds_blkno);
	
		gd = (struct ext2_group_desc *)gd_bh->data;
		for (i = 0; i < n_gds && gd[i].bg_free_inodes_count == 0; ++i)
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

	free_inode_gd_no = i + blk_idx_of_gds * n_gds;
	
	bitmap_bh = Ext2GetBlock(msb->dev, gd[i].bg_inode_bitmap);
	j = GetFreeBit(bitmap_bh->data,  msb->block_size / sizeof(s8));
	assert(j != -1, "Unexception situation. no free inode.");
	bitmap_bh->dirt = 1;
	
	// ret = EXT2_ROOT_INO + i * msb->super->s_inodes_per_group + j;
	reti = free_inode_gd_no * msb->super->s_inodes_per_group + j + 1;	// +1 >> +1
	
	gd[i].bg_free_inodes_count--;
	gd_bh->dirt = 1;
	msb->super->s_free_inodes_count--;
	msb->buf->dirt = 1;
	Ext2FreeBlock(gd_bh);
	Ext2FreeBlock(bitmap_bh);
	
	UnLockBuffer(msb->buf);
	mret = GetMInode(msb->dev, reti);
	return mret;
}

int FreeInode(struct MExt2SuperBlk *msb, struct MInode *minode)
{
	struct BufferHead *gd_bh, *bitmap_bh;
	struct ext2_group_desc *gd;
	int i, j;
	ino_t inode = minode->inum;
	
	LockBuffer(msb->buf);
	LockMInode(minode);

	int num_gd_per_blk = msb->block_size / sizeof(struct ext2_group_desc);
	
	i = (inode - 1) / msb->super->s_inodes_per_group;
	j = (inode - 1) % msb->super->s_inodes_per_group;
	
	gd_bh = Ext2GetBlock(msb->dev, msb->super->s_first_data_block + 1 + i / num_gd_per_blk);
	gd = (struct ext2_group_desc *)gd_bh->data;
	
	bitmap_bh = Ext2GetBlock(msb->dev, gd[i % num_gd_per_blk].bg_inode_bitmap + i);
	FreeBit(bitmap_bh->data,  j);
	bitmap_bh->dirt = 1;
	
	gd[i % num_gd_per_blk].bg_free_inodes_count++;
	gd_bh->dirt = 1;
	msb->super->s_free_inodes_count++;
	msb->buf->dirt = 1;
	Ext2FreeBlock(gd_bh);
	Ext2FreeBlock(bitmap_bh);
	
	UnLockBuffer(msb->buf);
	UnLockMInode(minode);
	return 1;	
}

struct MInode *Ext2ReadMInode(struct MInode *root, const char *path)
{
	const char *end;
	struct File *prev_dir_fp, *dir_fp;
	struct MInode *dir_inode, *ret;
	struct ext2_dir_entry *dir_entry;
	
	while (*path == '/')
		++path;
		
	if (*path == '\0')
		return root;
		
	prev_dir_fp = GFSOpenFileByMInode(root, MAY_EXEC | MAY_READ);
	dir_entry = KMalloc(sizeof(*dir_entry));
	
	while (*path != '\0')
	{
		for (end = path; *end != '\0' && *end != '/'; ++end)
			;
		--end;
		
		// this dir: [path, end]
		dir_inode = NULL;
		
		while (Ext2GetDirEntry(prev_dir_fp, dir_entry) == 0)
		{
			int n = end - path + 1;
			if (n == dir_entry->name_len && strncmp(dir_entry->name, path, n) == 0)
			{
				dir_inode = GetMInode(prev_dir_fp->inode->dev, dir_entry->inode);
				break;
			}
		}
		
		if (dir_inode == NULL)
		{
			GFSCloseFileByFp(prev_dir_fp);
			Current->errno = -ENOENT;
			return NULL;
		}
		dir_fp = GFSOpenFileByMInode(dir_inode, MAY_EXEC | MAY_READ);

		if (dir_inode->fs_type != Ext2FSTypeNo)
		{
			ret = GFSReadMInode(GFSGetRootMInode(dir_inode->mount), end + 1);
			if (prev_dir_fp)
				GFSCloseFileByFp(prev_dir_fp);
			if (dir_fp)
				GFSCloseFileByFp(dir_fp);
			if (dir_entry)
				KFree(dir_entry, 0);
			return ret;
		}
		
		GFSCloseFileByFp(prev_dir_fp);
		prev_dir_fp = dir_fp;
		
		path = end + 1;
		while (*path == '/')
			++path;
	}
	
	if (prev_dir_fp)
		GFSCloseFileByFp(prev_dir_fp);
	KFree(dir_entry, sizeof(*dir_entry));
	return dir_inode;
}

static int Ext2HDoMInode(struct MInode *p, int action)
{
	// Ext2HDoMInode.action, 0: Read Inode in disk to MInode in memory.
	//								 1:  Write MInode in memory to Inode in disk.
	
	int i, j, k, l;
	struct MExt2SuperBlk *msb;
	struct BufferHead *gd_bh, *inode_bh;
	struct ext2_group_desc *gd;
	off_t blk_no;
	struct ext2_inode *inode;
	struct Ext2InodeExInfo *ieinfo;
	
	msb = Ext2GetSuperBlk(p->dev);
	LockMInode(p);
	
	i = (p->inum - 1) / msb->super->s_inodes_per_group;
	j = (p->inum - 1) % msb->super->s_inodes_per_group;
	k = j / (msb->block_size / msb->super->s_inode_size);
	l = j % (msb->block_size / msb->super->s_inode_size);

	int num_gd_per_blk = msb->block_size / sizeof(struct ext2_group_desc);
	gd_bh = Ext2GetBlock(msb->dev, msb->super->s_first_data_block + 1 + i / num_gd_per_blk);
	gd = (struct ext2_group_desc *)gd_bh->data + i % num_gd_per_blk;
	blk_no = gd->bg_inode_table + k;
	inode_bh = Ext2GetBlock(msb->dev, blk_no);
	inode = (struct ext2_inode *)(inode_bh->data + l * msb->super->s_inode_size);
	
	if (action == 0)
	{
		p->mode = inode->i_mode;
		p->uid = inode->i_uid;
		p->size = inode->i_size;
		p->atime = inode->i_atime;
		p->ctime = inode->i_ctime;
		p->mtime = inode->i_mtime;
		p->dtime = inode->i_dtime;
		p->gid = inode->i_gid;
		p->nlinks = inode->i_links_count;
		p->blocks = inode->i_blocks;
		p->flags = inode->i_flags;
		p->extend_info = KMalloc(sizeof(struct Ext2InodeExInfo));
		ieinfo = p->extend_info;
		ieinfo->generation = inode->i_generation;
		ieinfo->file_acl = inode->i_file_acl;
		ieinfo->dir_acl = inode->i_dir_acl;
		ieinfo->faddr = inode->i_faddr;
		memcpy(p->zone, inode->i_block, EXT2_N_BLOCKS * sizeof(inode->i_block[0]));
		p->update = 1;
	}else
	{
		inode->i_mode = p->mode;
		inode->i_uid = p->uid;
		inode->i_size = p->size;
		inode->i_atime = p->atime;
		inode->i_ctime = p->ctime;
		inode->i_mtime = p->mtime;
		inode->i_dtime = p->dtime;
		inode->i_gid = p->gid;
		inode->i_links_count = p->nlinks;
		inode->i_blocks = p->blocks;
		inode->i_flags = p->flags;
		ieinfo = p->extend_info;
		inode->i_generation = ieinfo->generation;
		inode->i_file_acl = ieinfo->file_acl;
		inode->i_dir_acl = ieinfo->dir_acl;
		inode->i_faddr = ieinfo->faddr;
		memcpy(inode->i_block, p->zone, EXT2_N_BLOCKS * sizeof(inode->i_block[0]));
		inode_bh->dirt = 1;
	}
	
	Ext2FreeBlock(inode_bh);
	Ext2FreeBlock(gd_bh);
	UnLockMInode(p);
	return 0;
}

int Ext2HReadMInode(struct MInode *p)
{
	return Ext2HDoMInode(p, 0);
}

int Ext2HWriteMInode(struct MInode *p)
{
	return Ext2HDoMInode(p, 1);
}

void Ext2CleanMInodeExInfo(struct MInode *p)
{
	KFree(p->extend_info, 0);
}

struct MInode *Ext2GetMInode(struct MInode *p)
{
	struct MountItem *mi;
	struct MInode *ret;
	if (p->mount)
	{
		mi = GetMItemFromMTable(p->mount);
		FreeMInode(p);
		ret = mi->dev_root_dir;
		ret->count++;
		// 这个比较特殊，看起来好像违反了别名引用的count不变的规则，
		// 但是，p的count增加过，为了模仿p，ret的count也要增加
		return ret;
	}else
		return p;
}
