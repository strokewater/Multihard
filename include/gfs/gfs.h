#ifndef FS_H
#define FS_H

#include "types.h"
#include "process.h"
#include "config.h"
#include "gfs/minode.h"
#include "gfs/buffer.h"
#include "gfs/dirent.h"

#define NR_FS_TYPES		255
#define NR_MAX_OPEN_FILES	30


#define FS_NAME_MAX_LEN		11

#define MF_RDONLY		1
#define MF_RW			0
#define MF_DEFAULT		MF_RW

#define FS_TYPE_RAW		-1

struct File
{
	mode_t mode;	// mode 参数？
	u16 flags;
	u16 count;
	struct MInode *inode;
	off_t pos;
};

struct FSType
{
	char FSName[FS_NAME_MAX_LEN+1];
	
	struct MInode *(*GFSReadMInode)(struct MInode *root, const char *path);
	int (*GFSHReadMInode)(struct MInode *p);
	int (*GFSHWriteMInode)(struct MInode *p);
	struct MInode *(*GFSGetMInode)(struct MInode *p);
	void (*GFSCleanMInodeExInfo)(struct MInode *p);
	
	int (*GFSOpen)(struct MInode *minode, int flags, mode_t mode);
	struct MInode *(*GFSCreat)(struct MInode *root, const char *name, mode_t mode);
	ssize_t (*GFSRead)(struct File *fp, void *buf, size_t count);
	ssize_t (*GFSWrite)(struct File *fp, void *buf, size_t count);
	int (*GFSClose)(int fd);

	int (*GFSMkNod)(struct MInode *parent, const char *filename, mode_t mode, dev_no dev);
	int (*GFSLink)(struct MInode *dir, struct MInode *file, const char *filename);
	int (*GFSUnLink)(struct MInode *dir, const char *filename);

	int (*GFSMount)(struct MInode *mdev, struct MInode *mdir, int fs_type, int flags);
	int (*GFSMounted)(dev_no dev, struct MInode *mdir, int flags);
	int (*GFSUMount)(struct MInode *mdir);
	int (*GFSUMounted)(struct MInode *mdir);

	int (*GFSReadDir)(struct File *dir_fp, struct dirent *dirp, int cnt);
	int (*GFSMkDir)(struct MInode *parent, const char *filename, int mode);
	int (*GFSRmDir)(struct MInode *parent, const char *dirname);

};


void GFSInit();

int GetFSTypeNo(char *fs_type_str);
int InstallFS(struct FSType *fs);

struct File *GetFreeFileRec(void);
void FreeFileRec(struct File *fp);

extern struct CMDLineOption FSCMDLineOptions[];

extern dev_no RootDevNo;
extern char RootFSName[];

/* 通用文件系统，规定和自行处理的部分：
 * 自行处理的部分：
 * 1. 常规文件和文件夹
 * 2. 设备文件和PIPE
 * 3. 自行定义的其它类文件
 * 4. 根目录获取
 * 5. Inode的硬软读写
 * 规定的部分：
 * 1. MInode磁盘部分含义必须相同，详见MInode头文件
 * 2. 文件的访问权限（RWX和SetUID，SETGID和Sticky位）
 * 3. 目录分隔符
 * 4. 挂载时mount只允许置一，其他信息通过挂载项保存
 * 5. 各系统调用的接口（Sys层）
 * 6. 部分文件系统调用层的接口（FS层）
 */

extern struct FSType *FileSystem[];

#endif
