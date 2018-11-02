#include "sched.h"
#include "types.h"
#include "stdlib.h"
#include "signal.h"
#include "exception.h"
#include "printk.h"
#include "wait.h"
#include "errno.h"
#include "asm/fpu_sse.h"
#include "gfs/minode.h"
#include "gfs/syslevel.h"
#include "mm/mm.h"
#include "mm/vzone.h"
#include "mm/varea.h"


void TellFather(void)
{
	if (Current->father == NULL)
		Current->father = GetProcessFromPID(2);
	
	Current->father->signal |= (1 << (SIGCHLD - 1));
	Current->father->state = PROCESS_STATE_RUNNING;
}

void Release(struct Process *p)
{
	if (p == NULL)
		panic("Release null Process ???");
	if (p->fpu_sse)
		KFree(p->fpu_sse, 0);
	if (p == LastProcessUseMath)
		LastProcessUseMath = NULL;
		
	RemoveProcessFromPQueue(p);
	KFree(p, 0);

}

static void kill_session(void)
{
	int i;
	struct Process *p;
	
	for (i = 0; i < NR_PRIOPRITIES; ++i)
	{
		if (Priorities[i].head == NULL)
			continue;
		for (p = Priorities[i].head; p != NULL; p = p->prio_next)
		{
			if (p->session == Current->session)
				p->signal |= 1 << (SIGHUP - 1);
		}
	}
}

int SysExit(int err_code)
{
	int i;
	
	debug_printk("[PM] Process %s(PID = %d) exit, code = %d.", Current->name, Current->pid, err_code);
	if (Current->pid == 1)
		return -EPERM;;
	
	for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
	{
		if (Current->vzones[i])
		{
			DetachVZone(Current, Current->vzones[i]);
			Current->vzones[i] = NULL;
		}
	}
	
	for (i = 0; i < NR_OPEN; ++i)
		if (Current->filep[i])
			SysClose(i);
	FreeMInode(Current->pwd);
	Current->pwd = NULL;
	FreeMInode(Current->root);
	Current->root = NULL;
	if (Current->executable)
	{
		FreeMInode(Current->executable);
		Current->executable = NULL;
	}
	if (Current->leader)
		kill_session();
	
	Current->exit_code = err_code;
	Current->state = PROCESS_STATE_ZOMBIE;
	TellFather();
	
	Schedule();
	
	debug_printk("[PM] Process %s(PID = %d) exit finish", Current->name, Current->pid);
	
	return -1;
}

int SysWaitPID(pid_t pid, long *stat_addr, int options)
{
	int flag;
	int i;
	struct Process *p;
	debug_printk("[TM] SysWaitPID(%d, %x, %d)", -1,stat_addr, options);
Repeat:	
	for (i = 0; i < NR_PRIOPRITIES; ++i)
	{
		if (Priorities[i].head == NULL)
			continue;
		
		for (p = Priorities[i].head; p != NULL; p = p->prio_next)
		{
			if (p == Current || p->father != Current)
				continue;
			if (pid > 0)
			{
				if (p->pid != pid)
					continue;
			} else if (pid == 0)
			{
				if (p->pgrp != Current->pgrp)
					continue;
			} else if (pid != -1)
			{
				if (p->pgrp != -pid)
					continue;
			}
			
			if (p->state == PROCESS_STATE_STOPPED)
			{
				if (options & WUNTRACED)
				{
					*stat_addr = 0x7f;
					return p->pid;
				}
			} else if(p->state == PROCESS_STATE_ZOMBIE)
			{
				Current->cutime += p->utime;
				Current->cstime += p->stime;
				flag = p->pid;
				*stat_addr = p->exit_code;
				Release(p);
				return flag;
			} else
			{
				flag = 1;
				continue;
			}
		}
	}

	if (flag)
	{
		if (options & WNOHANG)
			return 0;
		Current->state = PROCESS_STATE_INTERRUPTIBLE;
		Schedule();
		if (!(Current->signal &= ~(1 << (SIGCHLD - 1))))
			goto Repeat;
		else
			return -EINTR;
	}
	
	return -ECHILD;
}

int SendSig(int sig, struct Process *p, int force)
{
	if (p == NULL || sig < 1 || sig > 32)
		return -EINVAL;
	
	if (force || (Current->euid == p->euid) || Current->euid == 0)
		p->signal |= (1 << (sig - 1));
	else
		return -EPERM;
	return 0;
}

#define FIND_PROCESS_BEGIN(i, p, flag)	for (i = 0; i < NR_PRIOPRITIES; ++i) {\
											if (Priorities[i].head == NULL) \
												break; \
											for (p = Priorities[i].head; p != Priorities[i].tail; p = p->prio_next) {
#define FIND_PROCESS_END(flag)				} if(flag) break;}

#define SYS_KILL_SEND_SIG(test_cond, force, is_loop_full)	\
											{FIND_PROCESS_BEGIN(i, p, flag) \
											if (test_cond) \
											{ \
												ret_cd = SendSig(sig, p, force); \
												if (is_loop_full) { \
													flag = 1; \
													break; \
												}\
											} \
											FIND_PROCESS_END(flag) \
											}

int SysKill(int pid, int sig)
{
	int i;
	int flag = 0;
	int ret_cd;
	struct Process *p;
	
	debug_printk("[SIGNAL] SysKill(%d, %d)", pid, sig);
	
	if (pid > 0)
	{
		SYS_KILL_SEND_SIG(p->pid == pid, 0, 0);
	}else if(pid == 0)
	{
		SYS_KILL_SEND_SIG(p->pgrp == Current->pid, 1, 1);
	}else if(pid == -1)
	{
		SYS_KILL_SEND_SIG(p->pid != 1, 0, 1);	
	} else
	{
		SYS_KILL_SEND_SIG(p->pgrp == -pid, 0, 1);
	}
	return ret_cd;
}
