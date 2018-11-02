#include "stdlib.h"
#include "assert.h"
#include "types.h"
#include "stdarg.h"
#include "exception.h"
#include "sched.h"
#include "drivers/drivers.h"
#include "mm/varea.h"

static struct MajorDevice *MajorDevicesGroup[NR_MAX_MAJOR_DEVICES];
static struct MajorDevice FreeDev = {.major = DEV_NO_GET_MAJOR_NO(FREE_DEV_NO), .status = DEV_STATUS_UNINIT, .name = "AllocDev"};

void DriverInit()
{
	MajorDevicesGroup[FREE_DEV_NO] = &FreeDev;
}

int RegisterMajorDevice(struct MajorDevice *mdev)
{
	struct MajorDevice *p;
	
	assert(mdev, "RegisterMajorDevice: NULL Pointer(dev).");
	assert(mdev->major >= 0 && mdev->major < NR_MAX_MAJOR_DEVICES, "RegisterMajorDevice: illegal major no.");
	assert(MajorDevicesGroup[mdev->major] == NULL, "RegisterMajorDevice: dev no is occupied.");
	
	p = KMalloc(sizeof(*p));
	assert(p, "RegisterMajorDevice:KMalloc failed.");
	
	*p = *mdev;
	MajorDevicesGroup[mdev->major] = p;
	return 0;
}

int CancelMajorDevice(dev_no dev)
{
	dev_no major = DEV_NO_GET_MAJOR_NO(dev);
	
	assert(major >= 0 || major < NR_MAX_MAJOR_DEVICES, "CancelMajorDevice: illegal major no.");
	assert(major != DEV_NO_GET_MAJOR_NO(dev), "CancelMajorDevice: unable to cancel alloc dev.");
	
	// together cancel all minor device.
	
	KFree(MajorDevicesGroup[major], 0);
	MajorDevicesGroup[major] = NULL;
	return 0;
}

int RegisterMinorDevice(dev_no dev_full_no, struct MinorDevice *dev)
{
	struct MinorDevice *p;
	dev_no major = DEV_NO_GET_MAJOR_NO(dev_full_no);
		
	assert(dev, "RegisterMinorDevice: NULL Pointer(dev).");
	assert(major >= 0 && major < NR_MAX_MAJOR_DEVICES, "RegisterMinorDevice: illegal minor no.");
	assert(dev->minor >= 0 && dev->minor <NR_MAX_MAJOR_DEVICES, "RegisterMinorDevice: illegal minor no.");
	assert(MajorDevicesGroup[DEV_NO_GET_MAJOR_NO(dev_full_no)], "RegisterMinorDevice: dev no is occupied.");
	
	p = KMalloc(sizeof(*p));
	assert(p, "RegisterMinorDevice: KMalloc failed.");
	*p = *dev;
	
	if (p->drg_proprity.is_drg_needed && p->drg_proprity.is_owner)
		// need and use only itself.
		p->drg = BuildDevRequestGroup();
	
	MajorDevicesGroup[DEV_NO_GET_MAJOR_NO(dev_full_no)]->minors[dev->minor] = p;
	
	return 0;
}

int CancelMinorDevice(dev_no dev_full_no)
{
	dev_no major, minor;
	struct MajorDevice *i;
	struct MinorDevice *j;
	
	major = DEV_NO_GET_MAJOR_NO(dev_full_no);
	minor = DEV_NO_GET_MINOR_NO(dev_full_no);
	
	assert(major >= 0 && major < NR_MAX_MAJOR_DEVICES, "CancelMinorDevice: illegal major no.");
	assert(minor >= 0 && minor < NR_MAX_MINOR_DEVICES, "CancelMinorDevice: illegal minor no.");

	i = MajorDevicesGroup[major];
	assert(i, "CancelMinorDevice: illegal major no.");
	
	j = i->minors[minor];
	assert(j->status == DEV_STATUS_UNINIT, "CancelMinorDevice: dev closed.");
	
	if (j->drg_proprity.is_drg_needed && j->drg_proprity.is_owner)
	{
		CleanDevRequestGroup(j->drg);
		FreeDevRequestGroup(j->drg);
	}
	KFree(j, 0);
	i->minors[minor] = NULL;
	return 9;
}

