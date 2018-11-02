#include "types.h"
#include "gfs/gfs.h"
#include "drivers/drivers.h"

ssize_t GFSReadCharDev(struct File *fp, void *buf, size_t count)
{
	ssize_t ret;
	struct MinorDevice *md = GetMinorDevice(fp->inode->zone[0]);
	if (md->type != DEV_TYPE_CHAR)
		return -1;
	
	LockMInode(fp->inode);
	ret = DRead(fp->inode->zone[0], buf, 0, count);
	UnLockMInode(fp->inode);
	return ret;
}

ssize_t GFSWriteCharDev(struct File *fp, void *buf, size_t count)
{
	ssize_t ret;
	struct MinorDevice *md = GetMinorDevice(fp->inode->zone[0]);
	if (md->type != DEV_TYPE_CHAR)
		return -1;
	
	LockMInode(fp->inode);
	ret = DWrite(fp->inode->zone[0], buf, 0, count);
	UnLockMInode(fp->inode);
	return ret;
}
