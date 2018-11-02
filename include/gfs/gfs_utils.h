#ifndef FS_FS_UTILS_H
#define FS_FS_UTILS_H

#define MAY_EXEC	1
#define MAY_WRITE	2
#define	MAY_READ	4

#include "drivers/drivers.h"
#include "gfs/gfs.h"
#include "gfs/stat.h"

struct MInode *GFSGetRootMInode(dev_no dev);
struct File *GFSOpenFileByMInode(struct MInode *minode, int per);
void GFSCloseFileByFp(struct File *fp);
int GFSPermission(struct MInode *inode, int mask);
struct MInode *GFSGetMInodeByFileName(const char *filename);
struct MInode *GFSGetParentDirMInode(const char *path, char ** filename);
int GFSIsCStrEqualEntryName(const char cstr[], const char entry_name[], size_t entry_name_size);


#endif
