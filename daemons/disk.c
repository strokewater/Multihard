#include "disk.h"
#include "stdlib.h"
#include "printk.h"

void DoSyncBuffer(dev_no dev)
{
	int i;
	struct BufferHead *p;
	for (i = 0; i < NR_HASH; ++i)
	{
		if (HashTable[i] == NULL)
			continue;
		for (p = HashTable[i]; p != NULL; p = p->next)
		{
			if (p->dev_no == 0)
				continue;
			if (dev != 0 && p->dev_no != dev)
				continue;
				
			if (p->data != NULL && p->dirt && p->lock == 0)
			{
				WriteBufferToDisk(p);
				debug_printk("[DAEMON] disk: write buffer(dev_no = %x, block_no = %d) back.", p->dev_no, p->block_no);
				p->uptodate = 1;
				p->dirt = 0;
			}
		}
	}
}

int _start(int argc, char *argv[], char *envps[])
{
	int count = 0;
	while (1)
	{
		if (SyncRequestDevsTop != -1)
		{
			// Sync Request.
			SyncMInodes(SyncRequestDevs[SyncRequestDevsTop]);
			DoSyncBuffer(SyncRequestDevs[SyncRequestDevsTop]);
			SyncRequestDevsTop--;
		} else if(count == 100)
		{
			SyncMInodes(0);
			DoSyncBuffer(0);
			count = -1;
		}
		++count;
		Schedule();
	}
}
