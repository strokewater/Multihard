#include "sched.h"
#include "errno.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include "printk.h"
#include "mm/varea.h"
#include "drivers/drivers.h"
#include "gfs/minode.h"
#include "gfs/gfs_utils.h"
#include "gfs/gfslevel.h"
#include "gfs/gfs.h"
#include "gfs/stat.h"
#include "gfs/fcntl.h"
#include "gfs/ext2/open.h"
#include "gfs/ext2/truncate.h"
#include "gfs/ext2/super.h"
#include "gfs/ext2/ext2.h"
#include "gfs/ext2/inode.h"
#include "gfs/ext2/dir.h"

int Ext2Open(struct MInode *minode, int flags, mode_t mode)
{
	struct File *fp;
	int fd;
	
	if ((flags & O_DIRECTORY) && !(S_ISDIR(minode->mode)))
		return -1;
	
	mode &= 0777 & ~Current->umask;
	
	/* follwing, file will be installed in FS record and Process filep. */
	fp = GetFreeFileRec();
	if (fp == NULL)
		return -ENFILE;
	
	for (fd = 0; fd < NR_OPEN; ++fd)
	{
		if (Current->filep[fd] == NULL)
		{
			Current->filep[fd] = fp;
			break;
		}
	}
	
	if (fd == NR_OPEN)
		return -ENFILE;
	
	if (S_ISCHR(minode->mode) | S_ISBLK(minode->mode))
	{
		int dopen_ret = DOpen(minode->zone[0], fp, flags);
		if (dopen_ret != 0)
		{
			Current->filep[fd] = NULL;
			FreeFileRec(fp);
			return dopen_ret;
		}
	}
	
	Current->close_on_exec &= ~(1 << fd);
	fp->count = 1;
	fp->mode = mode;
	fp->flags = flags;
	fp->inode = minode;
	minode->count++;
	fp->pos = 0;
	
	if ((flags & O_WRONLY || flags & O_RDWR) && flags & O_TRUNC)
		Ext2Truncate(minode, 0);
	if (flags & O_APPEND)
		fp->pos = fp->inode->size == 0;

	return fd;
}

int Ext2Close(int fd)
{
	struct File *fp;
	
	fp = Current->filep[fd];
	if (S_ISCHR(fp->inode->mode) | S_ISBLK(fp->inode->mode))
		DClose(fp->inode->zone[0], fp);
	
	fp->count--;
	
	if (fp->count > 0)
		return 0;
	else
	{
		if (fp->inode->pipe)
		{
			fp->inode->count--;
			if (fp->inode->count == 0)
			{
				KFree((char *)fp->inode->size, 0);
				KFree(fp->inode, 0);
			}
		}else
			FreeMInode(fp->inode);
	}
	
	return 0;
}

struct MInode *Ext2Creat(struct MInode *root, const char *name, mode_t mode)
{
	char *tmp_name = NULL;
	int name_len;
	struct MInode *parent_dir = NULL;
	struct MInode *mret = NULL;
	char *filename = NULL;
	int i;
		
	name_len = strlen(name);
	tmp_name = KMalloc(name_len + 1);
	if (tmp_name == NULL)
	{
		Current->errno = -ENOMEM;
		goto resource_clean;
	}
	strcpy(tmp_name, name);
	for (i = name_len - 1; i >= 0; --i)
	{
		if (tmp_name[i] == '/')
			break;
	}
	if (i < 0)
	{
		parent_dir = root;
		parent_dir->count++;	// 下面FreeMInode时统一操作, 原本别名引用的count不变,下同
		filename = tmp_name;
	}else if (i == 0)
	{
		parent_dir = root;
		parent_dir->count++;	// 同上
		filename = tmp_name + 1;
	}else
	{
		tmp_name[i] = '\0';
		parent_dir = GFSReadMInode(root, tmp_name);
		filename = tmp_name + i + 1;
	}
	
	mret = GFSReadMInode(parent_dir, filename);
	if (mret == NULL)
	{
		struct MExt2SuperBlk *msb = Ext2GetSuperBlk(parent_dir->dev);
		int blk_size = msb->block_size;
		mret = GetFreeInode(msb);
		if (mret < 0)
			goto resource_clean;
			
		mret->nlinks = 1;
		mret->dtime = 0;
		mret->size = 0;
		mret->atime = CURRENT_TIME;
		mret->ctime = CURRENT_TIME;
		mret->blocks = blk_size / 512;
		mode = (mode & (~Current->umask)) & 0777;
		mret->mode = S_IFREG | mode;
		mret->dirt = 1;
		mret->uid = Current->euid;
		mret->gid = Current->egid;
		mret->flags = 0;
		mret->extend_info = KMalloc(sizeof(struct Ext2InodeExInfo));
		struct Ext2InodeExInfo *ex_info = mret->extend_info;
		ex_info->generation = 0;
		ex_info->file_acl = 0;
		ex_info->dir_acl = 0;
		ex_info->faddr = 0;
		for (i = 0; i < EXT2_N_BLOCKS; ++i)
			mret->zone[i] = 0;
			
		Current->errno = Ext2Link(parent_dir, mret, filename);
		if (Current->errno < 0)
			goto resource_clean;
	}
	
resource_clean:
	if (tmp_name != NULL)
		KFree(tmp_name, 0);
	if (parent_dir != NULL)
		FreeMInode(parent_dir);
	return mret;
}

int Ext2MkNod(struct MInode *root, const char *filename, mode_t mode, dev_no dev)
{
	struct MInode *mi;
	mi = Ext2Creat(root, filename, mode);
			
	if (mi == NULL)
		return Current->errno;
	mi->zone[0] = dev;
	mi->mode = mode;
	mi->mtime = mi->atime = CURRENT_TIME;
	mi->dirt = 1;
	FreeMInode(mi);
	return 0; 
}
