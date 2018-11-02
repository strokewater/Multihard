#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"

typedef int sig_atomic_t;
typedef unsigned int sigset_t;

#define _NSIG		32
#define NSIG		_NSIG

#define SIGHUP		1
#define SIGINT		2
#define SIGQUIT		3
#define SIGILL		4
#define SIGTRAP		5
#define SIGABRT		6
#define SIGIOT		6
#define SIGUNUSED	7
#define SIGFPE		8
#define SIGKILL		9
#define SIGUSER1	10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17

#define SA_NOCLDSTOP	1
#define SA_NOMASK		0x40000000
#define SA_ONESHOT		0x80000000

#define SIG_BLOCK		0
#define SIG_UNBLOCK	1
#define SIG_SETMASK	2
#define SIG_DFL		((void (*)(int))1)

struct SigAction
{
	void (*sa_handler)(void);
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};


int SysSignal(int signum, addr handler, addr restorer);
int SysSigReturn();
int SysSigAction(int sig, struct SigAction *act, struct SigAction *oldact);
int SysSGetMask();
int SysSSetMask(int newmask);
int SysSigSuspend(const sigset_t *sigmask);
int SysSigPending(sigset_t *set);

void DoSignal(s32 signr);


#endif
