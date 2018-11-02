#ifndef FS_EXT2_DIR_H
#define FS_EXT2_DIR_H

#include "gfs/dirent.h"

int Ext2ReadDir(struct File *dir_fp, struct dirent *dirp, int cnt);

int Ext2GetDirEntry(struct File *dir_fp, struct ext2_dir_entry *entry);
void Ext2WriteDirEntry(struct File *dir, struct ext2_dir_entry *entry);

int Ext2Link(struct MInode *dir, struct MInode *file, const char *filename);
int Ext2UnLink(struct MInode *dir, const char *filename);
int Ext2MkDir(struct MInode *parent, const char *filename, int mode);
int Ext2RmDir(struct MInode *parent, const char *dirname);


#endif
