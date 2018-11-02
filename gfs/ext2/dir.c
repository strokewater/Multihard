#include "errno.h"
#include "string.h"
#include "stdlib.h"
#include "exception.h"
#include "mm/varea.h"
#include "gfs/gfs_utils.h"
#include "gfs/gfslevel.h"
#include "gfs/gfs.h"
#include "gfs/fcntl.h"
#include "gfs/dirent.h"
#include "gfs/minode.h"
#include "gfs/ext2/ext2.h"
#include "gfs/ext2/super.h"
#include "gfs/ext2/inode.h"

int Ext2ReadDir(struct File *dir_fp, struct dirent *dirp, int cnt)
{
	struct ext2_dir_entry entry;
	int i;
	for (i = 0; i < cnt; ++i)
	{
		int pos = dir_fp->pos;
		if (Ext2GetDirEntry(dir_fp, &entry) != 0)
			return  i;
		dirp[i].d_ino = entry.inode;
		dirp[i].d_off = pos;
		dirp[i].d_namlen = entry.name_len + 1;
		memcpy(dirp[i].d_name, entry.name, entry.name_len);
		dirp[i].d_name[entry.name_len] = '\0';
	}
	return i;
}

int Ext2GetDirEntry(struct File *dir_fp, struct ext2_dir_entry *entry)
{
	int ret;
	off_t pos = dir_fp->pos;
	ret = GFSRead(dir_fp, entry, 8);
	if (ret != 8)
	{
		dir_fp->pos = pos;
		return 1;
	}
	ret = GFSRead(dir_fp, (void *)entry + 8, entry->name_len);
	if (ret != entry->name_len)
		panic("Ext2GetDirEntry Cannot get complete dir_entry.");
	dir_fp->pos = pos + entry->rec_len;	
	return 0;
}

int INODE_FTYPE_TO_DIR_ENTRY_FTYPE(int inode_ftype)
{
	switch (inode_ftype & S_IFMT)
	{
		case S_IFREG:
			return EXT2_FT_REG_FILE;
		case S_IFBLK:
			return EXT2_FT_BLKDEV;
		case S_IFDIR:
			return EXT2_FT_DIR;
		case S_IFCHR:
			return EXT2_FT_CHRDEV;
		case S_IFIFO:
			return EXT2_FT_FIFO;
	}
	return EXT2_FT_UNKNOWN;
}

int Ext2Link(struct MInode *dir, struct MInode *file, const char *filename)
{
	struct ext2_dir_entry *entry;
	struct File *dir_fp;
	int need_len;
	int filename_len;
	int rec_occupy_len, rec_free_len;
	int pos;
	
	filename_len = need_len = strlen(filename);
	if (need_len % 4)
		need_len = (need_len / 4 + 1) * 4;
	need_len += 8;
	
	entry = KMalloc(sizeof(*entry));
	dir_fp = GFSOpenFileByMInode(dir, MAY_READ | MAY_WRITE | MAY_EXEC);
	
	while (1)
	{
		pos = dir_fp->pos;
		if (Ext2GetDirEntry(dir_fp, entry) != 0)
			break;
			
		rec_occupy_len = 8 + entry->name_len;
		if (rec_occupy_len % 4)
			rec_occupy_len = (rec_occupy_len / 4 + 1) * 4;
		rec_free_len = entry->rec_len - rec_occupy_len;
		if (rec_free_len >= need_len)
		{
			entry->rec_len -= need_len;
			dir_fp->pos = pos;
			GFSWrite(dir_fp, entry, 8);
			dir_fp->pos = pos + entry->rec_len;
			break;
		}
	}
	
	if (dir_fp->pos % 4 != 0)
		dir_fp->pos = (dir_fp->pos / 4 + 1) * 4;
	pos = dir_fp->pos;
	
	struct MExt2SuperBlk *msb = Ext2GetSuperBlk(file->dev);
	int blk_size = msb->block_size;
	
	file->nlinks++;
	entry->inode = file->inum;
	if ((dir_fp->pos + need_len) % blk_size)
	{
		int tmp = dir_fp->pos + need_len;
		tmp = (tmp / blk_size + 1) * blk_size;
		need_len = tmp - dir_fp->pos;
	}
	entry->rec_len = need_len;
	entry->name_len = filename_len;
	entry->file_type = INODE_FTYPE_TO_DIR_ENTRY_FTYPE(file->mode);
	memcpy(entry->name, filename, filename_len);
	GFSWrite(dir_fp, entry, entry->rec_len);
	KFree(entry, 0);
	GFSCloseFileByFp(dir_fp);
	return 0;
}

