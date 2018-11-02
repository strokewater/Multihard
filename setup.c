#include "setup.h"
#include "config.h"
#include "printk.h"
#include "gfs/minode.h"
#include "gfs/gfs.h"
#include "gfs/gfslevel.h"
#include "gfs/fcntl.h"
#include "gfs/gfs_utils.h"
#include "drivers/drivers.h"
#include "drivers/ata/ata.h"
#include "drivers/ramdisk/ramdisk.h"
#include "drivers/terminal/terminal.h"
#include "drivers/v86/v86bios.h"

static struct MInode FalseRoot;

int SysSetup()
{
	static int callable = 1;
	struct MinorDevice *md;
	
	if (!callable)
		return 0;
		
	DriverInit();
	debug_printk("[SETUP] Driver Init Finish!");
	AtaInit();
	debug_printk("[SETUP] Ata Init Finish!");
	TerminalInit();
	debug_printk("[SETUP] Terminal Init Finish!");
	RAMDiskInit();
	GFSInit();
	GFSMounted(RootDevNo, &FalseRoot, MF_DEFAULT, GetFSTypeNo(RootFSName));
	md = GetMinorDevice(RootDevNo);
	Current->root = md->root;
	Current->pwd = Current->root;
	Current->root->count += 2;
	debug_printk("[SETUP] FS Init Finish!");
	return 1;
}
