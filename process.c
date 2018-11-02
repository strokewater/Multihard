#include "sched.h"
#include "printk.h"
#include "asm/pm.h"
#include "drivers/i8259a/i8259a.h"
#include "asm/asm.h"
#include "asm/fpu_sse.h"
#include "stdlib.h"
#include "idle.h"
#include "sched.h"
#include "mm/mhash.h"
#include "mm/mm.h"
#include "mm/varea.h"
#include "mm/vzone.h"
#include "errno.h"
#include "types.h"
#include "string.h"
#include "time.h"
#include "asm/int_content.h"

void RetFromSysCall();

PDIR *ProcessNewRawCR3()
{
	PDIR *ret, *vret;
	// 返回物理地址
	
	ret = GetPhyPage();
	vret = MHashPage(ret);
	memcpy(vret, KerGlobalPages, sizeof(KerGlobalPages));
	memset(vret + sizeof(KerGlobalPages), 0, PAGE_SIZE - sizeof(KerGlobalPages));
	UnHashPage(vret);
	
	return ret;
}

void ProcessNewStk0()
{
	void * pa;
	addr va;
	
	for (va = CORE_SPACE_STK0_LOWER; va <= CORE_SPACE_STK0_UPPER; va += PAGE_SIZE)
	{
		pa = GetPhyPage();
		SetMPage((void *)va, (s32)pa | PG_P | PG_RW);
	}	
}

void ProcessNewVZones(struct Process *p)
{
	int i;
	struct VZone *vz;
	for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
	{
		if (Current->vzones[i])
		{
			if (Current->vzones[i]->type & VZONE_TYPE_WRITE && 
				!(Current->vzones[i]->type & VZONE_TYPE_PROCESS_PUBLIC))
			{
				vz = DupVZone(Current->vzones[i]);
			} else
				vz = Current->vzones[i];
			p->vzones[i] = NULL;
			AttachVZone(p, vz);
		}
	}
	p->brk_offset_in_vzones = -1;
}

void ProcessInit()
{
	struct VZone *vz;
	void *text_base, *text_base_pa;
	
	Idle.name = "System Process";
	Idle.pid = GetFreePID();
	Idle.father = NULL;
	Idle.state = PROCESS_STATE_RUNNING;
	Idle.counter = 1;
	Idle.total_counter = 1;
	Idle.priority = 0;		// meaningless
	Idle.prio_next = NULL;
	Idle.umask = 0111;
	Idle.cr3 = (u32)ProcessNewRawCR3();
	Idle.esp = IDLE_STK_BASE + PAGE_SIZE - 4;
	Idle.ebp = 0;
	Idle.eflags = 0x200;
	Idle.fpu_sse = NULL;
	Idle.int_content = NULL;
	Idle.alarm = 0;
	Idle.utime = 0;
	Idle.stime = 0;
	Idle.cutime = 0;
	Idle.start_time = jiffies;
	Idle.executable = NULL;
	Idle.brk_offset_in_vzones = -1;
	Idle.leader = 0;
	
	scr3 = Idle.cr3;	// ?
	text_base_pa = GetPhyPage();
	text_base = MHashPage(text_base_pa);
	memcpy(text_base, &IdleText, PAGE_SIZE);
	UnHashPage(text_base);
	vz = AllocVZone((void *)IDLE_TEXT_BASE, PAGE_SIZE, VZONE_CTYPE_TEXT, 
					VZONE_MAPPED_FILE_PHYSICAL_MEM, (off_t)text_base_pa, PAGE_SIZE);
	AttachVZone(&Idle, vz);
	vz = AllocVZone((void *)IDLE_STK_BASE + PAGE_SIZE - 1, PAGE_SIZE, VZONE_CTYPE_STACK, 
					VZONE_MAPPED_FILE_ANONYMOUSE_MEM, 0, 0);
	AttachVZone(&Idle, vz);
	
	ProcessNewStk0();
	
	Current = &Idle;
	
	
	SetupIDTGate(INT_NO_IRQ_TIMER, SelectorFlatC, (addr)&ClockHandle);
	ASMCli();
	EnableIRQ(IRQ_MASTER_MASK_TIMER);
	
	__asm__("movl %%eax, %%cr3;" : : "a"(scr3): );
	__asm__("pushl %%eax; ret;" : : "a"((addr)&MoveToIdle) : );
}

// pid.
static int PIDBitmap[NR_PIDS / sizeof(int)] = {1};
static pid_t LastPID = 0;

