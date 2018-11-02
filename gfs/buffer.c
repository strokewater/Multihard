#include "gfs/buffer.h"
#include "mm/varea.h"
#include "stdlib.h"
#include "sched.h"
#include "asm/asm.h"
#include "gfs/minode.h"
#include "drivers/drivers.h"
#include "drivers/request.h"
#include "gfs/gfs.h"
#include "assert.h"
#include "printk.h"

// FreeList: all are unlocked, count = 0
static struct BufferHead *FreeList;
static struct BufferHead *FreeListTail;
static struct Process *BufferWait;
static struct BufferHead Bufs[NR_BUFS];
struct BufferHead *HashTable[NR_HASH];

void WaitOnBuffer(struct BufferHead *i)
{
	while (i->lock)
	{
		//debug_printk("[FS] Process PID = %d Wait Buffer(block_no = %d).", Current->pid, i->block_no);
		Sleep(&(i->wait_process), SLEEP_TYPE_UNINTABLE);
	}
}

void LockBuffer(struct BufferHead *i)
{
	WaitOnBuffer(i);
	//debug_printk("[FS] Process PID = %d Lock Buffer(block_no = %d).", Current->pid, i->block_no);
	ASMCli();
	i->lock = 1;
	ASMSti();
}

void UnLockBuffer(struct BufferHead *i)
{
	//debug_printk("[FS] Process PID = %d UnLock Buffer(block_no = %d).", Current->pid, i->block_no);
	ASMCli();
	i->lock = 0;
	ASMSti();
	WakeUp(&(i->wait_process));
}

static void PopFromFreeList(struct BufferHead *i)
{
	if (i->prev_free)
		i->prev_free->next_free = i->next_free;
	if (i->next_free)
		i->next_free->prev_free = i->prev_free;
	if (FreeList == i)
		FreeList = FreeList->next_free;
	if (FreeListTail == i)
		FreeListTail = i->prev_free;
}

static struct BufferHead *FindHashTable(dev_no dev, off_t block)
{
	struct BufferHead *tmp, *list;
	
	list = HashTable[block % NR_HASH];
	for (tmp = list; tmp != NULL; tmp = tmp->next)
		if (tmp->dev_no == dev && tmp->block_no == block)
			break;
	
	return tmp;
}

static void PopFromHashTable(struct BufferHead *i)
{
	if (i->prev)
		i->prev->next = i->next;
	if (i->next)
		i->next->prev = i->prev;
	
	if (HashTable[i->block_no % NR_HASH] == i)
		HashTable[i->block_no % NR_HASH] = i->next;
}

static void PushToFreeList(struct BufferHead *i)
{
	if (FreeList == NULL)
	{
		i->prev_free = i->next_free = NULL;
		FreeList = FreeListTail = i;
		return;
	}
	FreeListTail->next_free = i;
	i->prev_free = FreeListTail;
	i->next_free = NULL;
	FreeListTail = i;
}

static void PushToHashTable(struct BufferHead *i)
{
	struct BufferHead *list;
	struct BufferHead *tmp;
	
	list = HashTable[i->block_no % NR_HASH];
	// Empty?
	if (list == NULL)
	{
		i->prev = NULL;
		i->next = NULL;
		HashTable[i->block_no % NR_HASH] = i;
		return;
	}
	for (tmp = list; tmp->next != NULL; tmp = tmp->next)
		;
	tmp->next = i;
	i->prev = tmp;
	i->next = NULL;
}

int WriteBufferToDisk(struct BufferHead *i)
{
	struct MinorDevice *md;
	int dreq_id;
	size_t n;
	size_t written_blocks;
	
	md = GetMinorDevice(i->dev_no);

	assert(md->fs_blk_size >= md->block_size, "Block Size Error!");
	assert(md->fs_blk_size % md->block_size == 0, "Block Size Error!");

	n = md->fs_blk_size / md->block_size;
	if (md->drg_proprity.is_drg_needed)
	{
		dreq_id = MakeDevRequest(DEV_REQ_CMD_WRITE, i->dev_no, i->data, i->block_no * n, n);
		WaitDevRequest(i->dev_no, dreq_id);
	}
	written_blocks = DWrite(i->dev_no, i->data, i->block_no * n, n);
	
	if (md->drg_proprity.is_drg_needed)
		FinishDevRequest(i->dev_no);
	
	return written_blocks == n;	
}

