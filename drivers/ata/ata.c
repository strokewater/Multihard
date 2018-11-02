#include "string.h"
#include "assert.h"
#include "types.h"
#include "stdlib.h"
#include "sched.h"
#include "mm/varea.h"
#include "asm/asm.h"
#include "asm/pm.h"
#include "drivers/drivers.h"
#include "drivers/i8259a/i8259a.h"
#include "drivers/ata/ata.h"
#include "drivers/ata/ata_hd.h"
#include "gfs/select.h"


static char *AtaDevName = "ATACtrler";

static struct MajorDevice AtaMajorDev;
static struct MinorDevice AtaMinorDev;

struct AtaDevice AtaDevs[NR_ATA_DEVS] = {{.PData = PRIMARY_PDATA, .PError = PRIMARY_PERROR, .PFeatures = PRIMARY_PFEATURES, 
							 .PSecCount = PRIMARY_PSEC_COUNT, .PLBALow = PRIMARY_PLBA_LOW, .PLBAMid = PRIMARY_PLBA_MID, 
							 .PLBAHigh = PRIMARY_PLBA_HIGH, .PDevice = PRIMARY_PDEVICE, .PStatus = PRIMARY_PSTATUS, 
							 .PCommand = PRIMARY_PCOMMAND, .PAlterStatus = PRIMARY_PALTER_STATUS, .PDeviceCtrl = PRIMARY_PDEVICE_CTRL,
							 }, 
							 {.PData = PRIMARY_PDATA, .PError = PRIMARY_PERROR, .PFeatures = PRIMARY_PFEATURES, 
							 .PSecCount = PRIMARY_PSEC_COUNT, .PLBALow = PRIMARY_PLBA_LOW, .PLBAMid = PRIMARY_PLBA_MID, 
							 .PLBAHigh = PRIMARY_PLBA_HIGH, .PDevice = PRIMARY_PDEVICE, .PStatus = PRIMARY_PSTATUS, 
							 .PCommand = PRIMARY_PCOMMAND, .PAlterStatus = PRIMARY_PALTER_STATUS, .PDeviceCtrl = PRIMARY_PDEVICE_CTRL,
							 },
							 {.PData = SECONDARY_PDATA, .PError = SECONDARY_PERROR, .PFeatures = SECONDARY_PFEATURES, 
							 .PSecCount = SECONDARY_PSEC_COUNT, .PLBALow = SECONDARY_PLBA_LOW, .PLBAMid = SECONDARY_PLBA_MID, 
							 .PLBAHigh = SECONDARY_PLBA_HIGH, .PDevice = SECONDARY_PDEVICE, .PStatus = SECONDARY_PSTATUS, 
							 .PCommand = SECONDARY_PCOMMAND, .PAlterStatus = SECONDARY_PALTER_STATUS, .PDeviceCtrl = SECONDARY_PDEVICE_CTRL,
							 },
							 {.PData = SECONDARY_PDATA, .PError = SECONDARY_PERROR, .PFeatures = SECONDARY_PFEATURES, 
							 .PSecCount = SECONDARY_PSEC_COUNT, .PLBALow = SECONDARY_PLBA_LOW, .PLBAMid = SECONDARY_PLBA_MID, 
							 .PLBAHigh = SECONDARY_PLBA_HIGH, .PDevice = SECONDARY_PDEVICE, .PStatus = SECONDARY_PSTATUS, 
							 .PCommand = SECONDARY_PCOMMAND, .PAlterStatus = SECONDARY_PALTER_STATUS, .PDeviceCtrl = SECONDARY_PDEVICE_CTRL,
							 }
							 };
static void (*AtaMediaLoad[NR_ATA_MEDIA_TYPES])(int AtaDevNo);
static void (*AtaMediaUnLoad[NR_ATA_MEDIA_TYPES])();
int CurAtaDevNo = -1;

void AtaDevIRQIntASM();
static int AtaDInit();
static int AtaDUnInit();
static ssize_t AtaDRead(dev_no dev, void *dest, off_t offset, size_t n);
static ssize_t AtaDWrite(dev_no dev, const void *src, off_t offset, size_t n);
static int AtaDIoctl(dev_no dev, int cmd, unsigned int arg);
static int AtaDOpen(dev_no dev, struct File *fp, int flags);
static int AtaDClose(dev_no dev, struct File *fp);

static struct Process *WaitPs[NR_ATA_DEVS];

void AtaDevWaitIRQ(int AtaDevNo)
{
	int status;

	while (1)
	{
		CaseAtaDev(AtaDevNo);
		status = ASMInByte(AtaDevs[AtaDevNo].PStatus);
		if ((status & ATA_STATUS_DRQ) || (status & ATA_STATUS_ERR))
			return;
		Sleep(WaitPs + AtaDevNo, SLEEP_TYPE_UNINTABLE);
	}
}

