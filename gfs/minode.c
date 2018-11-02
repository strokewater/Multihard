#include "sched.h"
#include "stdlib.h"
#include "exception.h"
#include "asm/asm.h"
#include "gfs/minode.h"
#include "gfs/gfslevel.h"
#include "drivers/drivers.h"


static struct MInode MInodeGroup[NR_MINODES];
static struct Process *WaitFreeMInode;

void LockMInode(struct MInode *p)
{
	//debug_printk("[FS] Process %d trying to lock MInode(%d)", Current->pid, p->inum);
	WaitOnMInode(p);
	ASMCli();
	p->lock = 1;
	ASMSti();
	//debug_printk("[FS] Process %d Lock MInode(%d).", Current->pid, p->inum);
}

void UnLockMInode(struct MInode *p)
{
	ASMCli();
	p->lock = 0;
	ASMSti();
	//debug_printk("[FS] Process %d UnLock MInode(%d).", Current->pid, p->inum);
	WakeUp(&(p->wait_process));
}

void WaitOnMInode(struct MInode *p)
{
	while (p->lock)
	{
		//debug_printk("[FS] Process %d sleeping for MInode(%d).", Current->pid, p->inum);
		Sleep(&(p->wait_process), SLEEP_TYPE_UNINTABLE);
		//debug_printk("[FS] Process %d wake up from waitting for MInode(%d). ", Current->pid, p->inum);
	}
}

#define BADNESS(p)	((((((p)->count == 0) << 1) | (((p)->lock) == 0)) << 16) | (p)->used_count)

struct MInode *GetMInode(dev_no dev, ino_t inode_no)
{
	struct MInode *p;
	int i, j;
	int fs_type;
	struct MinorDevice *md;
	
	md = GetMinorDevice(dev);
	fs_type = md->fs_type;
	

Repeat:
	for (i = 0, j = -1; i < NR_MINODES; ++i)
	{
		p = MInodeGroup + i;
		if (p->dev == dev && p->inum == inode_no)
		{
			p->used_count++;
			if (p->used_count > MAX_USED_COUNT)
				p->used_count = MAX_USED_COUNT;
			p->count++;
			WaitOnMInode(p);
			return GFSGetMInode(p);
		}
		if (p->count == 0 && p->lock == 0)
		{
			if (j == -1 || MInodeGroup[j].used_count > p->used_count)
				j = i;
		}
	}
	
	p = MInodeGroup + j;
	if (j == -1)
	// no free minode
	{
		Sleep(&WaitFreeMInode, SLEEP_TYPE_UNINTABLE);
		goto Repeat;
	}
	
	ASMCli();
	p->count++;
	ASMSti();
	
	// now: p->cout == 1(prev is 0) && p->lock = 0 || as following
	if (p->dirt)
	{
		if (GFSHWriteMInode(p) == 0)
			p->dirt = 0;
		else
			panic("FSHWriteMInode fail.");
	}
	
	if (p->count > 1)
	{
		// other process will do actions on it.
		// so give up.
		p->count--;
		UnLockMInode(p);
		goto Repeat;
	}
	if (p->dev != 0)
		GFSCleanMInodeExInfo(p);
		
	// finally. (p->) count == 1(this occupy), dirt == 0, lock == 0
	// so update it.
	p->used_count = 0;
	p->count = 1;
	p->dev = dev;
	p->inum = inode_no;
	p->update = 0;
	p->fs_type = fs_type;
	UnLockMInode(p);
	if (GFSHReadMInode(p) == 0)
	{
		p->update = 1;
		UnLockMInode(p);
		return GFSGetMInode(p);
	}
	else
	{
		LockMInode(p);
		p->update = 0;
		p->dev = 0;
		p->inum = 0;
		p->count--;
		UnLockMInode(p);
		return NULL;
	}
}

void FreeMInode(struct MInode *p)
{
	p->count--;
	if (p->count == 0)
		WakeUp(&WaitFreeMInode);	
}

int IsMInodeOfDevVaildExist(dev_no dev)
{
	int i;
	struct MinorDevice *md;
	md = GetMinorDevice(dev);
	struct MInode *mi = md->root;
	for (i = 0; i < NR_MINODES; ++i)
	{
		if (MInodeGroup + i != mi && MInodeGroup[i].dev == dev && MInodeGroup[i].count > 0)
			return 1;
	}
	
	return 0;
}

int SyncMInodes(dev_no dev)
{
	int i;
	for (i = 0; i < NR_MINODES; ++i)
	{
		if ((MInodeGroup[i].dev == dev || dev == 0) && MInodeGroup[i].dirt)
		{
			GFSHWriteMInode(MInodeGroup + i);
			MInodeGroup[i].update = 1;
			MInodeGroup[i].dirt = 0;
		}
	}
	
	return 0;
}