struct BufferHead *GetBlkLoaded(dev_no dev, off_t block)
{
	int dreq_id;
	struct BufferHead *i;
	struct MinorDevice *md;
	size_t n;
	int read_status;
	
	i = GetBlk(dev, block);
	if (i->data == NULL)
	{
		md = GetMinorDevice(dev);

		assert(md->fs_blk_size >= md->block_size, "Block Size Error!");
		assert(md->fs_blk_size % md->block_size == 0, "Block Size Error!");
		
		i->data = KMalloc(md->fs_blk_size);
		if (i->data == NULL)
		{
			UnLockBuffer(i);
			PutBlk(i);
			return NULL;
		}
		i->data_size = md->fs_blk_size;
		
		n = md->fs_blk_size / md->block_size;
		if (md->drg_proprity.is_drg_needed)
		{
			dreq_id = MakeDevRequest(DEV_REQ_CMD_READ, dev, i->data, block * n, n);
			WaitDevRequest(i->dev_no, dreq_id);
		}
		
		read_status = DRead(dev, i->data, block * n, n);
		
		if (md->drg_proprity.is_drg_needed)
			FinishDevRequest(i->dev_no);
		if (read_status == n)
		{
			i->uptodate = 1;
			return i;
		}else
		{
			KFree(i->data, i->data_size);
			i->data = NULL;
			i->data_size = 0;
			PutBlk(i);
			return NULL;
		}
	}else
		return i;
}

struct BufferHead *GetBlk(dev_no dev, off_t block)
{
	struct BufferHead *i;
Repeat:
	if ((i = FindHashTable(dev, block)))
	{
		WaitOnBuffer(i);
		
		if (i->dev_no != dev || i->block_no != block)
			goto Repeat;
			
		ASMCli();
		i->count++;
		LockBuffer(i);
		ASMSti();
		
		PopFromFreeList(i);
		return i;
	}else
	{
		if (FreeList == NULL)
		{
			Sleep(&BufferWait, SLEEP_TYPE_UNINTABLE);
			goto Repeat;
		}else
		{
			i = FreeList;
			PopFromFreeList(i);
			if (i->dirt)
			{
				LockBuffer(i);
				WriteBufferToDisk(i);
				UnLockBuffer(i);
				PushToFreeList(i);
				
				goto Repeat;
			}else
			{
				// find a empty buffer.
				LockBuffer(i);
				if (i->data)
					KFree(i->data, i->data_size);
				i->data = NULL;
				i->data_size = 0;
				i->count = 1;
				i->dirt = 0;
				i->uptodate = 0;
				PopFromHashTable(i);
				i->dev_no = dev;
				i->block_no = block;
				PushToHashTable(i);
								
				return i;
			}
		}
	}
}

void PutBlk(struct BufferHead *blk)
{
	ASMCli();
	blk->count--;
	UnLockBuffer(blk);
	
	if (blk->count == 0)
		PushToFreeList(blk);
	ASMSti();
	
	WakeUp(&BufferWait);
}

void InitBuffer()
{
	int i;
	for (i = 1; i < NR_BUFS - 1; ++i)
	{
		Bufs[i].prev = Bufs[i].prev_free = Bufs + i - 1;
		Bufs[i].next = Bufs[i].next_free = Bufs + i + 1;
	}
	
	Bufs[0].prev = Bufs[0].prev_free = NULL;
	Bufs[0].next = Bufs[0].next_free = &Bufs[1];
	
	Bufs[NR_BUFS - 1].prev = Bufs[NR_BUFS - 1].prev_free = &Bufs[NR_BUFS - 2];
	Bufs[NR_BUFS - 1].next = Bufs[NR_BUFS - 1].next_free = NULL;
	
	FreeList = HashTable[0] = &Bufs[0];
	FreeListTail = &Bufs[NR_BUFS - 1];
}

int SyncRequestDevs[NR_BUFS];
int SyncRequestDevsTop = -1;

int SysSync(dev_no dev)
{
	SyncMInodes(dev);
	SyncRequestDevs[++SyncRequestDevsTop] = dev;
	return 0;
}
