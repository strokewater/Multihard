#ifndef OPEN_H
#define OPEN_H

#include "types.h"
#include "gfs/minode.h"

int Ext2Open(struct MInode *minode, int flags, mode_t mode);
int Ext2Close(int fd);
struct MInode *Ext2Creat(struct MInode *root, const char *name, mode_t mode);
int Ext2MkNod(struct MInode *root, const char *filename, mode_t mode, dev_no dev);

#endif
