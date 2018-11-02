#include "drivers/ramdisk/ramdisk.h"
#include "drivers/drivers.h"
#include "config.h"
#include "mm/varea.h"
#include "mm/darea.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"
#include "gfs/gfs.h"

static s32 **RAMDiskPhyAddr;
static struct MajorDevice RAMDiskMajorDev = {.major = MAJOR_DEV_NO_RAMDISK, .status = DEV_STATUS_UNINIT};

static int RDInit(dev_no dev);
static int RDUnInit(dev_no dev);
static ssize_t RDRead(dev_no dev, void *dest, off_t offset, size_t n);
static ssize_t RDWrite(dev_no dev, const void *src, off_t offset, size_t n);
static int RDIoctl(dev_no dev, int cmd, unsigned int arg);
static int RDSelect(dev_no dev, enum SelectCheckType check_type, select_table *tbl);
static int RDOpen(dev_no dev, struct File *fp, int flags);
static int RDClose(dev_no dev, struct File *fp);

static int SetRAMDiskSize(void);

struct CMDLineOption RAMDiskCMDLineOptions[] = 
{
	{"--ramdisk-sizek", SetRAMDiskSize},
	{NULL, NULL}
};

size_t RAMDiskCount;
size_t RAMDiskSizeByK[MAX_RAMDISK_NUM];

static int SetRAMDiskSize(void)
{
	if (NRCMDLineOptionPara == 0)
		return CMDLINE_OPTION_INT_SEQU;
	else
	{
		for (int i = 0; i < NRCMDLineOptionPara; ++i)
			RAMDiskSizeByK[i] = CMDLineOptionParaInt[i];
		RAMDiskCount = NRCMDLineOptionPara;
		return CMDLINE_OPTION_FINISH;
	}

	return -1;
}

void RAMDiskInit()
{
	int i;
	struct MinorDevice *md;

	if (RAMDiskCount == 0)
		return;
	
	RAMDiskPhyAddr = KMalloc(RAMDiskCount * sizeof(s32 *));
	assert(RAMDiskPhyAddr, "RAMDiskInit: kmalloc failed.");
	
	strncpy(RAMDiskMajorDev.name, "ramdisk", NCHARS_DEVICE_NAME);
	RegisterMajorDevice(&RAMDiskMajorDev);
	
	for (i = 0; i < RAMDiskCount; ++i)
	{
		RAMDiskPhyAddr[i] = DMalloc(RAMDiskSizeByK[i] * 1024);
		assert(RAMDiskPhyAddr[i], "RAMDiskInit: dmalloc failed.");
		
		md = KMalloc(sizeof(*md));
		assert(md, "RAMDiskInit: KMalloc failed.");
		md->minor = i;
		md->status = DEV_STATUS_INITED;
		strncpy(md->name, "rmx", NCHARS_DEVICE_NAME);
		md->name[2] = '0' + i;
		md->drg_proprity.is_owner = 1;
		md->drg_proprity.is_drg_needed = 0;
		md->drg = NULL;
		md->type = DEV_TYPE_BLOCK;
		md->block_size = RAMDISK_BLK_SIZE;
		md->fs_type = -1;
		md->root = NULL;
		
		md->DInit = RDInit;
		md->DRead = RDRead;
		md->DWrite = RDWrite;
		md->DUnInit = RDUnInit;
		md->DIoctl = RDIoctl;
		md->DSelect = RDSelect;
		md->DOpen = RDOpen;
		md->DClose = RDClose;
		
		RegisterMinorDevice(DEV_NO_MAKE(MAJOR_DEV_NO_RAMDISK, i), md);
	}
}

static int RDInit(dev_no dev)
{
	struct MinorDevice *md;
	dev_no minor_dev_no;
	void *pa;
	
	md = GetMinorDevice(dev);
		
	minor_dev_no = DEV_NO_GET_MINOR_NO(md->minor);
	if (minor_dev_no < 0 || minor_dev_no >= RAMDiskCount)
		return 0;
	
	pa = DMalloc(RAMDiskSizeByK[minor_dev_no] * 1024);
	if (pa == NULL)
		return 0;
	RAMDiskPhyAddr[minor_dev_no] = pa;
	md->status = DEV_STATUS_INITED;
	return 1;
}

static int RDUnInit(dev_no dev)
{
	struct MinorDevice *md;
	int i;
	
	md = GetMinorDevice(dev);
	i = DEV_NO_GET_MINOR_NO(md->minor);
	DFree(RAMDiskPhyAddr[i], RAMDiskSizeByK[i] * 1024);
	RAMDiskPhyAddr[i] = NULL;
	md->status = DEV_STATUS_UNINIT;
	
	return 1;
}

static ssize_t RDRead(dev_no dev, void *dest, off_t offset, size_t n)
{
	struct MinorDevice *md;
	dev_no i;
	ssize_t ret;
	
	md = GetMinorDevice(dev);
	// check major and minor dev no.
	i = DEV_NO_GET_MINOR_NO(md->minor);
	ret = (s32)memcpy(dest, RAMDiskPhyAddr[i], n * md->block_size);
	
	return ret / md->block_size;
}

static ssize_t RDWrite(dev_no dev, const void *src, off_t offset, size_t n)
{
	struct MinorDevice *md;
	dev_no i;
	ssize_t ret;
	
	md = GetMinorDevice(dev);
	// check major and minor dev no.
	i = DEV_NO_GET_MINOR_NO(md->minor);
	ret = (s32)memcpy(RAMDiskPhyAddr[i], src, n * md->block_size);
	
	return ret / md->block_size;
}

static int RDIoctl(dev_no dev, int cmd, unsigned int arg)
{
	return 0;
}

static int RDSelect(dev_no dev, enum SelectCheckType check_type, select_table *tbl)
{
	return 1;
}

static int RDOpen(dev_no dev, struct File *fp, int flags)
{
	return 0;
}

static int RDClose(dev_no dev, struct File *fp)
{
	return 0;
}
