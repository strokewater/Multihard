#include "drivers/ata/ata.h"
#include "drivers/ata/ata_hd.h"
#include "drivers/drivers.h"
#include "mm/varea.h"
#include "assert.h"
#include "stdlib.h"
#include "string.h"
#include "asm/asm.h"
#include "printk.h"

static struct PartitionTable *PTTables[NR_ATA_DEVS];	// pointers of array(struct PartitionTable[]).

static int DHDInit(dev_no dev);
static ssize_t DHDRead(dev_no dev, void *dest, off_t offset, size_t n);
static ssize_t DHDWrite(dev_no dev, const void *src, off_t offset, size_t n);
static int DHDUnInit(dev_no dev);
static int DHDIoctl(dev_no dev, int cmd, unsigned int arg);
static int DHDOpen(dev_no dev, struct File *fp, int flags);
static int DHDClose(dev_no dev, struct File *fp);

static struct MinorDevice HDDevModule = {.type = DEV_TYPE_BLOCK, .block_size = SECTOR_SIZE, 
					.DInit = &DHDInit, .DRead = DHDRead, .DWrite = DHDWrite, .DIoctl = DHDIoctl, 
					.DUnInit = DHDUnInit, .DSelect = AtaDSelect, .DOpen = DHDOpen, .DClose = DHDClose};

void AtaHDUnLoad()
{
	// ?
}

void AtaHDLoad(int AtaDevNo)
{
	struct MinorDevice *EntireHD;
	
	EntireHD = KMalloc(sizeof(*EntireHD));
	assert(EntireHD, "kmalloc failed.");
	
	*EntireHD = HDDevModule;
	
	EntireHD->minor = DEV_NO_GET_MINOR_NO(AtaNoToDevNo(AtaDevNo));
	strcpy(EntireHD->name, "hdx");
	EntireHD->name[2] = 'a' + AtaDevNo;
	EntireHD->drg_proprity.is_drg_needed = 1;
	EntireHD->drg_proprity.is_owner = 1;
	
	RegisterMinorDevice(DEV_NO_MAKE(MAJOR_DEV_NO_ATA, EntireHD->minor), EntireHD);
	DInit(DEV_NO_MAKE(MAJOR_DEV_NO_ATA, EntireHD->minor));
	KFree(EntireHD, sizeof(*EntireHD));
}

// dev: entire HD device no
static size_t ReadExtendPartitions(off_t extend_start_sect, struct PartitionTable *pts, dev_no hd_dev)
{
	void *buf_tptr;
	struct PartitionTable *buf, *t;
	size_t nlogical = 0;
	
	buf_tptr = KMalloc(SECTOR_SIZE);
	
	while (1)
	{
		DHDRead(hd_dev, buf_tptr, extend_start_sect, 1);
		buf = buf_tptr + 0x1be;
			
		t = KMalloc(sizeof(*t));
		*t = buf[0];
		t->start_sect += extend_start_sect;
		*pts = *t;
		
		if (buf[1].sys_ind == PT_SYS_TYPE_EMPTY)
			break;
		
		extend_start_sect += buf[1].start_sect;
		++nlogical;
		++pts;
	}
	 
	KFree(buf_tptr, SECTOR_SIZE);
	return nlogical;
}

static void ReadPatitions(struct PartitionTable *pts, dev_no dev)
{
	void *buf_tptr;
	struct PartitionTable *buffer;
	int bi, pi, tmp;
	size_t n;
	
	buf_tptr = KMalloc(SECTOR_SIZE);
	assert(buf_tptr, "kmalloc failed.");
	
	DHDRead(dev, buf_tptr, pts[0].start_sect, 1);
	buffer = buf_tptr + 0x1BE;
	
	for (bi = 0, pi = 1, n = 0; n < 4 && pi < NR_MAX_PT_PER_ATA_DEV; ++n)
	{
		if (buffer[bi].sys_ind == PT_SYS_TYPE_EMPTY)
			++bi;
		else
		{
			pts[pi] = buffer[bi];
			if (buffer[bi].sys_ind == PT_SYS_TYPE_EXTEND)
			{
				tmp = ReadExtendPartitions(pts[pi].start_sect, &pts[pi+1], dev) + 1;
				pi += tmp;
				bi += tmp;
			}
			else
			{
				++pi;
				++bi;
			}
		}
	}
	
	KFree(buf_tptr, 0);
}

