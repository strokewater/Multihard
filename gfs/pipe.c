#include "errno.h"
#include "types.h"
#include "sched.h"
#include "string.h"
#include "gfs/gfs.h"

int PipeEmpty(struct MInode *mi)
{
	return mi->zone[0] == mi->zone[1];
}

int PipeFull(struct MInode *mi)
{
	// 0:write pointer, 1:read pointer
	return ((mi->zone[0] + 1) & (PAGE_SIZE - 1)) == mi->zone[1];
}

ssize_t GFSReadPipe(struct File *fp, void *buf, size_t count)
{
	size_t nchars  = 0;
	struct MInode *mi = fp->inode;
	size_t tmp_count = count;
	
	while (count > 0)
	{
		while (PipeEmpty(mi))
		{
			WakeUp(&(mi->wait_process));
			if (mi->count != 2)
				return tmp_count - count;
			Sleep(&(mi->wait_process), SLEEP_TYPE_UNINTABLE);
		}
		
		if (mi->zone[0] > mi->zone[1])
		{
			nchars = mi->zone[0] - mi->zone[1];
			if (nchars > count)
				nchars = count;
			memcpy(buf, (char *)mi->size + mi->zone[1], nchars);
			buf += nchars;
			count -= nchars;
			mi->zone[1] += nchars;
		}else
		{
			nchars = PAGE_SIZE - 1 - mi->zone[1];
			if (nchars > count)
				nchars = count;
			memcpy(buf, (char *)mi->size + mi->zone[1], nchars);
			buf += nchars;
			count -= nchars;
			mi->zone[1] += nchars;
			if (mi->zone[1] >= PAGE_SIZE)
				mi->zone[1] -= PAGE_SIZE;
			
			nchars = mi->zone[0];
			if (nchars > count)
				nchars = count;
			memcpy(buf, (char *)mi->size, nchars);

			buf += nchars;
			count -= nchars;
			mi->zone[1] += nchars;
		}	
		WakeUp(&(mi->wait_process));
		
	}
	return tmp_count - count;
}

ssize_t GFSWritePipe(struct File *fp, void *buf, size_t count)
{
	size_t nchars  = 0;
	struct MInode *mi = fp->inode;
	size_t tmp_count = count;
	
	while (count > 0)
	{
		while (PipeFull(mi))
		{
			WakeUp(&(mi->wait_process));
			if (mi->count != 2)
			{
				Current->signal |= (1 << (SIGPIPE - 1));
				return (tmp_count - count) ? tmp_count - count : -1;
			}
			Sleep(&(mi->wait_process), SLEEP_TYPE_UNINTABLE);
		}
		
		if (mi->zone[0] < mi->zone[1])
		{
			nchars = mi->zone[1] - mi->zone[0];
			if (nchars > count)
				nchars = count;
			memcpy(buf, (char *)mi->size + mi->zone[1], nchars);
			buf += nchars;
			count -= nchars;
			mi->zone[0] += nchars;
		}else
		{
			nchars = PAGE_SIZE - 1 - mi->zone[0];
			if (nchars > count)
				nchars = count;
			memcpy((char *)mi->size + mi->zone[0], buf, nchars);
			buf += nchars;
			count -= nchars;
			mi->zone[0] += nchars;
			if (mi->zone[0] >= PAGE_SIZE)
				mi->zone[0] -= PAGE_SIZE;
			
			nchars = mi->zone[1] - mi->zone[0];
			if (nchars > count)
				nchars = count;
			memcpy((char *)mi->size, buf, nchars);

			buf += nchars;
			count -= nchars;
			mi->zone[0] += nchars;
		}	
		WakeUp(&(mi->wait_process));
	}
	return tmp_count - count;
}
