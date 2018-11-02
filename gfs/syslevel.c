#include "stdlib.h"
#include "ctype.h"
#include "sched.h"
#include "errno.h"
#include "printk.h"
#include "string.h"
#include "exception.h"
#include "mm/varea.h"
#include "gfs/gfs.h"
#include "gfs/minode.h"
#include "gfs/utime.h"
#include "gfs/fcntl.h"
#include "gfs/mtable.h"
#include "gfs/syslevel.h"
#include "gfs/gfslevel.h"
#include "gfs/gfs_utils.h"
#include "gfs/dirent.h"


int SysOpen(char *name, int oflags, mode_t mode)
{
	int ret;
	struct MInode *mi;
	struct MInode *root;
	
	debug_printk("[FS] Process %s(PID=%d) Do SysOpen(%s, %d).", Current->name, Current->pid, name, oflags);
	
	// did not consider create file.
	if (*name == '/')
		root = Current->root;
	else
		root = Current->pwd;
	mi = GFSReadMInode(root, name);
	if (mi == NULL)
	{
		if (oflags & O_CREAT)
		{
			mi = GFSCreat(root, name, mode);
			if (mi == NULL)
				return Current->errno;
		}else
			return Current->errno; //file is not exist.
	}

	ret = FileSystem[mi->fs_type]->GFSOpen(mi, oflags, mode);
	if (ret == 0 && mode & O_CLOEXEC)
		Current->close_on_exec |= (1 << ret);

	debug_printk("[FS] SysOpen(%s, %d) == %d", name, oflags, ret);
	FreeMInode(mi);
	return ret;
}

int SysCreat(char *name, mode_t mode)
{
	return SysOpen(name, O_WRONLY | O_CREAT, mode);
}

ssize_t SysRead(int fd, void *buf, size_t count)
{
	debug_printk("[FS] Process %s(PID=%d) Do SysRead(%d, %x, %d).", Current->name, Current->pid, fd, buf, count);
	struct File *fp;
	if (fd < 0 || fd >= NR_OPEN)
		return -EBADF;
	if (buf == NULL)
		return -EFAULT;
	if (count < 0)
		return -EFAULT;
	fp = Current->filep[fd];
	
	int ret = GFSRead(fp, buf, count);
	debug_printk("[FS] Process %s(PID=%d) Do SysRead(%d, %x, %d) == %d", Current->name, Current->pid, fd, buf, count, ret);
	return ret;
}

ssize_t SysWrite(int fd, void *buf, size_t count)
{
	debug_printk("[FS] Process %s(PID=%d) Do SysWrite(%d, %x, %d).", Current->name, Current->pid, fd, buf, count);
	ssize_t ret;
	struct File *fp;
	if (fd < 0 || fd >= NR_OPEN)
		return -EBADF;
	if (buf == NULL)
		return -EFAULT;
	if (count < 0)
		return -EFAULT;
	fp = Current->filep[fd];
	ret =  GFSWrite(fp, buf, count);
	debug_printk("[FS] Process %s(PID=%d) Do SysWrite(%d, %x, %d) == %d", Current->name, Current->pid, fd, buf, count, ret);
	return ret;
}

int SysClose(int fd)
{
	struct File *fp;
	int ret;
	debug_printk("[FS] Process %s(PID=%d) Do SysClose(%d).", Current->name, Current->pid, fd);
	if (fd < 0)
		return -EFAULT;
	if (fd >= NR_MAX_OPEN_FILES)
		return -EFAULT;
	
	fp = Current->filep[fd];
	ret = FileSystem[fp->inode->fs_type]->GFSClose(fd);
	if (ret == 0)
		Current->filep[fd] = NULL;
	return ret;
}


int SysMount(char *dev, char *dir, char *fs_type_str, int flags)
{
	int ret;
	struct MInode *mdev, *mdir;
	int fs_type;
	flags = 0;
	
	fs_type = GetFSTypeNo(fs_type_str);
	if (fs_type == -1)
		return -EFAULT;
	
	if (*dev == '/')
		mdev = GFSReadMInode(Current->root, dev);
	else
		mdev = GFSReadMInode(Current->pwd, dev);
	
	if (mdev == NULL)
		return -EFAULT;
	
	if (*dev == '/')
		mdir = GFSReadMInode(Current->root, dir);
	else
		mdir = GFSReadMInode(Current->pwd, dir);
	
	if (mdir == NULL)
	{
		FreeMInode(mdev);
		return -EFAULT;
	}
	
	ret = FileSystem[mdir->fs_type]->GFSMount(mdev, mdir, fs_type, flags);
	FreeMInode(mdir);
	FreeMInode(mdev);
	return ret;
}

