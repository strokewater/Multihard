#include "sched.h"
#include "asm/asm.h"
#include "asm/pm.h"
#include "asm/fpu_sse.h"
#include "errno.h"
#include "stdlib.h"
#include "assert.h"
#include "exception.h"
#include "mm/mm.h"
#include "mm/varea.h"

static int PriorityOrder[NR_PRIOPRITY_QUEUE] = {0, 4, 0, 1, 3, 1, 4, 1, 0, 2, 5, 0, 2};	// running.
static int CurrentPos;
struct PriorityQueue Priorities[NR_PRIOPRITIES];

struct Process Idle;
struct Process *Current;

int debug_start = 0;

void InsertPQueue(struct Process *p)
{
	int prio;
	
	assert(p, "InsertPQueue: NULL Pointer.");
	
	prio = p->priority;
	assert(p->priority >= 0 && p->priority < NR_PRIOPRITIES, "InsertPQueue: Process Prioprity out of range.");
	
	if (Priorities[prio].tail)
	{
		Priorities[prio].tail->prio_next = p;
		Priorities[prio].tail = p;
	}
	else
		Priorities[prio].tail = Priorities[prio].head = p;
	p->prio_next = NULL;
}

void RemoveProcessFromPQueue(struct Process *p)
{
	int prio;
	struct Process *i, *pi;
	prio = p->priority;
	
	for (pi = NULL, i = Priorities[prio].head; i != NULL && i != p; pi = i, i = i->prio_next)
		;
	if (p == i)
	{
		if (Priorities[prio].head != p && Priorities[prio].tail != p)
			pi->prio_next = p->prio_next;
		else
		{
			if (Priorities[prio].head == p)
			{
				Priorities[prio].head = p->prio_next;
			}
			if(Priorities[prio].tail == p)
			{
				Priorities[prio].tail = pi;
				pi->prio_next = NULL;
			}
		}
	}else
		panic("Process structure not found.");
}

struct Process *PopPQueue(int prio)
{
	struct Process *ret;
	assert(prio >= 0 && prio < NR_PRIOPRITIES, "InsertPQueue: Process Prioprity out of range.");
	
	ret = Priorities[prio].head;
	if (ret == NULL)
		return NULL;
	else
	{
		if(ret->prio_next == NULL)
			Priorities[prio].head = Priorities[prio].tail = NULL;
		else
			Priorities[prio].head = ret->prio_next;
		ret->prio_next = NULL;
	}
		
	return ret;
}

void Schedule()
{
	struct Process *i;
	int mask = SCHED_INIT_MASK;
		
	if (Current != &Idle)
		InsertPQueue(Current);
	
	++CurrentPos;
	if (CurrentPos == NR_PRIOPRITY_QUEUE)
		CurrentPos = 0;
	
	while (mask != SCHED_MASK_ALL_CHECKED)
	{
		struct Process *first;
		int found = 0;
		first = i = PopPQueue(PriorityOrder[CurrentPos]);
		if (first != NULL)
		{
			do
			{
				if (i->state == PROCESS_STATE_RUNNING)
				{
					found = 1;
					break;
				}
				InsertPQueue(i);
				i = PopPQueue(PriorityOrder[CurrentPos]);
			} while (i != first);
		}
		
		if (found)
			break;
		else
		{
			if (first != NULL)
				InsertPQueue(first);
			mask |= (1 << PriorityOrder[CurrentPos]);
			CurrentPos++;
			if (CurrentPos == NR_PRIOPRITY_QUEUE)
				CurrentPos = 0;
		}
	}
	if (mask == SCHED_MASK_ALL_CHECKED)
		i = &Idle;

	DoLaterCaseForFPUSSE();
	SwitchTo(i);
}

static void SleepUnInt(struct Process **p)
{
	struct Process *tmp;
	
	if (Current == &Idle)
		panic("idle tried to sleep.");
		
	tmp = *p;
	*p = Current;
	Current->state = PROCESS_STATE_UNINTERRUPTIBLE;
	Schedule();
		
	if (tmp)
		tmp->state = PROCESS_STATE_RUNNING;
}

static void SleepInt(struct Process **p)
{
	struct Process *tmp;
	
	if (!p)
		return;
	if (Current == &Idle)
		die("idle tried to sleep.");
	tmp = *p;
	*p = Current;
repeat: Current->state = PROCESS_STATE_INTERRUPTIBLE;
	Schedule();
	if (*p && *p != Current)
	{
		(**p).state = PROCESS_STATE_RUNNING;
		goto repeat;
	}
	*p = tmp;
	if (tmp)
		tmp->state = PROCESS_STATE_RUNNING;
}

void Sleep(struct Process **p, int type)
{
	if (type == SLEEP_TYPE_INTABLE)
		SleepInt(p);
	else if(type == SLEEP_TYPE_UNINTABLE)
		SleepUnInt(p);
}


void WakeUp(struct Process **p)
{
	if (p && *p)
	{
		(**p).state = PROCESS_STATE_RUNNING;
		*p = NULL;
	}
}

struct Process *GetProcessFromPID(pid_t pid)
{
	int i;
	struct Process *p;
	
	
	if (Current->pid == pid)
		return Current;
	
	for (i = NR_PRIOPRITIES - 1; i >= 0; --i)
	{
		p = Priorities[i].head;
		while (p)
		{
			if (p->pid == pid)
				return p;
			p = p->prio_next;
		}
	}
	
	panic("not found process whose pid match.");
	return NULL;
}

void SchedInit()
{
	ASMOutByte(0x43, 0x36);
	ASMOutByte(0x40, (1193180/SCHED_FREQ) & 0xff);
	ASMOutByte(0x40, (1193180/SCHED_FREQ) >> 8);
}

int SysPause(void)
{
	debug_printk("[PROCESS] SysPause()");
	int sig;
	int i;
	while (1)
	{
		Current->state = PROCESS_STATE_INTERRUPTIBLE;
		Schedule();
		if ((sig = (Current->signal & ~Current->blocked)))
		{
			for (i = 0; i < NSIG; ++i)
			{
				if ((sig & (1 << i)) && (u32)(Current->sigaction[i].sa_handler) != 1)
				{
					ASMCli();
					DoSignal(i + 1);
					Current->signal &= ~(1 << i);
					ASMSti();
					return -EINTR;
				}
			}
		}
	}
}

int SysNice(int increase)
{
	debug_printk("[PROCESS] SysNice(%d)", increase);
	if (increase < 0)
		return 0;
	Current->priority += increase;
	if (Current->priority >= NR_PRIOPRITIES)
		Current->priority = NR_PRIOPRITIES - 1;
	return 0;
}
