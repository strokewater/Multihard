#ifndef SCHED_H
#define SCHED_H

#include "types.h"
#include "time.h"
#include "signal.h"
#include "limits.h"
#include "gfs/gfs.h"
#include "asm/pm.h"
#include "asm/int_content.h"
#include "timer.h"

#define PROCESS_STATE_RUNNING				0
#define PROCESS_STATE_INTERRUPTIBLE			1
#define PROCESS_STATE_UNINTERRUPTIBLE		2
#define PROCESS_STATE_ZOMBIE				3
#define PROCESS_STATE_STOPPED				4

#define SLEEP_TYPE_INTABLE				0
#define SLEEP_TYPE_UNINTABLE				1

#define NR_VZONES_PER_PROCESS	10

struct Process
{
	char *name;
	pid_t pid;
	struct Process *father;
	
	int state;
	int counter;
	int total_counter;
	
	int priority;
	struct Process *prio_next;
	
	int signal;
	int blocked;
	struct SigAction sigaction[NSIG];
	
	int exit_code;
	int errno;
	
	int tty;
	long pgrp;
	long session;
	long leader;
	uid_t uid, euid, suid;
	gid_t gid, egid, sgid;
	struct Timer *alarm;
	int utime;
	int stime;
	int cutime;
	int cstime;
	int start_time;
	
	u16 umask;
	
	struct MInode *pwd;
	struct MInode *root;
	
	struct MInode *executable;
	u32 close_on_exec;
	
	struct File *filep[NR_OPEN];

	struct VZone *vzones[NR_VZONES_PER_PROCESS];
	int brk_offset_in_vzones;
	
	u32 cr3;
	u32 esp;
	u32 ebp;
	u32 eflags;
	char *fpu_sse;
	struct IntContent *int_content;
};

struct PriorityQueue
{
	struct Process *head;
	struct Process *tail;
};

#define NR_PRIOPRITY_QUEUE		13
#define NR_PRIOPRITIES			6

#define SCHED_INIT_MASK			0xffffffc0
#define SCHED_MASK_ALL_CHECKED 		0xffffffff
// schedule prioprity mask, every bit stands for a proiprity,
//  if one bit is set, the prioprity it stands for has been accessed, otherwise, it still accessable.
//   for some undefinded prioprity, their bits are all set to 1.

#define SCHED_DEFAULT_COUNTER	10
#define SCHED_DEFAULT_PRIORITY	(NR_PRIOPRITIES - 1)

extern struct PriorityQueue Priorities[];

void Schedule();
void MathStateRestore();

extern struct Process *Current;
extern struct Process Idle;

void InsertPQueue(struct Process *p);
struct Process *PopPQueue(int prio);
struct Process *GetProcessFromPID(pid_t pid);
void RemoveProcessFromPQueue(struct Process *p);

int SysPause(void);
int SysNice(int increase);

void Sleep(struct Process **p, int type);
void WakeUp(struct Process **p);


#endif
