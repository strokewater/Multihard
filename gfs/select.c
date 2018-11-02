#include "process.h"
#include "sched.h"
#include "limits.h"
#include "printk.h"
#include "stdlib.h"
#include "exception.h"
#include "time.h"
#include "errno.h"
#include "mm/varea.h"
#include "asm/int_content.h"
#include "gfs/select.h"
#include "gfs/stat.h"
#include "gfs/pipe.h"
#include "drivers/drivers.h"

void add_wait(struct Process ** wait_address, select_table * p)
{
	int i;

	if (!wait_address)
		return;
	for (i = 0 ; i < p->nr ; i++)
		if (p->entry[i].wait_address == wait_address)
			return;
	p->entry[p->nr].wait_address = wait_address;
	p->entry[p->nr].old_task = * wait_address;
	*wait_address = Current;
	p->nr++;
}

void free_wait(select_table * p)
{
	int i;
	struct Process ** tpp;

	for (i = 0; i < p->nr ; i++) {
		tpp = p->entry[i].wait_address;
		while (*tpp && *tpp != Current) {
			(*tpp)->state = 0;
			Current->state = PROCESS_STATE_UNINTERRUPTIBLE;
			Schedule();
		}
		if (!*tpp)
			printk("free_wait: NULL");
		if ((*tpp = p->entry[i].old_task) != NULL)
			(**tpp).state = 0;
	}
	p->nr = 0;
}

/*
 * The check_XX functions check out a file. We know it's either
 * a pipe, a character device or a fifo (fifo's not implemented)
 */
static int check_read(select_table *tbl, struct File * fp)
{
	struct MInode *inode = fp->inode;
	if (inode->pipe)
	{
		if (!PipeEmpty(fp->inode))
			return 1;
		else
			add_wait(&(fp->inode->wait_process), tbl);
	} else if(S_ISCHR(inode->mode) || S_ISBLK(inode->mode))
		return DSelect(fp->inode->zone[0], SELECT_READ, tbl);
		
	return 0;
}

static int check_write(select_table * tbl, struct File * fp)
{
	struct MInode *inode = fp->inode;
	if (inode->pipe)
	{
		if (!PipeFull(fp->inode))
			return 1;
		else
			add_wait(&(fp->inode->wait_process), tbl);
	} else if(S_ISCHR(inode->mode) || S_ISBLK(inode->mode))
		return DSelect(fp->inode->zone[0], SELECT_WRITE, tbl);
		
	return 0;
}

static int check_ex(select_table * tbl, struct File * fp)
{
	struct MInode *inode = fp->inode;
	if(S_ISCHR(inode->mode) || S_ISBLK(inode->mode))
		return DSelect(fp->inode->zone[0], SELECT_EXCEPT, tbl);
	else if (inode->pipe)
	{
		if (inode->count < 2)
			return 1;
		else
			add_wait(&(fp->inode->wait_process), tbl);	
	}
	return 0;
}

struct SelectTimeOutInfo
{
	struct Process *p;
	int flag;
};

void SelectTimeOutAct(void *p)
{
	struct SelectTimeOutInfo *info = (struct SelectTimeOutInfo *)p;
	info->flag = 1;
	if (info->p->state == PROCESS_STATE_INTERRUPTIBLE)
		info->p->state = PROCESS_STATE_RUNNING;
}

int SysSelect(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *tvp)
{
	int ret_val = 0;
	select_table wait_table;

	int timeout = 0;
	struct Timer *timeout_timer = NULL;
	struct SelectTimeOutInfo *info = NULL;
	
	wait_table.nr = 0;
	
	if (tvp && tvp->tv_usec != 0 && tvp->tv_sec != 0) {
		timeout = tvp->tv_usec / (1000000/SCHED_FREQ);
		timeout += tvp->tv_sec * SCHED_FREQ;

		info = KMalloc(sizeof(*info));
		info->flag = 0;
		info->p = Current;
		timeout_timer = TimerCreate(timeout, SelectTimeOutAct, info);

	}

	int count;
	fd_set out_readfds;
	fd_set out_writefds;
	fd_set out_exceptfds;
repeat:
	if (Current->signal & ~Current->blocked)
	{
		ret_val = -EINTR;
		goto ret;
	}
	if (info != NULL && info->flag == 1)
	{
		// 如果设置了定时，并且已经到时了
		ret_val = count;
		goto ret;
	}

	count = 0;
	out_readfds = out_writefds = out_exceptfds = 0;
	
	for (int i = 0; i < (nfds < NR_OPEN ? nfds : NR_OPEN); ++i)
	{
		if (readfds != NULL && (*readfds & (1 << i)))
		{
			if (check_read(&wait_table, Current->filep[i])) {
				out_readfds |= (1 << i);
				count++;
			}
		}
		if (writefds != NULL && (*writefds & (1 << i)))
		{
			if (check_write(&wait_table, Current->filep[i])) {
				out_writefds |= (1 << i);
				count++;
			}
		}
		if (exceptfds != NULL && (*exceptfds & (1 << i)))
		{
			if (check_ex(&wait_table, Current->filep[i])) {
				out_exceptfds |= (1 << i);
				count++;
			}
		}
	}

	if (count == 0)
	{
		if (tvp && tvp->tv_usec == 0 && tvp->tv_sec == 0)
		{
			// 不等待，直接返回
			ret_val = 0;
			goto ret;
		}
		Current->state = PROCESS_STATE_INTERRUPTIBLE;
		Schedule();
		free_wait(&wait_table);
		goto repeat;
	} else
		ret_val = count;


ret:
	if (info != NULL)
	{
		if (info->flag == 0)
			TimerDelete(timeout_timer);
		KFree(info, sizeof(*info));
	}

	free_wait(&wait_table);
	if (ret_val >= 0)
	{
		*readfds = out_readfds;
		*writefds = out_writefds;
		*exceptfds = out_exceptfds;
		return ret_val;
	} else
		return ret_val;
}
