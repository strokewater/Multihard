#ifndef SYS_WAIT_H
#define SYS_WAIT_H

#define WNOHANG			1
#define WUNTRACED		2

#define WIFEXITED(s)	(! ((s) & 0xff)))
#define WIFSTOPPED(s)	(((s) & 0xff) == 0x7f)
#define WEXITSTATUS(s)	(((s)>>8) & 0xff)
#define WTERMSIG(s)		((s) & 0x7f)
#define WSTOPSIG(s)		(((s) >> 8) & 0xff)
#define WIFSIGNALED(s)	(((unsigned int)(s)-1 & 0xffff) < 0xff)

int SysWaitPID(pid_t pid, long *stat_addr, int options);

#endif