int SysUMount(char *dir)
{
	int ret;
	struct MInode *dev_root_dir, *mdir;
	struct MountItem *mi;
	struct MinorDevice *md;

	if (*dir == '/')
		dev_root_dir = GFSReadMInode(Current->root, dir);
	else
		dev_root_dir = GFSReadMInode(Current->pwd, dir);
	
	if (dev_root_dir == NULL)
		return -EFAULT;
	md = GetMinorDevice(dev_root_dir->dev);
	mi = GetMItemFromMTable(dev_root_dir->mount);
	FreeMInode(dev_root_dir);
	mdir = mi->mount_dir;
	ret = FileSystem[mdir->fs_type]->GFSUMount(mdir);
	FreeMInode(md->root);
	md->root = NULL;
	return ret;
}


int SysChDir(const char *filename)
{
	struct MInode *inode;
	struct MInode *rt;

	debug_printk("[FS] SysChDir(%s)", filename);
	
	if (*filename == '/')
		rt = Current->root;
	else
		rt = Current->pwd;
	inode = GFSReadMInode(rt, filename);
	
	if (inode == NULL)
		return -ENOENT;
		
	if (!S_ISDIR(inode->mode))
	{
		FreeMInode(inode);
		return -ENOTDIR;
	}
	FreeMInode(Current->pwd);
	Current->pwd = inode;
	// 增加Current->pwd指向的MInode的计数, 因函数结束而要减少inode指向的MInode的计数
	// 然而这两个指针指向同一个MInode, 因此相互抵消不予处理.
	// SysChRoot类似
	return 0;
}


int SysChRoot(const char *filename)
{
	struct MInode *inode;
	struct MInode *rt;

	debug_printk("[FS] SysChroot(%s)", filename);
	
	if (*filename == '/')
		rt = Current->root;
	else
		rt = Current->pwd;
	inode = GFSReadMInode(rt, filename);
	if (inode == NULL)
		return -ENOENT;
	if (!S_ISDIR(inode->mode))
	{
		FreeMInode(inode);
		return -ENOTDIR;
	}
	FreeMInode(Current->root);
	Current->root = inode;
	// 同SysChDir
	return 0;
}

int SysChMod(const char *filename, mode_t mode)
{
	struct MInode *mi;
	int ret;
	debug_printk("[FS] SysChMod(%s, %x)", filename, mode);
	mi = GFSGetMInodeByFileName(filename);
	
	if (mi == NULL)
		return Current->errno;

	ret = GFSChMod(mi, mode);
	FreeMInode(mi);	
	return ret;
}

int SysChOwn(const char *filename, uid_t uid, gid_t gid)
{
	struct MInode *mi;
	int ret;
	debug_printk("[FS] SysChOwn(%s, %d, %d)", filename, uid, gid);
	mi = GFSGetMInodeByFileName(filename);
	
	if (mi == NULL)
		return Current->errno;

	ret = GFSChOwn(mi, uid, gid);	
	FreeMInode(mi);
	
	return ret;
}

int SysAccess(const char *filename, int mode)
{
	int ret;

	debug_printk("[FS] SysAccess(%s, %x)", filename, mode);

	struct MInode *mi;
	mi = GFSGetMInodeByFileName(filename);
	
	if (mode == F_OK)
	{
		if (mi == NULL)
			return Current->errno;
		else
		{
			FreeMInode(mi);
			return 0;
		}
	}
	if (mi == NULL)
		return Current->errno;

	ret = GFSAccess(mi, mode);
	FreeMInode(mi);
	return ret;
}

int SysStat(const char *filename, struct stat *buf)
{
	struct MInode *mi;
	int ret;
	debug_printk("[FS] SysStat(%s)", filename);
	mi = GFSGetMInodeByFileName(filename);
	if (mi == NULL)
		return Current->errno;
	ret = GFSStat(mi, buf);
	FreeMInode(mi);
	return ret;
}

int SysFStat(int fd, struct stat *buf)
{
	debug_printk("[FS] SysFStat(%d, %x)", fd, buf);
	struct File *fp;
	int ret;
	if (fd < 0 || fd >= NR_OPEN)
		return -EBADF;
	if (Current->filep[fd] == NULL)
		return -EBADF;
	fp = Current->filep[fd];
	ret = GFSStat(fp->inode, buf);
	
	return ret;
}

