#include "gfs/gfs.h"
#include "gfs/minode.h"
#include "gfs/fcntl.h"
#include "gfs/gfs_utils.h"
#include "gfs/stat.h"
#include "assert.h"
#include "ctype.h"
#include "string.h"
#include "drivers/drivers.h"
#include "sched.h"
#include "errno.h"
#include "config.h"
#include "gfs/ext2/ext2.h"
#include "gfs/buffer.h"
#include "mm/varea.h"
#include "stdlib.h"

static int SetRootFSDevNo(void);
static int SetRootFSFSName(void);

struct CMDLineOption FSCMDLineOptions[] = 
{
	{"--rootfs-dev", SetRootFSDevNo}, 
	{"--rootfs-fsname", SetRootFSFSName},
	{NULL, NULL}
};


dev_no RootDevNo;
char RootFSName[FS_NAME_MAX_LEN+1];

static int SetRootFSDevNo(void)
{
	if (NRCMDLineOptionPara == 0)
		return CMDLINE_OPTION_INT_SEQU;
	else if(NRCMDLineOptionPara > 1)
		panic("parameter to --rootfs-dev is too many.");
	else
	{
		RootDevNo = CMDLineOptionParaInt[0];
		return CMDLINE_OPTION_FINISH;
	}
	return -1;
}

static int SetRootFSFSName(void)
{
	if (NRCMDLineOptionPara == 0)
		return CMDLINE_OPTION_STR_SEQU;
	else if(NRCMDLineOptionPara > 1)
		panic("parameter to --rootfs-fsname is too many.");
	else
	{
		strcpy(RootFSName, CMDLineOptionParaStr[0]);
		return CMDLINE_OPTION_FINISH;
	}
	return -1;
}


struct FSType *FileSystem[NR_FS_TYPES];
struct File FileTables[NR_MAX_OPEN_FILES];

struct File *GetFreeFileRec(void)
{
	int i;
	for (i = 0; i < NR_MAX_OPEN_FILES; ++i)
	{
		if (FileTables[i].inode == NULL)
			return FileTables + i;
	}
	return NULL;
}

void FreeFileRec(struct File *fp)
{
	fp->inode = NULL;
}

int GetFSTypeNo(char *fs_type_str)
{
	int fs_type;
	
	for (fs_type = 0; fs_type < NR_FS_TYPES; ++fs_type)
		if (strcmp(fs_type_str, FileSystem[fs_type]->FSName) == 0)
			break;
	
	if (fs_type == NR_FS_TYPES)
		return -1;
	else
		return fs_type;
}

int InstallFS(struct FSType *fs)
{
	struct FSType *tmp;
	int i;
	
	tmp = KMalloc(sizeof(*tmp));
	if (tmp == NULL)
		return -1;
	
	*tmp = *fs;
	for (i = 0; i < NR_FS_TYPES; ++i)
	{
		if (FileSystem[i] == NULL)
		{
			FileSystem[i] = tmp;
			return i;
		}
	}
	
	KFree(tmp, sizeof(*tmp));
	return -1;
}

void GFSInit()
{
	if (RootDevNo == 0)
		panic("Need rootfs cmdline parameter.");

	InitBuffer();
	Ext2Init();
}
