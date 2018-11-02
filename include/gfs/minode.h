#ifndef FS_MINODE_H
#define FS_MINODE_H

#include "types.h"

#define NR_MINODES		32
#define MAX_USED_COUNT	64
#define MAX_ZONES		18

/* MInode作为文件系统的通用结点的数据结构，所表示的内容含义在各文件系统使用时不允许改变
 * 对于zone，数组项可以不直接全部保存数据块，但必须与数据块相关，设备文件设备号必须放在zone[0]
 * 对于权限和其他的信息可以放在extend_info数据结构.
 */

/* minode的count和lock
 * count记录的是minode的指针在所有内存的个数
 * 对于一个返回minode指针的函数，返回的minode的count已经自动增加
 * 看最终的目的，如果最终的目的是获取MInode指针的指针的，则count需要增加，如果只是从MInode指针副本复制另一个副本则不必增加。
 * 所有形参都属于第二种情况。
 * 减少count请使用FreeMInode
 * 
 * 锁变量：事到临头再锁，刚做完事就解锁
 */
struct MInode
{
	mode_t mode;
	uid_t uid;
	size_t size;
	u32 atime;
	u32 ctime;
	u32 mtime;
	u32 dtime;
	gid_t gid;
	u8 nlinks;
	u32 blocks;
	u32 flags;
	u32 zone[MAX_ZONES];
	void *extend_info;
	
	struct Process *wait_process;
	dev_no dev;
	ino_t inum;
	u16 count;
	u8 lock;
	u8 dirt;
	u8 pipe;
	u8 mount;
	u8 seek;
	u8 update;
	
	u16 used_count;
	int fs_type;	// index in FileSystem.
};

void LockMInode(struct MInode *p);
void UnLockMInode(struct MInode *p);
void WaitOnMInode(struct MInode *p);
struct MInode *GetMInode(dev_no dev, ino_t inode_no);
void FreeMInode(struct MInode *p);
int IsMInodeOfDevVaildExist(dev_no dev);
int SyncMInodes(dev_no dev);

#endif