int SysLStat(const char *filename, struct stat *buf)
{
	return SysStat(filename, buf);
}

int SysLseek(int fd, off_t offset, int origin)
{
	struct File *fp;

	debug_printk("[FS] SysLseek(%d, %d, %d)", fd, offset, origin);

	if (fd < 0 || fd >= NR_OPEN)
		return -EBADF;
	if ((fp = Current->filep[fd]) == NULL)
		return -EBADF;
	if (fp->inode->pipe)
		return -ESPIPE;
	if (origin == 0)
	{
		if (offset < 0)
			return -EINVAL;
		fp->pos = offset;
	} else if(origin == 1)
	{
		if (fp->pos + offset < 0)
			return -EINVAL;
		fp->pos += offset;
	} else if(origin == 2)
	{
		if (fp->inode->size + offset < 0)
			return -EINVAL;
		fp->pos = fp->inode->size + offset;
	} else
		return -EINVAL;
	
	return fp->pos;
}

int SysMkNod(const char *path, mode_t mode, dev_no dev)
{
	int ret;
	struct MInode *parent_dir;
	char *filename = NULL;

	debug_printk("[FS] SysMkNod(%s, %x, %x)", path, mode, dev);

	parent_dir = GFSGetParentDirMInode(path, &filename);
	if (parent_dir == NULL)
	{
		if (IS_VAREA_MEM((addr)filename))
			KFree(filename, 0);
		return Current->errno;
	}
	ret = GFSMkNod(parent_dir, filename, mode, dev);
	FreeMInode(parent_dir);
	if (IS_VAREA_MEM(filename))
		KFree(filename, 0);
	return ret;
}

int SysMkDir(const char *path, int mode)
{
	int ret;
	struct MInode *parent_dir;
	char *filename = NULL;

	debug_printk("[FS] SysMkDir(path = %s, mode = %x)", path, mode);

	parent_dir = GFSGetParentDirMInode(path, &filename);
	if (parent_dir == NULL)
	{
		if (IS_VAREA_MEM(filename))
			KFree(filename, 0);
		debug_printk("[FS] SysMkDir return %d", Current->errno);
		return Current->errno;
	}
	ret = GFSMkDir(parent_dir, filename, mode);
	FreeMInode(parent_dir);
	if (IS_VAREA_MEM(filename))
		KFree(filename, 0);
	debug_printk("[FS] SysMkDir(path = %s, mode = %x) == %d", path, mode, ret);
	return ret;	
}

int SysRmDir(const char *path)
{
	int ret;
	struct MInode *parent_dir;
	char *filename;

	debug_printk("[FS] SysRmDir(%s)", path);

	parent_dir = GFSGetParentDirMInode(path, &filename);
	if (parent_dir == NULL)
	{
		if (IS_VAREA_MEM(filename))
			KFree(filename, 0);
		return Current->errno;
	}
	ret = GFSRmDir(parent_dir, filename);
	
	if (IS_VAREA_MEM(filename))
		KFree(filename, 0);
	FreeMInode(parent_dir);
	return ret;	
}

int SysReadDir(int fd, struct dirent *dirp, int cnt)
{
	struct File *fp = Current->filep[fd];
	return GFSReadDir(fp, dirp, cnt);
}

int SysLink(const char *old_path, const char *new_path)
{
	struct MInode *parent_dir;
	struct MInode *source;
	char *filename;
	struct MInode *root;
	int ret;

	if (old_path != NULL && new_path != NULL)
		debug_printk("[FS] SysLink(%s, %s)", old_path, new_path);

	if (old_path[0] == '/')
		root = Current->root;
	else
		root = Current->pwd;
	source = GFSReadMInode(root, old_path);
	
	if (source == NULL)
		return Current->errno;
		
	parent_dir = GFSGetParentDirMInode(new_path, &filename);
	if (parent_dir == NULL)
	{
		FreeMInode(source);
		if (IS_VAREA_MEM(filename))
			KFree(filename, 0);
		return Current->errno;
	}

	ret = GFSLink(parent_dir, source, filename);
	FreeMInode(parent_dir);
	FreeMInode(source);
	if (IS_VAREA_MEM(filename))
		KFree(filename, 0);
	
	return ret;
}

