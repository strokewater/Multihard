#ifndef DRIVERS_ATA_ATA_HD_H
#define DRIVERS_ATA_ATA_HD_H

#include "types.h"

#define PT_BOOT_ID_NO			0

#define PT_SYS_TYPE_EMPTY		0
#define PT_SYS_TYPE_EXTEND		5

#define ATA_HD_MAX_ERR_PER_RW	7

struct PartitionTable
{
	u8 boot_ind;
	u8 head;
	u8 sector_cyl;
	u8 cyl;
	u8 sys_ind;
	u8 end_head;
	u8 end_sector;
	u8 end_cyl;
	u32 start_sect;
	u32 nr_sects;
};

void AtaHDLoad(int AtaDevNo);
void AtaHDUnLoad();
#endif
