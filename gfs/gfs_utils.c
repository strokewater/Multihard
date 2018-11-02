#include "stdlib.h"
#include "errno.h"
#include "ctype.h"
#include "string.h"
#include "mm/varea.h"
#include "drivers/drivers.h"
#include "gfs/stat.h"
#include "gfs/fcntl.h"
#include "gfs/gfslevel.h"
#include "gfs/minode.h"
#include "gfs/gfs_utils.h"


struct MInode *GFSGetRootMInode(dev_no dev)
{
	struct MinorDevice *md;
	struct MInode *ret;
	
	md = GetMinorDevice(dev);
	if (md == NULL)
		return NULL;
	ret = md->root;
	return ret;
}

struct File *GFSOpenFileByMInode(struct MInode *minode, int per)
{
	struct File *ret;
	if (!GFSPermission(minode, per))
	{
		Current->errno = -EACCES;
		return NULL;
	}
		
	LockMInode(minode);
	ret = KMalloc(sizeof(*ret));
	if (ret == NULL)
	{
		UnLockMInode(minode);
		Current->errno = -ENOMEM;
		return NULL;
	}
	ret->mode = minode->mode;
	ret->flags = O_RDONLY;
	ret->count = 1;
	ret->inode = minode;
	ret->pos = 0;
	
	minode->count++;
	UnLockMInode(minode);
	return ret;
}

void GFSCloseFileByFp(struct File *fp)
{
	fp->inode->count--;
	KFree(fp, sizeof(*fp));
}


int GFSPermission(struct MInode *inode, int mask)
{
	int ret = 1;
	static int per_tables[] = 	{S_IRUSR, S_IWUSR, S_IXUSR,
				 	S_IRGRP, S_IWGRP, S_IXGRP,
				 	S_IROTH, S_IWOTH, S_IXOTH};
	int offset;
	if (inode->dev && !inode->nlinks)
		return 0;
	if (Current->euid == 0)
		return 1;
	
	if (Current->euid == inode->uid)
	{
		/// owner.
		offset = 0;
	}else if(Current->egid == inode->gid)
	{
		offset = 3;
		/// group,
	}else
	{
		offset = 6;
		// others.
	}
	if (mask & MAY_READ)
		ret = ret && (inode->mode & per_tables[offset]) == 0 ? 0 : 1;
	if (mask & MAY_WRITE)
		ret = ret && (inode->mode & per_tables[offset + 1]) == 0 ? 0 : 1;
	if (mask & MAY_EXEC)
		ret = ret && (inode->mode & per_tables[offset + 2]) == 0 ? 0 : 1;
	return ret;
}

struct MInode *GFSGetMInodeByFileName(const char *filename)
{
	struct MInode *ret, *root;
	
	if (*filename == '/')
		root = Current->root;
	else
		root = Current->pwd;
	ret = GFSReadMInode(root, filename);
	
	return ret;
}

struct MInode *GFSGetParentDirMInode(const char *path, char ** filename)
{
	int name_len;
	char *tmp;
	struct MInode *ret;
	struct MInode *root;
	int i;
	
	name_len = strlen(path);
	tmp = KMalloc(name_len + 1);
	strcpy(tmp, path);
	
	for (i = name_len - 1; i >= 0; --i)
	{
		if (tmp[i] == '/')
			break;
	}
	
	if (i < 0)
	{
		*filename = tmp;
		Current->pwd->count++;
		return Current->pwd;
	}
	else if(i == 0)
	{
		*filename = tmp + 1;
		Current->root->count++;
		return Current->root;
	}else
	{
		if (tmp[0] == '/')
			root = Current->root;
		else
			root = Current->pwd;
		
		tmp[i] = 0; // /AB/C
		ret = GFSReadMInode(root, tmp);
		KFree(tmp, 0);
		*filename = path + i + 1;
		return ret;
	}
}

int GFSIsCStrEqualEntryName(const char cstr[], const char entry_name[], size_t entry_name_size)
{
	int i;
	for (i = 0; cstr[i]; ++i)
	{
		if (i >= entry_name_size)
			return 0;
		else if (cstr[i] != entry_name[i])
			return 0;
	}
	return i == entry_name_size;
}