int SysUnLink(const char *path)
{
	int ret;
	struct MInode *parent_dir;
	char *filename;

	if (path != NULL)
		debug_printk("[FS] SysUnLink(%s, %s)", path);

	parent_dir = GFSGetParentDirMInode(path, &filename);
	if (parent_dir == NULL)
	{
		if (IS_VAREA_MEM(filename))
			KFree(filename, 0);
		return Current->errno;
	}
	ret = GFSUnLink(parent_dir, filename);
	FreeMInode(parent_dir);
	
	if (IS_VAREA_MEM(filename))
		KFree(filename, 0);
	return ret;	
}

int SysUTime(const char *filename, struct utime_buf *times)
{
	struct MInode *p;
	struct MInode *root;
	
	debug_printk("[FS] SysUTime(%s, %x)", filename, times);

	if (*filename == '/')
		root = Current->root;
	else
		root = Current->pwd;
	p = GFSReadMInode(root, filename);
	if (p == NULL)
		return Current->errno;
	p->atime = times->actime;
	p->mtime = times->modtime;
	p->dirt = 1;
	FreeMInode(p);
	
	return 0;
}

static int DupFD(int fd, unsigned int arg)
{
	if (fd >= NR_OPEN || Current->filep[fd] == NULL)
		return -EBADF;
	if (fd < 0)
		return -EBADF;
	if (arg >= NR_OPEN)
		return -EINVAL;
	while (arg < NR_OPEN && Current->filep[arg])
		++arg;
	if (arg >= NR_OPEN)
		return -EMFILE;
	Current->close_on_exec &= ~(1 << arg);
	Current->filep[arg] = Current->filep[fd];
	Current->filep[arg]->count++;
	return arg;
}

int SysDup2(int fd, int newfd)
{
	debug_printk("[FS] SysDup2(%d, %d)", fd, newfd);
	SysClose(newfd);
	return DupFD(fd, newfd);
}

int SysDup(int fd)
{
	debug_printk("[FS] SysDup(%d, %d)", fd);
	return DupFD(fd, 0);
}

int SysFcntl(int fd, int cmd, int arg)
{
	struct File *fp;
	if (fd < 0 || fd >= NR_OPEN || (fp = Current->filep[fd]))
		return -EBADF;
	switch (cmd)
	{
		case F_DUPFD:
			return DupFD(fd, arg);
		case F_GETFD:
			return (Current->close_on_exec >> fd) & 1;
		case F_SETFD:
			if (arg & 1)
				Current->close_on_exec |= (1 << fd);
			else
				Current->close_on_exec &= ~(1 << fd);
			return 0;
		case F_GETFL:
			return fp->flags;
		case F_SETFL:
			if (arg & O_APPEND)
				fp->flags |= O_APPEND;
			if (arg & O_NONBLOCK)
				fp->flags |= O_NONBLOCK;
			return 0;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			return -1;
		default:
			return -1;
	}
}

int SysIoctl(int fd, unsigned long cmd, unsigned long arg)
{
	struct File *fp;
	int dev, mode;
	int ret;

	debug_printk("[FS] SysIoctl(%d, %d, %d)", fd, cmd, arg);
	
	if (fd >= NR_OPEN)
	{
		ret = -EBADF;
		goto Ret;
	}
	fp = Current->filep[fd];
	if (fp == NULL)
	{
		ret = -EBADF;
		goto Ret;
	}
	mode = fp->inode->mode;
	if (!S_ISCHR(mode) && !S_ISBLK(mode))
	{
		ret = -EINVAL;
		goto Ret;
	}
	dev = fp->inode->zone[0];
	ret = DIoctl(dev, cmd, arg);
	
Ret:
	debug_printk("[FS] SysIoctl(%d, %d, %d) = %d", fd, cmd, arg, ret);
	return ret;
} 