int Ext2UnLink(struct MInode *dir, const char *filename)
{
	struct File *dir_fp;
	struct ext2_dir_entry *now, *prev;
	int pos, prev_pos;
	int old_rec_len;
	struct MInode *fminode;
	struct MExt2SuperBlk *super;
	
	prev = KMalloc(sizeof(*prev));
	now = KMalloc(sizeof(*now));
	prev->rec_len = 0;
	
	dir_fp = GFSOpenFileByMInode(dir, MAY_READ | MAY_WRITE | MAY_EXEC);
	pos = prev_pos = 0;
	
	while (1)
	{
		pos = dir_fp->pos;
		if (Ext2GetDirEntry(dir_fp, now) != 0)
			break;
			
		if (GFSIsCStrEqualEntryName(filename, now->name, now->name_len))
		{
			old_rec_len = now->rec_len;
			fminode = GetMInode(dir->dev, now->inode);
			super = Ext2GetSuperBlk(dir->dev);
			fminode->nlinks--;
			fminode->dirt = 1;
			if (fminode->nlinks == 0)
				FreeInode(super, fminode);
			else
				FreeMInode(fminode);
			memset(now, 0, old_rec_len);
			dir_fp->pos = pos;
			GFSWrite(dir_fp, now, old_rec_len);
			
			dir_fp->pos = prev_pos;
			Ext2GetDirEntry(dir_fp, now);
			now->rec_len += old_rec_len;
			dir_fp->pos = prev_pos;
			GFSWrite(dir_fp, now, 8);
			
			KFree(prev, 0);
			KFree(now, 0);
			GFSCloseFileByFp(dir_fp);
			
			return 0;
		}
		
		prev_pos = pos;
	}
	
	KFree(prev, 0);
	KFree(now, 0);
	GFSCloseFileByFp(dir_fp);
	// file not found.
	return -ERROR;
}

int Ext2MkDir(struct MInode *parent, const char *filename, int mode)
{
	struct MInode *p;
	struct MExt2SuperBlk *msb;
	
	msb = Ext2GetSuperBlk(parent->dev);
	p = GetFreeInode(msb);
	if (p == NULL)
		return -ENOSPC;
	LockMInode(p);
	p->size = 0;
	p->ctime = p->atime = CURRENT_TIME;
	p->uid = Current->euid;
	p->gid = Current->egid;
	p->nlinks = 0;
	p->mode = (mode & 07777 & ~Current->umask) | S_IFDIR;
	p->dirt = 1;
	int i;
	for (i = 0; i < MAX_ZONES; ++i)
		p->zone[i] = 0;
	UnLockMInode(p);
	
	Ext2Link(parent, p, filename);
	Ext2Link(p, p, ".");
	Ext2Link(p, parent, "..");
	FreeMInode(p);
	return 0;
}

int Ext2RmDir(struct MInode *parent, const char *dirname)
{
	struct File *dir_fp;
	struct ext2_dir_entry *entry;
	struct MInode *mi;
	
	entry = KMalloc(sizeof(*entry));
	if (entry == NULL)
		return -ENOMEM;
	
	mi = GFSReadMInode(parent, dirname);
	if (mi == NULL)
	{
		KFree(entry, 0);
		return Current->errno;
	}
	dir_fp = GFSOpenFileByMInode(mi, MAY_READ | MAY_WRITE | MAY_EXEC);
	
	while (Ext2GetDirEntry(dir_fp, entry) == 0)
	{
		if ((memcmp(entry->name, ".", 1) != 0) && (memcmp(entry->name, "..", 2) != 0))
		{
			KFree(entry, 0);
			GFSCloseFileByFp(dir_fp);
			FreeMInode(mi);
			return -ERROR;
		}
	}
	
	Ext2UnLink(parent, dirname);
	
	GFSCloseFileByFp(dir_fp);
	KFree(entry, 0);
	FreeMInode(mi);
	return 0;
}