void AtaDevIRQIntC()
{
// wake up all
	int i;
	// printk("AtaDev Catch IRQ: ");
	for (i = 0; i < NR_ATA_DEVS; ++i)
	{
		if (WaitPs[i])
		{
			WakeUp(WaitPs + i);
		}
	}
	// printk("\n");
}

void AtaDevNoOpenIDTGate()
{
	panic("No Open for ata device.");
}

void CaseAtaDev(int AtaDevNo)
{
	int ata_dev;
	int ata_dev_status;
	int n = 6;
	
	if (AtaDevNo == CurAtaDevNo)
		return;
	if (CurAtaDevNo != -1 && AtaDevs[CurAtaDevNo].CableType == ATA_CABLE_TYPE_UNDEF)
		return; // need abortion.
		
	CurAtaDevNo = AtaDevNo;
	ata_dev = ASMInByte(AtaDevs[CurAtaDevNo].PDevice);
	if ((AtaDevs[CurAtaDevNo].CableType - 1) == ((ata_dev >> 4) & 0x1))
		return;
	else
	{
		if (AtaDevs[CurAtaDevNo].CableType == ATA_CABLE_TYPE_SLAVE)
			ata_dev |= (1 << 4);
		else
			ata_dev &= ~(1 << 4);
			
		ASMOutByte(AtaDevs[CurAtaDevNo].PDevice, ata_dev);
		
		while (n-- & (ata_dev_status = ASMInByte(AtaDevs[CurAtaDevNo].PStatus)))
			;
		return;
	}
	// ~a~b + AB
}

void AtaInit()
{
	struct MajorDevice *ata_real;
	
	AtaMajorDev.major = MAJOR_DEV_NO_BUS;
	memcpy(AtaMajorDev.name, AtaDevName, strlen(AtaDevName));
	RegisterMajorDevice(&AtaMajorDev);

	AtaMinorDev.minor = MINOR_DEV_NO_ATA;
	memcpy(AtaMinorDev.name, AtaDevName, strlen(AtaDevName));
	AtaMinorDev.drg_proprity.is_drg_needed = 0;
	AtaMinorDev.drg = NULL;
	AtaMinorDev.DIoctl = AtaDIoctl;
	AtaMinorDev.DRead = AtaDRead;
	AtaMinorDev.DInit = AtaDInit;
	AtaMinorDev.DUnInit = AtaDUnInit;
	AtaMinorDev.DWrite = AtaDWrite;
	AtaMinorDev.DSelect = AtaDSelect;
	AtaMinorDev.DOpen = AtaDOpen;
	AtaMinorDev.DClose = AtaDClose;
	AtaMinorDev.block_size = 1;
	AtaMinorDev.type = DEV_TYPE_MAJOR_DEV;
	
	AtaMediaLoad[ATA_MEDIA_TYPE_HD] = AtaHDLoad;
	AtaMediaUnLoad[ATA_MEDIA_TYPE_HD] = AtaHDUnLoad;
	
	RegisterMinorDevice(DEV_NO_MAKE(MAJOR_DEV_NO_BUS, MINOR_DEV_NO_ATA), &AtaMinorDev);	// ?
	
	ata_real = KMalloc(sizeof(*ata_real));
	ata_real->major = MAJOR_DEV_NO_ATA;
	memcpy(ata_real->name, "ATA", strlen("ATA"));
	RegisterMajorDevice(ata_real);
	
	DInit(DEV_NO_MAKE(MAJOR_DEV_NO_BUS, MINOR_DEV_NO_ATA));
	
}

