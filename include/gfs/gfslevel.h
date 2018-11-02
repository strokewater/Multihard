#ifndef FS_FS_FS_LEVEL_H
#define FS_FS_FS_LEVEL_H

#include "gfs/minode.h"
#include "types.h"
#include "gfs/stat.h"
#include "gfs/utime.h"
#include "gfs/gfs.h"

struct MInode *GFSGetMInode(struct MInode *p);
struct MInode *GFSReadMInode(struct MInode *root, const char *path);
int GFSHReadMInode(struct MInode *p);
int GFSHWriteMInode(struct MInode *p);
void GFSCleanMInodeExInfo(struct MInode *p);
int GFSMounted(s32 dev, struct MInode *mdir, int flags, int dev_fs_type);
int GFSUMount(struct MInode *mdir);
int GFSUMounted(struct MInode *mdir);
struct MInode *GFSCreat(struct MInode *root, const char *name, mode_t mode);
ssize_t GFSRead(struct File *fp, void *buf, size_t count);
ssize_t GFSWrite(struct File *fp, void *buf, size_t count);
int GFSChMod(struct MInode *minode, mode_t mode);
int GFSChOwn(struct MInode *minode, uid_t uid, gid_t gid);
int GFSAccess(struct MInode *minode, int mode);
int GFSStat(struct MInode *minode, struct stat *buf);
int GFSMkNod(struct MInode *parent, const char *filename, mode_t mode, dev_no dev);
int GFSMkDir(struct MInode *parent, const char *filename, int mode);
int GFSRmDir(struct MInode *parent, const char *dirname);
int GFSReadDir(struct File *dir_fp, struct dirent *dirp, int cnt);
int GFSLink(struct MInode *dir, struct MInode *file, const char *filename);
int GFSUnLink(struct MInode *dir, const char *filename);


#endif
