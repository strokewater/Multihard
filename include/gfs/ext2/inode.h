#ifndef FS_EXT2_INODE_H
#define FS_EXT2_INDOE_H

#include "gfs/ext2/super.h"

struct Ext2InodeExInfo
{
	u32	generation;
	u32	file_acl;
	u32	dir_acl;
	u32	faddr;
};

struct MInode *GetFreeInode(struct MExt2SuperBlk *msb);
int FreeInode(struct MExt2SuperBlk *msb, struct MInode *inode);
struct MInode *Ext2GetMInode(struct MInode *p);
void Ext2CleanMInodeExInfo(struct MInode *p);

struct MInode *Ext2ReadMInode(struct MInode *root, const char *path);
int Ext2WriteMInode(struct MInode *p);
int Ext2HReadMInode(struct MInode *p);
int Ext2HWriteMInode(struct MInode *p);

#endif