pid_t GetFreePID()
{
	int i, j;
	int l;
	int n;
	
	l = LastPID + 1;
	if (l == NR_PIDS)
		l = 0;
	
	for (i = l / (sizeof(int) * 8), n = 0; 
		i < NR_PIDS / sizeof(int) && n < NR_PIDS / sizeof(int); 
		n++)
	{
		if (PIDBitmap[i] != 0xffffffff)
		{
			for (j = 0; j < sizeof(int) * 8 && PIDBitmap[i] & (1 << j); ++j)
				;
			if (j < sizeof(int) * 8)
			{
				PIDBitmap[i] |= (1 << j);
				return (pid_t)(i * sizeof(int) * 8 + j);
			}
		}
		++i;
		if (i == NR_PIDS / sizeof(int))
			i = 0;
	}
	
	return -EAGAIN;
}

void ReleasePID(pid_t pid)
{
	pid_t i,j;
	 i = pid / (sizeof(s32) * 8);
	 j = pid % (sizeof(s32) * 8);
	 
	 PIDBitmap[i] = PIDBitmap[i] & (~(1 << j));
	 LastPID = pid;
}

int SysFork()
{
	int len;
	struct Process *new_process;
	pid_t pid;
	PDIR *dirs;
	PAGE *pages;
	u32 *child_stk0;
	u32 child_stk0_esp;
	pid_t i;
	struct File *f;
	
	pid = GetFreePID();
	if (pid == -EAGAIN)
		return -EAGAIN;
	
	new_process = KMalloc(sizeof(*new_process));
	if (new_process == NULL)
	{
		ReleasePID(pid);
		return -ENOMEM;
	}
	
	memcpy(new_process, Current, sizeof(*Current));
	len = strlen(Current->name);
	new_process->name = KMalloc(len + 1);
	strcpy(new_process->name, Current->name);
	new_process->state = PROCESS_STATE_UNINTERRUPTIBLE;
	new_process->pid = pid;
	new_process->father = Current;
	new_process->counter = SCHED_DEFAULT_COUNTER;
	new_process->total_counter = SCHED_DEFAULT_COUNTER;
	new_process->priority = SCHED_DEFAULT_PRIORITY;
	new_process->signal = 0;
	new_process->cr3 = (u32)ProcessNewRawCR3();	// cow!!
	new_process->fpu_sse = NULL;
	new_process->int_content = NULL;
	new_process->alarm = 0;
	new_process->utime = 0;
	new_process->stime = 0;
	new_process->cutime = 0;
	new_process->cstime = 0;
	new_process->start_time = jiffies;
	new_process->brk_offset_in_vzones = -1;
	new_process->leader = 0;
	
	scr3 = new_process->cr3;
	
	for (i = 0; i < NR_OPEN; ++i)
	{
		if ((f = Current->filep[i]) != NULL)
			f->count++;
	}
	if (Current->pwd)
		Current->pwd->count++;
	if (Current->root)
		Current->root->count++;
	if (Current->executable)
		Current->executable->count++;
		
	
	// 对子进程的内核栈手动复制，--- 系统调用进入内核 ---- 时钟中断保存的数据 -----
	// 手动创建子进程的内核栈
	ProcessNewStk0();
	ProcessNewVZones(new_process);
	
	scr3 = Current->cr3;
	
	dirs = MHashPage((void *)new_process->cr3);
	if (dirs == NULL)
		return -EAGAIN;
	pages = MHashPage( (void *) ( PDIR_GET_PA(dirs[PDIR_NO(PROCESS_STK0_TOP)]) ) );
	if (pages == NULL)
		return -EAGAIN;
	child_stk0 = MHashPage((void *)(PAGE_GET_PA(pages[PAGE_NO(PROCESS_STK0_TOP)])));
	if (child_stk0 == NULL)
		return -EAGAIN;
	child_stk0_esp = ((addr)child_stk0 + PAGE_GET_OFFSET(PROCESS_STK0_TOP));
	
	child_stk0_esp -= sizeof(*CurIntContent);
	new_process->int_content = (struct IntContent *) (child_stk0_esp);
	*new_process->int_content = *CurIntContent;
	new_process->int_content->eax = 0;
	new_process->int_content->previous = 0;

	child_stk0_esp -= sizeof(addr);
	*(addr *)(child_stk0_esp) = (addr)RetFromSysCall;
	
	UnHashPage(child_stk0);
	UnHashPage(pages);
	UnHashPage(dirs);
	
	new_process->esp = PAGE_MASK_OFFSET(PROCESS_STK0_TOP) + PAGE_GET_OFFSET((addr)child_stk0_esp);
	new_process->int_content = PAGE_MASK_OFFSET(PROCESS_STK0_TOP) + PAGE_GET_OFFSET((addr)new_process->int_content);
	new_process->state = PROCESS_STATE_RUNNING;
	InsertPQueue(new_process);
	
	new_process->start_time = CURRENT_TIME;
	debug_printk("[TM] Do SysFork, PPID = %d, PID = %d .", Current->pid, new_process->pid);
	return pid; // for father   
}