static int AtaDInit()
{
	int t;
	int i;
	int n;
	
	SetupIDTGate(INT_NO_IRQ_ATA_PRIMARY, SelectorFlatC, (addr)AtaDevIRQIntASM);
	SetupIDTGate(INT_NO_IRQ_ATA_SECONDARY, SelectorFlatC, (addr)AtaDevIRQIntASM);
	
	EnableIRQ(IRQ_MASTER_MASK_SLAVE);
	EnableIRQ(IRQ_SLAVE_ATA_PRIMARY);
	EnableIRQ(IRQ_SLAVE_ATA_SECONDARY);
			
	for (i = 0; i < NR_ATA_DEVS; ++i)
	{
		if (IsMasterDev(i))
			AtaDevs[i].CableType = ATA_CABLE_TYPE_MASTER;
		else
			AtaDevs[i].CableType = ATA_CABLE_TYPE_SLAVE;
			
		CaseAtaDev(i);

		ASMOutByte(AtaDevs[CurAtaDevNo].PLBALow, 50);
		if (ASMInByte(AtaDevs[CurAtaDevNo].PLBALow) != 50)
			continue;

		ASMOutByte(AtaDevs[CurAtaDevNo].PSecCount, 0);
		ASMOutByte(AtaDevs[CurAtaDevNo].PLBALow, 0);
		ASMOutByte(AtaDevs[CurAtaDevNo].PLBAMid, 0);
		ASMOutByte(AtaDevs[CurAtaDevNo].PLBAHigh, 0);
		ASMOutByte(AtaDevs[CurAtaDevNo].PCommand, ATA_CMD_IDENTIFY);
		
		t = ASMInByte(AtaDevs[CurAtaDevNo].PStatus);
		if (t == 0 || (t & ATA_STATUS_ERR))
		{
			AtaDevs[CurAtaDevNo].MediaType = ATA_MEDIA_TPYE_UNKNOWN;
			continue;
		}

		while (!((t = ASMInByte(AtaDevs[CurAtaDevNo].PStatus)) & ATA_STATUS_DRQ))
			;
		t = ASMInByte(AtaDevs[CurAtaDevNo].PLBAMid) | ASMInByte(AtaDevs[CurAtaDevNo].PLBAHigh);
		if (t)
		{
			// atapi;
			AtaDevs[CurAtaDevNo].MediaType = ATA_MEDIA_TPYE_UNKNOWN;	// but it should be cdrom;
			continue;
		}
		
		while (!(((t = ASMInByte(AtaDevs[CurAtaDevNo].PStatus)) & ATA_STATUS_DRQ) || (t & ATA_STATUS_ERR)))
			;
		if (t & ATA_STATUS_ERR)
		{
			AtaDevs[CurAtaDevNo].MediaType = ATA_MEDIA_TPYE_UNKNOWN;
			continue;
		}
		
		AtaDevs[CurAtaDevNo].AtaID = KMalloc(ATA_IDENTIFY_SIZE);
		assert(AtaDevs[CurAtaDevNo].AtaID, "AtaDSetup: KMalloc failed.");
		
		for (n = 0; n < ATA_IDENTIFY_SIZE / sizeof(u16); ++n)
			*(((u16 *)AtaDevs[CurAtaDevNo].AtaID) + n) = ASMInWord(AtaDevs[CurAtaDevNo].PData);
				
		if (ATA_IDENTIFY_IS_ATA(AtaDevs[CurAtaDevNo].AtaID))
		{
			AtaDevs[CurAtaDevNo].MediaType = ATA_MEDIA_TYPE_HD;
			AtaMediaLoad[ATA_MEDIA_TYPE_HD](CurAtaDevNo);
		}else
			AtaDevs[CurAtaDevNo].MediaType = ATA_MEDIA_TYPE_CDROM;
	}
	
	return 0;
}

static ssize_t AtaDRead(dev_no dev, void *dest, off_t offset, size_t n)
{
	if (offset < 0 || offset >= NR_ATA_DEVS)
		return DERR_ATA_OUT_OF_RANGE;
	
	memcpy(dest, &AtaDevs[offset], sizeof(AtaDevs[offset]));
	return sizeof(AtaDevs[offset]);
}

static ssize_t AtaDWrite(dev_no dev, const void *src, off_t offset, size_t n)
{
	return 0;
}

static int AtaDUnInit(dev_no dev)
{
	int i;
	for (i = 0; i < NR_ATA_MEDIA_TYPES; ++i)
		AtaMediaUnLoad[i]();
	for (i = 0; i < NR_ATA_DEVS; ++i)
		AtaDevs[i].MediaType = ATA_MEDIA_TPYE_UNKNOWN;
		
	SetupIDTGate(INT_NO_IRQ_ATA_PRIMARY, SelectorFlatC, (addr)AtaDevNoOpenIDTGate);
	SetupIDTGate(INT_NO_IRQ_ATA_SECONDARY, SelectorFlatC, (addr)AtaDevNoOpenIDTGate);
	
	DisableIRQ(IRQ_SLAVE_ATA_PRIMARY | IRQ_SLAVE_ATA_SECONDARY);
	return 0;
}

static int AtaDIoctl(dev_no dev, int cmd, unsigned int arg)
{
	return 0;
}

int AtaDSelect(dev_no dev, enum SelectCheckType check_type, select_table *tbl)
{
	if (check_type == SELECT_EXCEPT)
		return 1;
	dev_no minor = DEV_NO_GET_MINOR_NO(dev);
	
	if (minor < NR_MAX_PT_PER_ATA_DEV * 2)
	{
		// primary
		if (WaitPs[0] != NULL || WaitPs[1] != NULL)
		{
			add_wait(&WaitPs[minor / NR_MAX_PT_PER_ATA_DEV], tbl);
			return 0;
		} else
			return 1;
	} else
	{
		// secondary
		if (WaitPs[2] != NULL || WaitPs[3] != NULL)
		{
			add_wait(&WaitPs[minor / NR_MAX_PT_PER_ATA_DEV], tbl);
			return 0;
		} else
			return 1;
	}
}

int AtaDOpen(dev_no dev, struct File *fp, int flags)
{
	return 0;
}

int AtaDClose(dev_no dev, struct File *fp)
{
	return 0;
}
