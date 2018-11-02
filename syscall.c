#include "types.h"
#include "stdlib.h"
#include "syscall.h"
#include "setup.h"
#include "exit.h"
#include "process.h"
#include "printk.h"
#include "time.h"
#include "sched.h"
#include "signal.h"
#include "errno.h"
#include "wait.h"
#include "mm/vzone.h"
#include "asm/pm.h"
#include "asm/asm.h"
#include "gfs/gfs.h"
#include "gfs/select.h"
#include "gfs/syslevel.h"

struct utsname {
	char sysname[9];
	char nodename[9];
	char release[9];
	char version[9];
	char machine[9];
};

int SysExecv(const char *filename, const char *argvs[], const char *envps);
int SysNoSYS();
int SysUName(struct utsname *name);

void SysPrint(const char *format)
{
	printk(format);
}

addr SysCallTable[NR_SYS_CALL] = {
							(addr)SysSetup,		// OK
							(addr)SysExit,  	// OK
							(addr)SysFork, 		// OK
							(addr)SysRead,		// OK
							(addr)SysWrite,		// OK
							(addr)SysOpen,		// OK
							(addr)SysClose,		// OK
							(addr)SysWaitPID,	// OK
							(addr)SysCreat,		// OK
							(addr)SysLink,		// OK
							(addr)SysUnLink,	// OK
							(addr)SysExecv,		// OK
							(addr)SysChDir,		// OK
							(addr)SysTime,		// NON-TEST OK
							(addr)SysMkNod,		// OK
							(addr)SysChMod, 	// NON-TEST OK
							(addr)SysChOwn,		// NON-TEST OK
							(addr)SysNoSYS,		// SysBreak
							(addr)SysStat,		// NON-TEST OK
							(addr)SysLseek,		// NON-TEST OK
							(addr)SysGetPid,	// NON-TEST OK
							(addr)SysMount,		// 当心Mount,与Linux不同!! // OK
							(addr)SysUMount,	// OK
							(addr)SysSetUID,	// NON-TEST OK
							(addr)SysGetUID,	// NON-TEST OK
							(addr)SysSTime,		// NON-TEST OK
							(addr)SysNoSYS,		// SysPTrace
							(addr)SysAlarm,		// OK
							(addr)SysFStat,		// NON-TEST OK
							(addr)SysPause,		// NON-TEST OK
							(addr)SysUTime,		// NON-TEST OK
							(addr)SysNoSYS,		// SysSTTY
							(addr)SysNoSYS,		// SysGTTY
							(addr)SysAccess,	// NON-TEST OK
							(addr)SysNice,		// NON-TEST OK
							(addr)SysNoSYS,		// SysFTime
							(addr)SysSync,		// NON-TEST OK
							(addr)SysKill,		// NON-TEST OK
							(addr)SysNoSYS,		// SysReName
							(addr)SysMkDir,		// OK
							(addr)SysRmDir,		// OK
							(addr)SysDup,		// NON-TEST OK
							(addr)SysPipe,		// OK
							(addr)SysTimes,		// NON-TEST OK
							(addr)SysNoSYS,		// SysProf
							(addr)SysBrk,
							(addr)SysSetGID,	// NON-TEST OK
							(addr)SysGetGID,	// NON-TEST OK
							(addr)SysSignal,	// NON-TEST OK
							(addr)SysGetEUID,	// NON-TEST OK
							(addr)SysGetEGID,	// NON-TEST OK
							(addr)SysNoSYS,		// SysAcct
							(addr)SysNoSYS,		// SysPhys
							(addr)SysNoSYS,		// SysLock
							(addr)SysIoctl,		// NON-TEST OK
							(addr)SysFcntl,		// NON-TEST OK
							(addr)SysNoSYS,		// SysMPX
							(addr)SysSetPGID,	// NON-TEST OK
							(addr)SysNoSYS,		// SysULimit
							(addr)SysUName,		// NON-TEST OK
							(addr)SysUMask,		// NON-TEST OK
							(addr)SysChRoot,	// NON-TEST OK
							(addr)SysNoSYS,		// SysUStat
							(addr)SysDup2,		// NON-TEST OK
							(addr)SysGetPPid,	// NON-TEST OK
							(addr)SysGetPGRP,	// NON-TEST OK
							(addr)SysSetGID,	// NON-TEST OK
							(addr)SysSigAction,	// NON-TEST OK
							(addr)SysSGetMask,	// NON-TEST OK
							(addr)SysSSetMask,	// NON-TEST OK
							(addr)SysSetReUID,	// NON-TEST OK
							(addr)SysSetReGID,	// NON-TEST OK
							(addr)SysSigSuspend,
							(addr)SysSigPending
							};

int SysNoSYS()
{
	return -ENOSYS;
}

static struct utsname osname = {"Multihard", "nodename", "0.1", "version", "i386"};

int SysUName(struct utsname *name)
{
	debug_printk("[SYS] SysUName(%x)", name);
	if (name == NULL)
		return -ERROR;
	*name = osname;
	return 0;
}

void SysCallInit(void)
{
	extern void SystemCall();
	
	SetupIDTGate(SYS_CALL_INT_NO, SelectorFlatC, (u32)SystemCall);
	SysCallTable[SYS_CALL_NO_SIGRETURN] = (addr)SysSigReturn;
	SysCallTable[SYS_CALL_NO_lstat] = (addr)SysLStat;
	SysCallTable[SYS_CALL_NO_readdir] = (addr)SysReadDir;
	SysCallTable[SYS_CALL_NO_GETTIMEOFDAY] = (addr)SysGetTimeOfDay;
	SysCallTable[SYS_CALL_NO_SETTIMEOFDAY] = (addr)SysSetTimeOfDay;
	SysCallTable[SYS_CALL_NO_getcwd] = (addr)SysGetCWD;
	SysCallTable[SYS_CALL_NO_SELECT] = (addr)SysSelect;
}
