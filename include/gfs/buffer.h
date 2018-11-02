#ifndef FS_BUFFER_H
#define FS_BUFFER_H

#include "types.h"

#define NR_HASH		307
#define NR_BUFS		128

struct BufferHead
{
	s8 *data;
	size_t data_size;
	off_t block_no;
	dev_no dev_no;
	u8 uptodate;
	u8 dirt;
	u8 count;
	u8 lock;
	struct Process *wait_process;
	struct BufferHead *prev;
	struct BufferHead *next;
	struct BufferHead *prev_free;
	struct BufferHead *next_free;
};


// 锁原则：count代表当前使用块的用户，锁代表独占状态
// 当一个用户上锁时，其他用户不允许访问
// 没有上锁时所有用户均可访问
// 获取块的函数返回的块均引用数加1且上锁
// 释放块的函数会解锁并引用数减1
// 从上层函数传下的块均已上锁
struct BufferHead *GetBlkLoaded(dev_no dev, off_t block);
struct BufferHead *GetBlk(dev_no dev, off_t block);
void PutBlk(struct BufferHead *blk);
void InitBuffer();

int WriteBufferToDisk(struct BufferHead *i);

void WaitOnBuffer(struct BufferHead *i);
void LockBuffer(struct BufferHead *i);
void UnLockBuffer(struct BufferHead *i);
int SysSync(dev_no dev);

struct BufferHead *AsyncBuffers;

extern int SyncRequestDevs[];
extern int SyncRequestDevsTop;
extern struct BufferHead *HashTable[];

#endif
