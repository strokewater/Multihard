#ifndef PROCESS_H
#define PROCESS_H

#define NR_PIDS			1024

#include "types.h"
#include "mm/mm.h"

void ProcessInit();
pid_t GetFreePID();
void ReleasePID(pid_t pid);

#define PROCESS_STK0_BASE		(CORE_SPACE_LOWER + 0x6000000)
#define PROCESS_STK0_TOP		(CORE_SPACE_LOWER + 0x6001fe0)

#define NR_PG_PROCESS_ARG_ENV_TMP		32
#define NR_MAX_ARGVS				512
#define NR_MAX_ENVPS				1024
#define PROCESS_ARG_ENV_TMP_UPPER	USER_SPACE_UPPER	// 0xffbfffff
#define PROCESS_ARG_ENV_TMP_LOWER	(PROCESS_ARG_ENV_TMP_UPPER - NR_PG_PROCESS_ARG_ENV_TMP * PAGE_SIZE + 1) // 0xffbe0000
#define PROCESS_ARG_ENV_DATA_SIZE	((PROCESS_ARG_ENV_TMP_UPPER - PROCESS_ARG_ENV_TMP_LOWER + 1) \
										- 4 * 5 - (NR_MAX_ARGVS + NR_MAX_ENVPS) * 4)
#define NR_PG_PROCESS_STACK				32
struct ArgEnvTmpArea
{
	int argc;
	char *data_ptr;
	char *argvs[NR_MAX_ARGVS];
	char *envps[NR_MAX_ENVPS];
	char data[PROCESS_ARG_ENV_DATA_SIZE];
};

int SysFork();
pid_t SysGetPid();
pid_t SysGetPPid();
uid_t SysGetUID();
uid_t SysGetEUID();
gid_t SysGetGID();
gid_t SysGetEGID();
int SysGetPGRP(void);
int SysSetReGID(gid_t rgid, gid_t egid);
int SysSetGID(gid_t gid);
int SysSetReUID(uid_t ruid, uid_t euid);
int SysSetUID(uid_t uid);
int SysSetPGID(pid_t pid, int pgid);
int SysSetSID(void);
int SysUMask(int mask);

#define PROCESS_STACK_UPPER			(PROCESS_ARG_ENV_TMP_LOWER - 1)
// 0xffbfffff

#endif
