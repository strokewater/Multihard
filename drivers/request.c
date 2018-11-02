#include "assert.h"
#include "stdlib.h"
#include "mm/varea.h"
#include "drivers/request.h"
#include "drivers/drivers.h"


struct DevRequestGroup *BuildDevRequestGroup()
{
	struct DevRequestGroup *ret;
	int i;
	
	ret = KMalloc(sizeof(*ret));
	assert(ret, "KMalloc failed.");
	
	ret->head = ret->tail = 0;
	ret->wait_free = NULL;
	
	for (i = 0; i < NR_DEV_REQ_ITEMS; ++i)
		ret->items[i].dev = -1;
		
	return ret;
}

int IsDevRequestGroupEmpty(struct DevRequestGroup *p)
{
	assert(p, "IsDevRequestGroupEmpty: NULL Pointer.");
	return p->head == p->tail;
}

int IsDevRequestGroupFull(struct DevRequestGroup *p)
{
	assert(p, "IsDevRequestGroupEmpty: NULL Pointer.");
	return (p->tail + 1) % NR_DEV_REQ_ITEMS == p->head;
}

void CleanDevRequestGroup(struct DevRequestGroup *p)
{
	assert(p, "IsDevRequestGroupEmpty: NULL Pointer.");
	while (1)
	{
		p->wait_free = Current;
		Current->state = PROCESS_STATE_INTERRUPTIBLE;
		if (IsDevRequestGroupEmpty(p))
			return;
	}
}

void FreeDevRequestGroup(struct DevRequestGroup *p)
{
	assert(p, "IsDevRequestGroupEmpty: NULL Pointer.");
	KFree(p, 0);
}

int MakeDevRequest(int cmd, dev_no dev, void *data, off_t offset, size_t n)
{
	int ret;
	struct MinorDevice *md;
	
	// only used for KAssert variable.
	dev_no major, minor;
	
	major = DEV_NO_GET_MAJOR_NO(dev);
	minor = DEV_NO_GET_MINOR_NO(dev);
	
	assert(major >= 0 && major < NR_MAX_MAJOR_DEVICES, "CancelMinorDevice: illegal major no.");
	assert(minor >= 0 && minor < NR_MAX_MINOR_DEVICES, "CancelMinorDevice: illegal minor no.");
	
	md = GetMinorDevice(dev);
	
	while (IsDevRequestGroupFull(md->drg))
		Sleep(&(md->drg->wait_free), SLEEP_TYPE_UNINTABLE);
	// at now's actions, last item must be free, so we add it.
	md->drg->items[md->drg->tail].dev = dev;
	md->drg->items[md->drg->tail].cmd = cmd;
	md->drg->items[md->drg->tail].data = data;
	md->drg->items[md->drg->tail].offset = offset;
	md->drg->items[md->drg->tail].n = n;
	md->drg->items[md->drg->tail].waitting = Current;
	// printk("Request %d: offset = %d, n = %d Make. Head = %d, Head = %d, Tail = %d. \n", md->drg->tail, offset, n, md->drg->head, md->drg->tail);
	ret = md->drg->tail;
	md->drg->tail = (md->drg->tail + 1) % NR_DEV_REQ_ITEMS;
	
	return ret;
}

void WaitDevRequest(dev_no dev, int dreq_no)
{
	struct MinorDevice *md;
	md = GetMinorDevice(dev);
	while (md->drg->items[md->drg->head].waitting != Current)
	{
		Current->state = PROCESS_STATE_UNINTERRUPTIBLE;
		Schedule();
	}
}

void FinishDevRequest(dev_no dev)
{
	struct MinorDevice *md;
	
	dev_no major, minor;
	
	major = DEV_NO_GET_MAJOR_NO(dev);
	minor = DEV_NO_GET_MINOR_NO(dev);
	
	assert(major >= 0 && major < NR_MAX_MAJOR_DEVICES, "CancelMinorDevice: illegal major no.");
	assert(minor >= 0 && minor < NR_MAX_MINOR_DEVICES, "CancelMinorDevice: illegal minor no.");
	
	md = GetMinorDevice(dev);
	md->drg->items[md->drg->head].dev = -1;
	md->drg->items[md->drg->head].waitting = NULL;		// nothing, just a custom, what if wake up this.
	// printk("Request %d: offset = %d, n = %d Finish. Head = %d, Head = %d, Tail = %d. \n", md->drg->head, md->drg->items[md->drg->head].offset, md->drg->items[md->drg->head].n, md->drg->head, md->drg->tail);
	md->drg->head = (md->drg->head + 1) % NR_DEV_REQ_ITEMS;
	if (md->drg->head != md->drg->tail)
		md->drg->items[md->drg->head].waitting->state = PROCESS_STATE_RUNNING;	// not empty.
		
	WakeUp(&(md->drg->wait_free));

}
