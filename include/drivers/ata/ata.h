#ifndef DRIVERS_ATA_H
#define DRIVERS_ATA_H

#include "types.h"
#include "gfs/select.h"

#define NR_ATA_DEVS				4
#define NR_MAX_PT_PER_ATA_DEV			20

#define SECTOR_SIZE				512
#define DEV_NO_MINOR_ATA_CTRLER	0

#define PRIMARY_PDATA				0x1f0
#define PRIMARY_PERROR				0x1f1
#define PRIMARY_PFEATURES			0x1f1
#define PRIMARY_PSEC_COUNT			0x1f2
#define PRIMARY_PLBA_LOW			0x1f3
#define PRIMARY_PLBA_MID			0x1f4
#define PRIMARY_PLBA_HIGH			0x1f5
#define PRIMARY_PDEVICE				0x1f6
#define PRIMARY_PSTATUS				0x1f7
#define PRIMARY_PCOMMAND			0x1f7
#define PRIMARY_PALTER_STATUS		0x3f6
#define PRIMARY_PDEVICE_CTRL		0x3f6

#define SECONDARY_PDATA				0x170
#define SECONDARY_PERROR			0x171
#define SECONDARY_PFEATURES			0x171
#define SECONDARY_PSEC_COUNT		0x172
#define SECONDARY_PLBA_LOW			0x173
#define SECONDARY_PLBA_MID			0x174
#define SECONDARY_PLBA_HIGH			0x175
#define SECONDARY_PDEVICE			0x176
#define SECONDARY_PSTATUS			0x177
#define SECONDARY_PCOMMAND			0x177
#define SECONDARY_PALTER_STATUS		0x376
#define SECONDARY_PDEVICE_CTRL		0x376

#define ATA_CABLE_TYPE_UNDEF		0
#define ATA_CABLE_TYPE_MASTER		1
#define ATA_CABLE_TYPE_SLAVE		2

#define NR_ATA_MEDIA_TYPES			2
#define ATA_MEDIA_TPYE_UNKNOWN	-1
#define ATA_MEDIA_TYPE_HD			0
#define ATA_MEDIA_TYPE_CDROM		1

#define ATA_STATUS_DRQ			8
#define ATA_STATUS_DF_SE		32
#define ATA_STATUS_DRDY			64
#define ATA_STATUS_BSY			128
#define ATA_STATUS_ERR			1

#define ATA_CMD_IDENTIFY			0xec
#define ATA_CMD_READ_SECTORS		0x20
#define ATA_CMD_WRITE_SECTORS		0x30

#define ATAPI_CMD_IDENTIFY			0xa1
#define ATAPI_CMD					0xa0

#define DERR_ATA_OUT_OF_RANGE		1001
#define DERR_ATA_CONTROL_TIME_OUT	1002

#define ATA_IDENTIFY_SIZE				512

#define ATA_IDENTIFY_GEN_CONFIG			0
#define ATA_IDENTIFY_GEN_CONFIG_IS_ATA	15
#define ATA_IDENTIFY_IS_ATA(id)			(!(*((s16 *)id + ATA_IDENTIFY_GEN_CONFIG) & (1 << ATA_IDENTIFY_GEN_CONFIG_IS_ATA)))

#define ATA_IDENTIFY_OFFSET_NR_SECS		(60 * 2)


struct AtaDevice
{
	u8 *AtaID;
	s8 MediaType;
	s8 CableType;
	
	u32 PData;
	u32 PError;
	u32 PFeatures;
	u32 PSecCount;
	u32 PLBALow;
	u32 PLBAMid;
	u32 PLBAHigh;
	u32 PDevice;
	u32 PStatus;
	u32 PCommand;
	u32 PAlterStatus;
	u32 PDeviceCtrl;
};

struct AtaDeviceReg
{
	s8 LBAOrHead: 4;
	s8 Drv:1;
	s8 Reserved1:1;
	s8 LBAOrCHS:1;
	s8 Reserved2:1;
};

#define AtaNoToDevNo(ata_no)		(DEV_NO_MAKE(MAJOR_DEV_NO_ATA, (ata_no) * NR_MAX_PT_PER_ATA_DEV))
#define DevNoToAtaNo(no)			(DEV_NO_GET_MINOR_NO(no) / NR_MAX_PT_PER_ATA_DEV)
#define IsPrimaryDev(AtaNo)			((AtaNo) == 0 || (AtaNo) == 1)
#define IsMasterDev(AtaNo)			((AtaNo) == 0 || (AtaNo) == 2)

#define LBA_FIRST_PART(lba)				((lba) >> 24)
#define LBA_SECOND_PART(lba)			(((lba) >> 16) & 0xff)
#define LBA_THIRD_PART(lba)				(((lba)  >> 8) & 0xff)
#define LBA_FOURTH_PART(lba)			(((lba) & 0xff))

extern struct AtaDevice AtaDevs[];
extern int CurAtaDevNo;
void CaseAtaDev(int AtaDevNo);
/*
 * ATA device: absoulte of all ata device.
 *  when setuping, it'll setup ata devices such as hds、cdroms and so on.
 *   unsetuping is also.
 *  read this device will get specialed ATA device information(struct AtaDevice), offset means ATA device no,
 *   n is ignored.
 * 
 *  As for ATA device no:
 * 	  Primary Master: 0, Primary Slave: 1, Secondary Master:2, Secondary Slave: 3
 */
 
 /* ATA minor no(In AtaDevNo):
  *  [0-19]: 0, [10-29]:1, [30-49]:2, [50-69]:3
  */
  
void AtaDevWaitIRQ(int AtaDevNo);
void AtaInit();
int AtaDSelect(dev_no dev, enum SelectCheckType check_type, select_table *tbl);

#endif
