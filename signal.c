#include "signal.h"
#include "sched.h"
#include "types.h"
#include "stdlib.h"
#include "exception.h"
#include "asm/int_content.h"

void DoSignal(s32 signr)
{
	u32 sa_handler;
	u32 old_eip = CurIntContent->eip;
	struct SigAction *sa = Current->sigaction + signr - 1;
	s32 longs;
	u32 *tmp_esp;
		
	sa_handler = (u32)sa->sa_handler;
	
	debug_printk("[SIGNAL] Do Signal(signr = %d, handler = %x)", signr, sa_handler);
	
	if (sa_handler == 1)
		return;
	if (!sa_handler)
	{
		if (signr == SIGCHLD)
			return;
		else
			die("Signal defualt.");
	}
	
	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;
	CurIntContent->eip = sa_handler;
	longs = (sa->sa_flags & SA_NOMASK) ? 7 : 8;
	CurIntContent->esp -= longs;
	
	tmp_esp = CurIntContent->esp;
	*tmp_esp++ = (u32)sa->sa_restorer;
	*(s32 *)(tmp_esp++) = signr;
	if (!(sa->sa_flags & SA_NOMASK))
		*tmp_esp++ = Current->blocked;
	*tmp_esp++ = CurIntContent->eax;
	*tmp_esp++ = CurIntContent->ecx;
	*tmp_esp++ = CurIntContent->edx;
	*tmp_esp++ = CurIntContent->eflags;
	*tmp_esp++ = old_eip;
	Current->blocked |= sa->sa_mask;
}

int SysSigReturn()
{
	int *sig_content = (int *)CurIntContent->esp;
	Current->blocked = *sig_content++;
	int eax = *sig_content++;
	CurIntContent->ecx = *sig_content++;
	CurIntContent->edx = *sig_content++;
	CurIntContent->eflags = *sig_content++;
	CurIntContent->eip = *sig_content;
	
	return eax;
}

int SysSignal(int signum, addr handler, addr restorer)
{
	debug_printk("[SIGNAL] SysSignal(%d, %x, %x)", signum, handler, restorer);
	if ((signum < 1 && signum > NSIG) || signum == SIGKILL)
		return -1;
	Current->sigaction[signum - 1].sa_flags = SA_ONESHOT | SA_NOMASK;
	Current->sigaction[signum - 1].sa_mask = 0;
	Current->sigaction[signum - 1].sa_handler = (void (*)())handler;
	Current->sigaction[signum - 1].sa_restorer = (void (*)())restorer;
	
	return handler;
}

int SysSigAction(int signum, struct SigAction *action, struct SigAction *oldaction)
{
	debug_printk("[SIGNAL] SysSigAction(%d, %x, %x)", signum, action, oldaction);
	if (signum < 1 || signum > 32 || signum == SIGKILL)
		return -1;
	if (oldaction)
		*oldaction = Current->sigaction[signum - 1];
	Current->sigaction[signum - 1] = *action;
	if (Current->sigaction[signum - 1].sa_flags & SA_NOMASK)
		Current->sigaction[signum - 1].sa_mask = 0;
	else
		Current->sigaction[signum - 1].sa_mask |= (1 << (signum - 1));
	return 0;
}

int SysSGetMask()
{
	debug_printk("[SIGNAL] SysSGetMask() == %x", Current->blocked);
	return Current->blocked;
}

int SysSSetMask(int newmask)
{
	debug_printk("[SIGNAL] SysSSetMask(%x) == %x", newmask, Current->blocked);
	int old = Current->blocked;
	Current->blocked = newmask & (1 << (SIGCHLD - 1));
	return old;
}


int SysSigPending(sigset_t *set)
{
	debug_printk("[SIGNAL] SysSigPending(%x)", set);
	*set = Current->blocked & Current->signal;
	return 0;
}


int SysSigSuspend(const sigset_t *sigmask)
{
	debug_printk("[SIGNAL] SysSigSuspend(%x)", sigmask);
	sigset_t old_mask = Current->blocked;
	Current->blocked = *sigmask;
	SysPause();
	Current->blocked = old_mask;

	return 0;
}
