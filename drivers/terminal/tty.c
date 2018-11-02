#include "sched.h"
#include "errno.h"
#include "gfs/select.h"
#include "drivers/terminal/tty.h"
#include "drivers/drivers.h"

static int DTTYInit(dev_no dev)
{
	return 0;
}

static int DTTYUnInit(dev_no dev)
{
	return 0;
}

static int DTTYOpen(dev_no dev, struct File *fp, int flags)
{
	if (Current->tty < 0)
		return -EPERM;
	
	return 0;
}

static int DTTYClose(dev_no dev, struct File *fp)
{
	return 0;
}

static ssize_t DTTYRead(dev_no dev, void *dest, off_t offset, size_t size)
{
	if (DEV_NO_GET_MINOR_NO(dev) == 0)
		return DRead(TTY_NO_TO_DEV_NO(Current->tty), dest, offset, size);	// /dev/tty
	else
		return -1;
}

static ssize_t DTTYWrite(dev_no dev, const void *src, off_t offset, size_t size)
{
	if (DEV_NO_GET_MINOR_NO(dev) == 0)
		return DWrite(TTY_NO_TO_DEV_NO(Current->tty), src, offset, size);	// /dev/tty
	else
		return -1;
}

static int DTTYIoctl(dev_no dev, int cmd, unsigned int arg)
{
	if (DEV_NO_GET_MINOR_NO(dev) == 0)
		return DIoctl(TTY_NO_TO_DEV_NO(Current->tty), cmd, arg);	// /dev/tty
	else
		return -1;
}

static int DTTYSelect(dev_no dev, enum SelectCheckType check_type, select_table *tbl)
{
	return DSelect(dev, check_type, tbl);
}

struct MajorDevice tty_major = {.major = MAJOR_DEV_NO_TTY, .status = DEV_STATUS_UNINIT,
								.name = "tty"};
struct MinorDevice tty_minor_dev = { .minor = 0, .status = DEV_STATUS_UNINIT,
								.name = "tty", .type = DEV_TYPE_CHAR,
								.DInit = DTTYInit, .DRead = DTTYRead,
								.DWrite = DTTYWrite, .DUnInit = DTTYUnInit,
								.DIoctl = DTTYIoctl, .DSelect = DTTYSelect,
								.DOpen = DTTYOpen, .DClose = DTTYClose
};


void TTYInit()
{
	RegisterMajorDevice(&tty_major);
	RegisterMinorDevice(DEV_NO_MAKE(MAJOR_DEV_NO_TTY, 0), &tty_minor_dev);
	DInit(DEV_NO_MAKE(MAJOR_DEV_NO_TTY, 0));
}
