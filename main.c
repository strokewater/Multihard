#include "mm/mm.h"
#include "mm/mhash.h"
#include "asm/traps.h"
#include "asm/pm.h"
#include "process.h"
#include "drivers/drivers.h"
#include "drivers/ata/ata.h"
#include "syscall.h"
#include "time.h"
#include "asm/fpu_sse.h"


void Main()
{
	FPUSSEInit();
	PmInit();
	TrapsInit();
	MMInit();
	InitTime();
	SysCallInit();
	ProcessInit();
	
	while (1);
	return;
}