static int DHDInit(dev_no dev)
{
	struct PartitionTable *pt;
	struct MinorDevice *PTDev;
	struct MinorDevice *EntireHD;
	int AtaDevNo = DevNoToAtaNo(dev);
	dev_no minor = DEV_NO_GET_MINOR_NO(dev);
	int i;
	
	if (minor % NR_MAX_PT_PER_ATA_DEV == 0)
	{
		// full disk.
		EntireHD = GetMinorDevice(dev);
		
		pt = KMalloc(sizeof(struct PartitionTable) * NR_MAX_PT_PER_ATA_DEV);
		assert(pt, "AtaHDLoad: KMalloc failed.");
		
		memset(pt, 0, sizeof(*pt));
		
		pt[0].boot_ind = PT_BOOT_ID_NO;
		pt[0].head = 0;
		pt[0].sector_cyl = 0 << 6 | 1;
		pt[0].cyl  = 0;
		pt[0].sys_ind = PT_SYS_TYPE_EMPTY;
		pt[0].end_head = 0;
		pt[0].end_cyl = 0;
		pt[0].start_sect = 0;
		pt[0].nr_sects = *(u32 *)(AtaDevs[AtaDevNo].AtaID + ATA_IDENTIFY_OFFSET_NR_SECS);
		for (i = 1; i < NR_MAX_PT_PER_ATA_DEV; ++i)
			pt[i].sys_ind = PT_SYS_TYPE_EMPTY;
		PTTables[AtaDevNo] = pt;
		ReadPatitions(pt, dev);
		
		for (i = 1; i < NR_MAX_PT_PER_ATA_DEV; ++i)
		{
			if (PTTables[AtaDevNo][i].sys_ind == PT_SYS_TYPE_EMPTY)
				continue;
			
			PTDev = KMalloc(sizeof(*PTDev));
			assert(PTDev, "kmalloc failed.");
			*PTDev = HDDevModule;
		
		
			strcpy(PTDev->name, EntireHD->name);
			PTDev->name[3] = itoc(i);
			PTDev->minor = DEV_NO_GET_MINOR_NO(dev + i);
			PTDev->drg_proprity.is_drg_needed = 1;
			PTDev->drg_proprity.is_owner = 0;
			PTDev->drg = EntireHD->drg;
			
			RegisterMinorDevice(dev + i, PTDev);
			DInit(DEV_NO_MAKE(MAJOR_DEV_NO_ATA, AtaDevNo * NR_MAX_PT_PER_ATA_DEV + i ));
		}
		printk("HD%d:\n", AtaDevNo);
		printk("\thd%c: Size = %dM. \n", 'a' + AtaDevNo, pt[0].nr_sects / 2048);
		for (i = 1; i < NR_MAX_PT_PER_ATA_DEV; ++i)
		{
			if (pt[i].sys_ind == PT_SYS_TYPE_EMPTY)
				break;
			printk("\thd%c%d: SystemID = %x, Size = %dM\n", 'a' + AtaDevNo, i, pt[i].sys_ind, pt[i].nr_sects / 2048);
		}
	}
	// for detailed partition, it's meaningless.
	
	return 0;
}

static int IsHDReady(void)
{
	int retries = 10000;
	int k;
	
	while (--retries && (k = ASMInByte(AtaDevs[CurAtaDevNo].PStatus) & ATA_STATUS_DRDY))
	{
		k = ASMInByte(AtaDevs[CurAtaDevNo].PStatus);
		if ((k & ATA_STATUS_DRDY) && !(k & ATA_STATUS_BSY))
			return retries;
	}
	return retries;
}

