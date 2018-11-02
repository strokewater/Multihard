#ifndef FS_EXT2_RW_H
#define FS_EXT2_RW_H

#include "types.h"
#include "gfs/gfs.h"

ssize_t Ext2Read(struct File *fp, void *buf, size_t count);
ssize_t Ext2Write(struct File *fp, void *buf, size_t count);

ssize_t Ext2ReadFile(struct File *fp, void *buf, size_t count);
ssize_t Ext2WriteFile(struct File *fp, void *buf, size_t count);

#endif
