#include "types.h"
#include "sched.h"
#include "assert.h"
#include "errno.h"
#include "stdlib.h"
#include "gfs/gfs.h"
#include "gfs/gfslevel.h"
#include "gfs/minode.h"
#include "gfs/gfs_utils.h"
#include "gfs/rw.h"
#include "gfs/pipe.h"
#include "gfs/fcntl.h"


struct MInode *GFSReadMInode(struct MInode *root, const char *path)
{
	assert(root, "FSReadMInode: root == NULL.");
	assert(path, "FSReadMInode: path == NULL.");
	assert(root->fs_type >= 0  && root->fs_type < NR_FS_TYPES, "FSReadMInode: fs_type error.");
	
	return FileSystem[root->fs_type]->GFSReadMInode(root, path);
}

struct MInode *GFSGetMInode(struct MInode *p)
{
	return FileSystem[p->fs_type]->GFSGetMInode(p);
}

int GFSHReadMInode(struct MInode *p)
{
	assert(p, "FSReadMInode: p == NULL.");
	assert(p->fs_type >= 0  && p->fs_type < NR_FS_TYPES, "FSReadMInode: fs_type error.");
	
	return FileSystem[p->fs_type]->GFSHReadMInode(p);
}
int GFSHWriteMInode(struct MInode *p)
{
	assert(p, "FSReadMInode: root == NULL.");
	assert(p->fs_type >= 0  && p->fs_type < NR_FS_TYPES, "FSReadMInode: fs_type error.");
	
	return FileSystem[p->fs_type]->GFSHWriteMInode(p);
}

void GFSCleanMInodeExInfo(struct MInode *p)
{
	FileSystem[p->fs_type]->GFSCleanMInodeExInfo(p);
}

ssize_t GFSRead(struct File *fp, void *buf, size_t count)
{
	assert(fp, "FSRead: fp == NULL");
	assert(buf, "FSRead: buf == NULL");
	assert(count >= 0,  "FSRead: count < 0");

	struct MInode *inode = fp->inode;
	if (inode->pipe)
		return fp->flags == O_RDONLY ? GFSReadPipe(fp, buf, count) : 0;
	if (S_ISCHR(inode->mode))
		return GFSReadCharDev(fp, buf, count);
	if (S_ISBLK(inode->mode))
		return GFSReadBlockDev(fp, buf, count);
	
	return FileSystem[fp->inode->fs_type]->GFSRead(fp, buf, count);
}

ssize_t GFSWrite(struct File *fp, void *buf, size_t count)
{
	assert(fp, "FSWrite: fp == NULL");
	assert(buf, "FSWrite: buf == NULL");
	assert(count >= 0, "FSWrite: count < 0");

	struct MInode *inode;
	
	inode = fp->inode;
	if (inode->pipe)
		return fp->flags == O_WRONLY ? GFSWritePipe(fp, buf, count) : 0;
	if (S_ISCHR(inode->mode))
		return GFSWriteCharDev(fp, buf, count);
	if (S_ISBLK(inode->mode))
		return GFSWriteBlockDev(fp, buf, count);
	
	return FileSystem[fp->inode->fs_type]->GFSWrite(fp, buf, count);
}

struct MInode *GFSCreat(struct MInode *root, const char *name, mode_t mode)
{
	assert(root, "FSCreat: root == NULL.");
	assert(name, "FSCReat: name == NULL,");
	
	return FileSystem[root->fs_type]->GFSCreat(root, name, mode);
}


int GFSMounted(s32 dev, struct MInode *mdir, int flags, int dev_fs_type)
{
	return FileSystem[dev_fs_type]->GFSMounted(dev, mdir, flags);
}

int GFSUMount(struct MInode *mdir)
{
	assert(mdir, "FSUMount: mdir == NULL");
	assert(mdir->dev >= 0, "FSUMount: dev no of mdir is invaild.");
	
	return FileSystem[mdir->fs_type]->GFSUMount(mdir);
}

int GFSUMounted(struct MInode *mdir)
{
	assert(mdir, "FSUMount: mdir == NULL");
	assert(mdir->dev >= 0, "FSUMount: dev no of mdir is invaild.");
	
	return FileSystem[mdir->fs_type]->GFSUMounted(mdir);	
}


int GFSChMod(struct MInode *minode, mode_t mode)
{
	if (Current->euid == minode->uid || Current->euid == 0)
	{
		LockMInode(minode);
		minode->mode = (mode & 07777) | (minode->mode & ~0777);
		minode->dirt = 1;
		UnLockMInode(minode);
		return 0;
	}else
		return -EACCES;
}

int GFSChOwn(struct MInode *minode, uid_t uid, gid_t gid)
{
	if (Current->euid != 0)
		return -EACCES;
	LockMInode(minode);
	minode->uid = uid;
	minode->gid = gid;
	minode->dirt = 1;
	UnLockMInode(minode);
	
	return 0;
}

int GFSAccess(struct MInode *minode, int mode)
{
	if (GFSPermission(minode, mode))
		return 0;
	else
		return Current->errno;
}

int GFSStat(struct MInode *minode, struct stat *buf)
{
	buf->st_dev = minode->dev;
	buf->st_ino = minode->inum;
	buf->st_mode = minode->mode;
	buf->st_nlink = minode->nlinks;
	buf->st_uid = minode->uid;
	buf->st_gid = minode->gid;
	buf->st_rdev = minode->zone[0];
	buf->st_size = minode->size;
	buf->st_atime = minode->atime;
	buf->st_ctime = minode->ctime;
	buf->st_mtime = minode->mtime;
	
	return 0;
}

int GFSMkNod(struct MInode *parent, const char *filename, mode_t mode, dev_no dev)
{
	assert(parent, "FSMkNod: parent == NULL.");
	assert(filename, "FSMkNod: filename == NULL.");
	
	return FileSystem[parent->fs_type]->GFSMkNod(parent, filename, mode, dev);
}


int GFSMkDir(struct MInode *parent, const char *filename, int mode)
{

	assert(parent, "FSMkDir: parent == NULL.");
	assert(filename, "FSMkDir: filename == NULL.");

	return FileSystem[parent->fs_type]->GFSMkDir(parent, filename, mode);
}


int GFSRmDir(struct MInode *parent, const char *dirname)
{

	assert(parent, "FSRmDir: parent == NULL.");
	assert(dirname, "FSRmDir: filename == NULL.");

	return FileSystem[parent->fs_type]->GFSRmDir(parent, dirname);
}

int GFSReadDir(struct File *dir_fp, struct dirent *dirp, int cnt)
{
	assert(dir_fp, "GFSReadDir: dir_fp == NULL.");
	assert(dirp, "GFSReadDir: dirp == NULL");
	assert(cnt > 0, "GFSReadDir: cnt <= 0");

	return FileSystem[dir_fp->inode->fs_type]->GFSReadDir(dir_fp, dirp, cnt);
}


int GFSLink(struct MInode *dir, struct MInode *file, const char *filename)
{
	assert(dir, "FSLink: dir == NULL.");
	assert(file, "FSLink: file == NULL.");
	assert(filename, "FSLink: filename == NULL.");
	
	return FileSystem[dir->fs_type]->GFSLink(dir, file, filename);
}


int GFSUnLink(struct MInode *dir, const char *filename)
{
	assert(dir, "FSUnLink: dir == NULL.");
	assert(filename, "FSUnLink: filename == NULL.");
	
	return FileSystem[dir->fs_type]->GFSUnLink(dir, filename);
}
