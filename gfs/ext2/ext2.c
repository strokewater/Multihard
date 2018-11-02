#include "gfs/ext2/ext2.h"
#include "gfs/ext2/block.h"
#include "gfs/ext2/inode.h"
#include "gfs/ext2/super.h"
#include "gfs/ext2/open.h"
#include "gfs/ext2/rw.h"
#include "gfs/ext2/truncate.h"
#include "gfs/ext2/dir.h"
#include "string.h"
#include "gfs/gfs.h"


static struct FSType FSTypeExt2 = {	.GFSReadMInode = Ext2ReadMInode, 
					.GFSHReadMInode = Ext2HReadMInode, 
					.GFSHWriteMInode = Ext2HWriteMInode,
					.GFSOpen = Ext2Open, 
					.GFSCreat = Ext2Creat, 
					.GFSRead = Ext2Read, 
					.GFSWrite = Ext2Write, 
					.GFSClose = Ext2Close, 
					.GFSMount = Ext2Mount, 
					.GFSUMount = Ext2UMount,
					.GFSMounted = Ext2Mounted, 
					.GFSUMounted = Ext2UMounted, 
					.GFSGetMInode = Ext2GetMInode, 
					.GFSCleanMInodeExInfo = Ext2CleanMInodeExInfo,
					.GFSMkNod = Ext2MkNod, 
					.GFSMkDir = Ext2MkDir, 
					.GFSRmDir = Ext2RmDir, 
					.GFSReadDir = Ext2ReadDir,
					.GFSLink = Ext2Link,
					.GFSUnLink = Ext2UnLink };

int Ext2FSTypeNo;

void Ext2Init()
{
	strncpy(FSTypeExt2.FSName, "ext2", FS_NAME_MAX_LEN);
	Ext2FSTypeNo = InstallFS(&FSTypeExt2);
	if (Ext2FSTypeNo < 0)
		;
}
