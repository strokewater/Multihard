#ifndef DRIVERS_DRIVERS_H
#define DRIVERS_DRIVERS_H

#include "types.h"
#include "sched.h"
#include "drivers/request.h"
#include "gfs/select.h"
#include "gfs/gfs.h"


#define NR_MAX_MAJOR_DEVICES		255
#define NR_MAX_MINOR_DEVICES		255

#define DEV_STATUS_INITED			1
#define DEV_STATUS_UNINIT			0

#define NCHARS_DEVICE_NAME			16

#define DEV_TYPE_MAJOR_DEV			0
#define DEV_TYPE_CHAR				1
#define DEV_TYPE_BLOCK				2



#define MAJOR_DEV_NO_UNDEF			0
#define MAJOR_DEV_NO_MEMORY			1
#define MAJOR_DEV_NO_ATA			2
#define MAJOR_DEV_NO_RAMDISK		3
#define MAJOR_DEV_NO_TTYX			4
#define MAJOR_DEV_NO_TTY			5
#define MAJOR_DEV_NO_BUS			50
#define MINOR_DEV_NO_ATA			0

#define FREE_DEV_NO					250

// dev_no[0-15]:minor no, dev_no[16:31]: major no.
#define DEV_NO_GET_MAJOR_NO(dno)	((dno) >> 8)
#define DEV_NO_GET_MINOR_NO(dno)	((dno) & 0xff)
#define DEV_NO_MAKE(major, minor)	(((major) << 8) | (minor))

// Syscalls about driver in syscall.h
#define DSUCCESS					0
#define DERR_ILLEGAL_DEV_NO			1
#define DERR_NULL_PTR				2
#define DERR_UNCLEAN_DEV_REQ_GROUP	3

#define DERR_NORMAL_MAX			1000

struct MinorDevice
{
	dev_no	minor;
	int	status;
	char  name[NCHARS_DEVICE_NAME+1];
	
	struct
	{
		s8 is_drg_needed;
		s8 is_owner;
	}drg_proprity;
	struct DevRequestGroup *drg;
		
	int	type;	// macro: DEV_TYPE_
	
	// block device settings.
	size_t	block_size;
	size_t fs_blk_size;
	int	fs_type;
	struct MInode *root;
	
	int (*DInit)(dev_no dev);
	int (*DOpen)(dev_no dev, struct File *fp, int flags);
	int (*DClose)(dev_no dev, struct File *fp);
	ssize_t (*DRead)(dev_no dev, void *dest, off_t offset, size_t n);
	ssize_t (*DWrite)(dev_no dev, const void *src, off_t offset, size_t n);
	int (*DUnInit)(dev_no dev);
	int (*DIoctl)(dev_no dev, int cmd, unsigned int arg);
	int (*DSelect)(dev_no dev, enum SelectCheckType check_type, select_table *tbl);
};

struct MajorDevice
{
	dev_no	major;
	int	status;
	char  name[NCHARS_DEVICE_NAME+1];
	
	struct MinorDevice *minors[NR_MAX_MINOR_DEVICES];
};

int RegisterMajorDevice(struct MajorDevice *dev);
int CancelMajorDevice(dev_no dev);
int RegisterMinorDevice(dev_no dev_full_no, struct MinorDevice *dev);
int CancelMinorDevice(dev_no dev_full_no);

int DInit(dev_no dev_full_no);
int DUnInit(dev_no dev_full_no);
int DOpen(dev_no dev, struct File *fp, int flags);
int DClose(dev_no dev, struct File *fp);
ssize_t DRead(dev_no dev, void *dest, off_t offset, size_t n);
ssize_t DWrite(dev_no dev, void *src, off_t offset, size_t n);
int DIoctl(dev_no dev, int cmd, unsigned int arg);
int DSelect(dev_no dev, enum SelectCheckType check_type, select_table *tbl);

struct MinorDevice *GetMinorDevice(dev_no dev);

void DriverInit();

#endif
