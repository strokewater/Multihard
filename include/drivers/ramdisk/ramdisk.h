#ifndef RAMDISK_H
#define RAMDISK_H

#include "config.h"

#define RAMDISK_BLK_SIZE	1024
#define MAX_RAMDISK_NUM		5
void RAMDiskInit();
extern struct CMDLineOption RAMDiskCMDLineOptions[];

#endif