int DInit(dev_no dev_full_no)
{
	dev_no major, minor;
	int ret;
	struct MajorDevice *i;
	struct MinorDevice *j;
	
	major = DEV_NO_GET_MAJOR_NO(dev_full_no);
	minor = DEV_NO_GET_MINOR_NO(dev_full_no);
	
	if (major < 0 || major >= NR_MAX_MAJOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	if (minor < 0 || minor >= NR_MAX_MINOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	i = MajorDevicesGroup[major];
	if (i == NULL)
		return -DERR_ILLEGAL_DEV_NO;
	j = i->minors[minor];
	if (j->status == DEV_STATUS_INITED)
		return -DERR_ILLEGAL_DEV_NO;
	if (j->drg_proprity.is_drg_needed && !IsDevRequestGroupEmpty(j->drg))
		return -DERR_UNCLEAN_DEV_REQ_GROUP;
	
	ret = j->DInit(dev_full_no);
	j->status = DEV_STATUS_INITED;
	return ret;
}

int DUnInit(dev_no dev_full_no)
{
	dev_no major, minor;
	struct MajorDevice *i;
	struct MinorDevice *j;
	
	major = DEV_NO_GET_MAJOR_NO(dev_full_no);
	minor = DEV_NO_GET_MINOR_NO(dev_full_no);
	
	if (major < 0 || major >= NR_MAX_MAJOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	if (minor < 0 || minor >= NR_MAX_MINOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	i = MajorDevicesGroup[major];
	if (i == NULL)
		return -DERR_ILLEGAL_DEV_NO;
	j = i->minors[minor];
	if (j->status == DEV_STATUS_UNINIT)
		return -DERR_ILLEGAL_DEV_NO;
	
	if (j->drg)
		CleanDevRequestGroup(j->drg);
	j->status = DEV_STATUS_UNINIT;
	return j->DUnInit(dev_full_no);
}

// users call.

ssize_t DRead(dev_no dev, void *dest, off_t offset, size_t n)
{
	dev_no major, minor;
	struct MajorDevice *i;
	struct MinorDevice *j;
	
	major = DEV_NO_GET_MAJOR_NO(dev);
	minor = DEV_NO_GET_MINOR_NO(dev);
	
	if (major < 0 || major >= NR_MAX_MAJOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	if (minor < 0 || minor >= NR_MAX_MINOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	i = MajorDevicesGroup[major];
	if (i == NULL)
		return -DERR_ILLEGAL_DEV_NO;
	j = i->minors[minor];
	if (j->status == DEV_STATUS_UNINIT)
		return -DERR_ILLEGAL_DEV_NO;
	if (dest == NULL)
		return -DERR_NULL_PTR;

	return j->DRead(dev, dest, offset, n);
}

ssize_t DWrite(dev_no dev, void *src, off_t offset, size_t n)
{

	dev_no major, minor;
	struct MajorDevice *i;
	struct MinorDevice *j;
	
	major = DEV_NO_GET_MAJOR_NO(dev);
	minor = DEV_NO_GET_MINOR_NO(dev);
	
	if (major < 0 || major >= NR_MAX_MAJOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	if (minor < 0 || minor >= NR_MAX_MINOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	if (src == NULL)
		return -DERR_NULL_PTR;
	i = MajorDevicesGroup[major];
	if (i == NULL)
		return -DERR_ILLEGAL_DEV_NO;
	j = i->minors[minor];
	if (j->status == DEV_STATUS_UNINIT)
		return -DERR_ILLEGAL_DEV_NO;
		
	return j->DWrite(dev, src, offset, n);
}

int DIoctl(dev_no dev, int cmd, unsigned int arg)
{
	dev_no major, minor;
	struct MajorDevice *i;
	struct MinorDevice *j;
	
	major = DEV_NO_GET_MAJOR_NO(dev);
	minor = DEV_NO_GET_MINOR_NO(dev);
	
	if (major < 0 || major >= NR_MAX_MAJOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	if (minor < 0 || minor >= NR_MAX_MINOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	i = MajorDevicesGroup[major];
	if (i == NULL)
		return -DERR_ILLEGAL_DEV_NO;
	j = i->minors[minor];
	if (j->status == DEV_STATUS_UNINIT)
		return -DERR_ILLEGAL_DEV_NO;	
	return j->DIoctl(dev, cmd, arg);
}


int DSelect(dev_no dev, enum SelectCheckType check_type, select_table *tbl)
{
	dev_no major, minor;
	struct MajorDevice *i;
	struct MinorDevice *j;
	
	major = DEV_NO_GET_MAJOR_NO(dev);
	minor = DEV_NO_GET_MINOR_NO(dev);
	
	if (major < 0 || major >= NR_MAX_MAJOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	if (minor < 0 || minor >= NR_MAX_MINOR_DEVICES)
		return -DERR_ILLEGAL_DEV_NO;
	i = MajorDevicesGroup[major];
	if (i == NULL)
		return -DERR_ILLEGAL_DEV_NO;
	j = i->minors[minor];
	if (j->status == DEV_STATUS_UNINIT)
		return -DERR_ILLEGAL_DEV_NO;	
	return j->DSelect(dev, check_type, tbl);
}

int DOpen(dev_no dev, struct File *fp, int flags)
{
	if (fp == NULL)
		panic("DOpen minode == NULL.");
	
	struct MinorDevice *j = GetMinorDevice(dev);
	if (j == NULL)
		panic("DOpen MinorDevice *j == NULL.");
	if (j->status == DEV_STATUS_UNINIT)
		panic("DOpen MinorDevice *j is uninited status.");
	return j->DOpen(dev, fp, flags);
}

int DClose(dev_no dev, struct File *fp)
{
	if (fp == NULL)
		panic("DClose fp == NULL.");
	
	struct MinorDevice *j = GetMinorDevice(dev);
	if (j == NULL)
		panic("DClose MinorDevice *j == NULL.");
	if (j->status == DEV_STATUS_UNINIT)
		panic("DClose MinorDevice *j is uninited status.");
	return j->DClose(dev, fp);
}

dev_no SysDGetDevNo()
{
	dev_no minor, major;
	
	major = DEV_NO_GET_MAJOR_NO(FREE_DEV_NO);
	
	for (minor = 0; minor < NR_MAX_MINOR_DEVICES; ++minor)
	{
		if (FreeDev.minors[minor] != NULL && FreeDev.minors[minor]->status == DEV_STATUS_UNINIT)
			return DEV_NO_MAKE(major, minor);
	}
	
	return 0;
}

struct MinorDevice *GetMinorDevice(dev_no dev)
{
	dev_no major, minor;
	
	major = DEV_NO_GET_MAJOR_NO(dev);
	minor = DEV_NO_GET_MINOR_NO(dev);
	
	if (major < 0 || major >= NR_MAX_MAJOR_DEVICES)
		return NULL;
	
	return MajorDevicesGroup[major]->minors[minor];
}