pid_t SysGetPid()
{
	debug_printk("[PROCESS] SysGetPid() == %d", Current->pid);
	return Current->pid;
}

pid_t SysGetPPid()
{
	if (Current->father)
		debug_printk("[PROCESS] SysGetPPid() == %d", Current->father->pid);
	
	if (Current->father == NULL)
		return -ENOENT;
	return Current->father->pid;
}

uid_t SysGetUID()
{
	debug_printk("[PROCESS] SysGetUID() == %d", Current->uid);
	return Current->uid;
}

uid_t SysGetEUID()
{
	debug_printk("[PROCESS] SysGetEUID() == %d", Current->euid);
	return Current->euid;
}

gid_t SysGetGID()
{
	debug_printk("[PROCESS] SysGetGID() == %d", Current->gid);
	return Current->gid;
}

gid_t SysGetEGID()
{
	debug_printk("[PROCESS] SysGetEGID() == %d", Current->egid);
	return Current->egid;
}

int SysGetPGRP(void)
{
	debug_printk("[PROCESS] SysGetPGRP() == %d", Current->pgrp);
	return Current->pgrp;
}

int SysSetReGID(gid_t rgid, gid_t egid)
{
	debug_printk("[PROCESS] SysSetReGID(%d, %d)", rgid, egid);
	if (rgid > 0)
	{
		if ((Current->gid == rgid) || Current->euid == 0)
			Current->gid = rgid;
		else
			return -EPERM;
	}
	if (egid > 0)
	{
		if ((Current->gid == egid) || 
			(Current->egid == egid) ||
			(Current->sgid == egid))
			Current->egid = egid;
		else
			return -EPERM;
	}
	return 0;
}

int SysSetGID(gid_t gid)
{
	debug_printk("[PROCESS] SysSetGID(%d)", gid);
	return SysSetReGID(gid, gid);
}

int SysSetReUID(uid_t ruid, uid_t euid)
{
	debug_printk("[PROCESS] SysSetReUID(%d, %d)", ruid, euid);
	int old_ruid = Current->uid;
	
	if (ruid > 0)
	{
		if ((Current->euid == ruid) ||
			(old_ruid == ruid) ||
			(Current->euid == 0))
			Current->uid = ruid;
		else
			return -EPERM;
	}
	
	if (euid > 0)
	{
		if ((old_ruid == euid) ||
			(Current->euid == euid) ||
			Current->euid == 0)
			Current->euid = euid;
		else
		{
			Current->euid = old_ruid;
			return -EPERM;
		}
	}
	return 0;
}

int SysSetUID(uid_t uid)
{
	debug_printk("[PROCESS] SysSetUID(%d)", uid);
	return SysSetReGID(uid, uid);
}

int SysSetPGID(pid_t pid, int pgid)
{
	debug_printk("[PROCESS] SysSetPGID(%d, %d)", pid, pgid);
	int i;
	struct Process *p;
	
	if (pid == 0)
		pid = Current->pid;
	if (pgid == 0)
		pgid = Current->pid;
	for (i = 0; i < NR_PRIOPRITIES; ++i)
	{
		if (Priorities[i].head == Priorities[i].tail)
			continue;
		for (p = Priorities[i].head; p != NULL; p = p->prio_next)
			if (p->pid == pid)
				break;
		if (p != NULL)
			break;
	}
	
	if (p)
	{
		if (p->leader)
			return -EPERM;
		if (p->session != Current->session)
			return -EPERM;
		p->pgrp = pgid;
		return 0;
	}
	return -ESRCH;
}

int SysSetSID(void)
{
	debug_printk("[PROCESS] SysSetSID()");
	if (Current->leader && Current->euid != 0)
		return -EPERM;
	Current->leader = 1;
	Current->session = Current->pgrp = Current->pid;
	Current->tty = -1;
	return Current->pgrp;
}

int SysUMask(int mask)
{
	debug_printk("[PROCESS] SysUMask(%d)", mask);
	int old = Current->umask;
	Current->umask = mask & 0777;
	return old;
}
