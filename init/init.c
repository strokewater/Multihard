#include "syscall.h"
#include "gfs/fcntl.h"
#include "gfs/stat.h"
#include "gfs/dirent.h"
#include "stdarg.h"
#include "types.h"

int errno;

_syscall3(int,open,const char *,path,int,flags,mode_t,mode);
_syscall1(int,dup,int,fd);
_syscall3(ssize_t,read,int,fd,void *,dest,size_t,cnt);
_syscall3(ssize_t,write,int,fd,const void *,dest,size_t,cnt);
_syscall0(pid_t,fork);
_syscall3(pid_t,waitpid,pid_t,pid,int *,wstatus,int,options);
_syscall1(int,close,int,fd);
_syscall1(int,exit,int,cd);
_syscall0(int,sync);
_syscall3(int,execv,const char *,program,char **,argv,char **,envs);
_syscall3(int,readdir,int,fd,struct dirent *,dirp,int,cnt);
_syscall1(int,CHDIR,const char *,pwd);
_syscall0(pid_t,SETSID);

#define NULL	((char *)0)

static char *argv[] = {"/bin/bash", NULL};
static char *envp[] = {"HOME=/", "PATH=/usr/bin", NULL};

void start_daemons()
{
	static char exec_path[255];
	const char *exec_dir = "/usr/sbin/daemons";
	int sbin = sys_open("/usr/sbin/daemons", O_RDONLY | O_DIRECTORY, 0);
	struct dirent dir;
	while (sys_readdir(sbin, &dir, 1) == 1)
	{
		int status;
		int pid = sys_fork();
		if (pid == 0)
		{
			int i, j;

			for (i = 0; exec_dir[i]; ++i)
				exec_path[i] = exec_dir[i];
			exec_path[i++] = '/';
			for (j = 0; j < dir.d_namlen; ++j)
				exec_path[i++] = dir.d_name[j];
			exec_path[i] = '\0';

			sys_execv(exec_path, argv, envp);
			sys_exit(-1);
		}
	}
	sys_close(sbin);
}

int _start()
{
	start_daemons();

	sys_SETSID();
	sys_open("/dev/tty0", O_RDWR, 0);
	sys_dup(0);
	sys_dup(0);
	
	int pid;
	int fd;
	pid = sys_fork();
	if (pid == 0)
	{
		sys_CHDIR("/");
		sys_execv("/bin/bash", argv, envp);
		sys_exit(2);
	}
	
	while (1);
}