static ssize_t DHDRW(dev_no dev, void *dest, off_t offset, size_t n, int cmd)
{
	int ret;
	struct AtaDeviceReg p;
	s8 p_buf;
	int ata_no = DevNoToAtaNo(dev);
	int i;
	int nerr = 0;
	int status;
	// printk("HD Action(RW), offset = %d, cmd = %d, Number = %d Make\n", offset, cmd, n);
	
	// lba change to absolute lba.
	offset += PTTables[ata_no][DEV_NO_GET_MINOR_NO(dev) % NR_MAX_PT_PER_ATA_DEV].start_sect;
	
	while (nerr < ATA_HD_MAX_ERR_PER_RW)
	{
		CaseAtaDev(ata_no);
		VerifyArea(dest, SECTOR_SIZE);
		// there is an error.
	
		if (!IsHDReady())
			return -DERR_ATA_CONTROL_TIME_OUT;
	
		p_buf = ASMInByte(AtaDevs[CurAtaDevNo].PDevice);
		memcpy(&p, &p_buf, sizeof(p));
		p.LBAOrCHS = 1;
		p.LBAOrHead = LBA_FIRST_PART(offset);
		memcpy(&p_buf, &p, sizeof(p));
		ASMOutByte(AtaDevs[CurAtaDevNo].PDevice, p_buf);	// 4
		ASMOutByte(AtaDevs[CurAtaDevNo].PFeatures, 0x0);
		ASMOutByte(AtaDevs[CurAtaDevNo].PSecCount, n);
		ASMOutByte(AtaDevs[CurAtaDevNo].PLBALow, LBA_FOURTH_PART(offset));
		ASMOutByte(AtaDevs[CurAtaDevNo].PLBAMid, LBA_THIRD_PART(offset));
		ASMOutByte(AtaDevs[CurAtaDevNo].PLBAHigh, LBA_SECOND_PART(offset));
		ASMCli();
		ASMOutByte(AtaDevs[CurAtaDevNo].PCommand, cmd);
	
		AtaDevWaitIRQ(ata_no);
		ASMSti();
		// go there, IRQ interrupt was gone.
		
		status = ASMInByte(AtaDevs[CurAtaDevNo].PStatus);
		if (status & ATA_STATUS_ERR)
			++nerr;
		else if(status & ATA_STATUS_DRDY)
		{
			if (cmd == ATA_CMD_READ_SECTORS)
			{
				for (i = 0; i < SECTOR_SIZE * n / 2; ++i)
					*((s16 *)(dest) + i)  = ASMInWord(AtaDevs[CurAtaDevNo].PData);
			}else
			{
				for (i = 0; i < SECTOR_SIZE * n / 2; ++i)
					ASMOutWord(AtaDevs[CurAtaDevNo].PData, *((s16 *)(dest) + i));
			}
			ret = n;
			break;
		}else
			panic("Unexcepted status register.");
		
	}
	// printk("HD Action(RW), offset = %d, cmd = %d, Number = %d Finish\n", toff, cmd, n);
	return ret;
}

// offset: LBA
static ssize_t DHDRead(dev_no dev, void *dest, off_t offset, size_t n)
{
	return DHDRW(dev, dest, offset, n, ATA_CMD_READ_SECTORS);
}

static ssize_t DHDWrite(dev_no dev, const void *src, off_t offset, size_t n)
{
	return DHDRW(dev, (void *)src, offset, n, ATA_CMD_WRITE_SECTORS);
}

static int DHDUnInit(dev_no dev)
{
	int i;
	if (DEV_NO_GET_MINOR_NO(dev) == 0)
	{
		for (i = 0; i < NR_ATA_DEVS; ++i)
		{
			CancelMinorDevice(AtaNoToDevNo(i));
			
			if (PTTables[i] == NULL)
				continue;
			KFree(PTTables[i], 0);
			PTTables[i] = NULL;
		}
	}else
		return 0;
	return 0;
}

static int DHDIoctl(dev_no dev, int cmd, unsigned int arg)
{
	return 0;
}

static int DHDOpen(dev_no dev, struct File *fp, int flags)
{
	return 0;
}

static int DHDClose(dev_no dev, struct File *fp)
{
	return 0;
}
