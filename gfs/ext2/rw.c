#include "gfs/gfs.h"
#include "gfs/fcntl.h"
#include "gfs/stat.h"
#include "gfs/ext2/rw.h"
#include "types.h"
#include "errno.h"


ssize_t Ext2Read(struct File *fp, void *buf, size_t count)
{
	struct MInode *inode;
	
	inode = fp->inode;
	
	if (S_ISDIR(inode->mode) || S_ISREG(inode->mode))
		return Ext2ReadFile(fp, buf, count);
	return -EINVAL;
}

ssize_t Ext2Write(struct File *fp, void *buf, size_t count)
{
	struct MInode *inode;
	
	inode = fp->inode;
	if (S_ISDIR(inode->mode) || S_ISREG(inode->mode))
		return Ext2WriteFile(fp, buf, count);
	return -EINVAL;
}