int SysPipe(int fds[])
{
	int i, j;
	int ret_cd;

	debug_printk("[FS] SysPipe()");

	struct File *fp[2] = {NULL, NULL};
	int tmp_fds[2] = {-1, -1};
	struct MInode *mi = NULL;
	
	if (fds == NULL)
		return -ERROR;
		
	for (i = 0; i < 2; ++i)
	{
		fp[i] = GetFreeFileRec();
		fp[i]->inode = (struct MInode *)1;	// occupy place.
	}
		
	if (fp[0] == NULL || fp[1] == NULL)
	{
		ret_cd = -1;
		goto free_resource;
	}
	
	for (i = 0, j = 0; i < NR_OPEN; ++i)
	{
		if (j >= 2)
			break;
		if (Current->filep[i] == NULL)
		{
			tmp_fds[j] = i;
			Current->filep[i] = fp[j++];
		}
	}
	
	if (j != 2)
	{
		ret_cd = -1;
		goto free_resource;
	}
	
	mi = KMalloc(sizeof(*mi));
	if (mi == NULL)
	{
			ret_cd = -ENOMEM;
			goto free_resource;
	}
	mi->count = 2;
	mi->size = 0;
	mi->pipe = 1;
	
	*((char **)&mi->size) = KMalloc(PAGE_SIZE);
	if ((char *)mi->size == NULL)
	{
		ret_cd = -ENOMEM;
		goto free_resource;
	}
	mi->zone[0] = mi->zone[1] = 0;
	
	fp[0]->flags = O_RDONLY;
	fp[1]->flags = O_WRONLY;
	fp[0]->inode = fp[1]->inode = mi;	// mi的count不必再加.
	fp[0]->pos = fp[1]->pos = 0;
	fp[0]->count = fp[1]->count = 1;
	fp[0]->mode = O_RDONLY;
	fp[1]->mode = O_WRONLY;
	fds[0] = tmp_fds[0];
	fds[1] = tmp_fds[1];
	ret_cd = 0;

	return ret_cd;
free_resource:
	for (i = 0; i < 2; ++i)
	{
		if (fp[i])
			FreeFileRec(fp[i]);
		if (tmp_fds[i] != -1)
			Current->filep[tmp_fds[i]] = NULL;
	}
	if (mi != NULL)
	{
		if ((char **)&mi->size != NULL)
			KFree((char **)&mi->size, 0);
		KFree(mi, 0);
	}
	
	return ret_cd;
}

char *SysGetCWD(char *buf, size_t size)
{
#define N_ENTRY_PER_ARRAY	20

struct name_node
{
	char *name;
	size_t name_len;
	struct name_node *next;
};

	char *ret;
	struct MInode *cur_dir;
	struct MInode *parent_dir;
	struct File *fp;
	struct dirent *entrys;
	char *cur_dir_name;
	int total_size = 1;	// including NUL

	struct name_node *lst = 0;
	struct name_node *i_node;
	struct name_node *inext;

	entrys = KMalloc(sizeof(*entrys) * N_ENTRY_PER_ARRAY);

	cur_dir = Current->pwd;
	parent_dir = NULL;
	while (cur_dir != Current->root)
	{
		parent_dir = GFSReadMInode(cur_dir, "..");
		fp = GFSOpenFileByMInode(parent_dir, O_RDONLY);
		cur_dir_name = NULL;
		int i;
		int n;
		while ((n = GFSReadDir(fp, entrys, N_ENTRY_PER_ARRAY)) > 0)
		{
			for (i = 0; i < n; ++i)
			{
				if (entrys[i].d_ino == cur_dir->inum)
				{
					cur_dir_name = KMalloc(entrys[i].d_namlen);
					memcpy(cur_dir_name, entrys[i].d_name, entrys[i].d_namlen);
					total_size += entrys[i].d_namlen;

					if (total_size > size)
					{
						ret = (char *)-ERANGE;
						goto clean_resource;
					}

					break;
				}
			}
			if (cur_dir_name != NULL)
				break;
		}

		if (cur_dir_name == NULL)
			panic("Find Entry failed.");
		struct name_node *j = KMalloc(sizeof(*j));
		j->name = cur_dir_name;
		j->name_len = entrys[i].d_namlen;
		j->next = lst;
		lst = j;

		cur_dir = parent_dir;
	}

	ret = buf;
	*buf++ = '/';
	for (i_node = lst; i_node != NULL; i_node = i_node->next)
	{
		memcpy(buf, i_node->name, i_node->name_len);
		buf += i_node->name_len;
		*buf++ = '/';
	}

	if (lst == NULL)
		*buf = '\0';
	else
	{
		--buf;
		*buf = '\0';
	}

clean_resource:
	for (i_node = lst; i_node != NULL; i_node = inext)
	{
		inext = i_node->next;
		KFree(i_node->name, i_node->name_len);
		KFree(i_node, sizeof(*i_node));
	}
	KFree(entrys, sizeof(*entrys) * N_ENTRY_PER_ARRAY);

	return ret;
}
