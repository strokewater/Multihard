#ifndef SYS_CALL_H
#define SYS_CALL_H

#include "types.h"
#include "asm/asm.h"
#include "asm/traps.h"

// int 0x80
// normal syscall

#define SYS_CALL_INT_NO				0x80

#define SYS_CALL_NO_setup			0
#define SYS_CALL_NO_exit			1
#define SYS_CALL_NO_fork			2
#define SYS_CALL_NO_read			3
#define SYS_CALL_NO_write			4
#define SYS_CALL_NO_open			5
#define SYS_CALL_NO_close			6
#define SYS_CALL_NO_waitpid			7
#define SYS_CALL_NO_creat			8
#define SYS_CALL_NO_link			9
#define SYS_CALL_NO_unlink			10
#define SYS_CALL_NO_execv			11
#define SYS_CALL_NO_CHDIR			12
#define SYS_CALL_NO_TIME			13
#define SYS_CALL_NO_MKNOD			14
#define SYS_CALL_NO_CHMOD			15
#define SYS_CALL_NO_CHOWN			16
#define SYS_CALL_NO_BREAK			17
#define SYS_CALL_NO_STAT			18
#define SYS_CALL_NO_LSEEK			19
#define SYS_CALL_NO_GETPID			20
#define SYS_CALL_NO_MOUNT			21
#define SYS_CALL_NO_UMOUNT			22
#define SYS_CALL_NO_SETUID			23
#define SYS_CALL_NO_GETUID			24
#define SYS_CALL_NO_STIME			25
#define SYS_CALL_NO_PTRACE			26
#define SYS_CALL_NO_ALARM			27
#define SYS_CALL_NO_FSTAT			28
#define SYS_CALL_NO_PAUSE			29
#define SYS_CALL_NO_UTIME			30
#define SYS_CALL_NO_STTY			31
#define SYS_CALL_NO_GTTY			32
#define SYS_CALL_NO_ACCESS			33
#define SYS_CALL_NO_NICE			34
#define SYS_CALL_NO_FTIME			35
#define SYS_CALL_NO_sync			36
#define SYS_CALL_NO_KILL			37
#define SYS_CALL_NO_RENAME			38
#define SYS_CALL_NO_MKDIR			39
#define SYS_CALL_NO_RMDIR			40
#define SYS_CALL_NO_dup				41
#define SYS_CALL_NO_PIPE			42
#define SYS_CALL_NO_TIMES			43
#define SYS_CALL_NO_PROF			44
#define SYS_CALL_NO_BRK				45
#define SYS_CALL_NO_SETGID			46
#define SYS_CALL_NO_GETGID			47
#define SYS_CALL_NO_SIGNAL			48
#define SYS_CALL_NO_GETEUID			49
#define SYS_CALL_NO_GETEGID			50
#define SYS_CALL_NO_ACCT			51
#define SYS_CALL_NO_PHYS			52
#define SYS_CALL_NO_LOCK			53
#define SYS_CALL_NO_IOCTL			54
#define SYS_CALL_NO_FCNTL			55
#define SYS_CALL_NO_MPX				56
#define SYS_CALL_NO_SETPGID			57
#define SYS_CALL_NO_ULIMIT			58
#define SYS_CALL_NO_UNAME			59
#define SYS_CALL_NO_UMASK			60
#define SYS_CALL_NO_CHROOT			61
#define SYS_CALL_NO_USTAT			62
#define SYS_CALL_NO_DUP2			63
#define SYS_CALL_NO_GETPPID			64
#define SYS_CALL_NO_GETPGRP			65
#define SYS_CALL_NO_SETSID			66
#define SYS_CALL_NO_SIGACTION		67
#define SYS_CALL_NO_SGETMASK		68
#define SYS_CALL_NO_SSETMASK		69
#define SYS_CALL_NO_SETREUID		70
#define SYS_CALL_NO_SETREGID		71
#define SYS_CALL_NO_SIGSUSPEND		72
#define SYS_CALL_NO_SIGPENDING		73
#define SYS_CALL_NO_GETTIMEOFDAY	78
#define SYS_CALL_NO_SETTIMEOFDAY 	79
#define SYS_CALL_NO_lstat			84
#define SYS_CALL_NO_SELECT			82
#define SYS_CALL_NO_readdir			89
#define SYS_CALL_NO_SIGRETURN		119
#define SYS_CALL_NO_getcwd			183

#define NR_SYS_CALL				255

#define _syscall0(type,name) \
static inline type sys_##name(void) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (SYS_CALL_NO_##name)); \
if (__res >= 0) \
	return (type) __res; \
errno = -__res; \
return -1; \
}

#define _syscall1(type,name,atype,a) \
static inline type sys_##name(atype a) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (SYS_CALL_NO_##name),"b" ((long)(a))); \
if (__res >= 0) \
	return (type) __res; \
errno = -__res; \
return -1; \
}

#define _syscall2(type,name,atype,a,btype,b) \
static inline type sys_##name(atype a,btype b) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (SYS_CALL_NO_##name),"b" ((long)(a)),"c" ((long)(b))); \
if (__res >= 0) \
	return (type) __res; \
errno = -__res; \
return -1; \
}

#define _syscall3(type,name,atype,a,btype,b,ctype,c) \
static inline type sys_##name(atype a,btype b,ctype c) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (SYS_CALL_NO_##name),"b" ((long)(a)),"c" ((long)(b)),"d" ((long)(c))); \
if (__res>=0) \
	return (type) __res; \
errno=-__res; \
return -1; \
}

#define _syscall4(type,name,atype,a,btype,b,ctype,c,dtype,d) \
static inline type sys_##name(atype a,btype b,ctype c,dtype d) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (SYS_CALL_NO_##name),"b" ((long)(a)),"c" ((long)(b)),"d" ((long)(c)), "S"((long)(d))); \
if (__res>=0) \
	return (type) __res; \
errno=-__res; \
return -1; \
}

#define _syscall5(type,name,atype,a,btype,b,ctype,c,dtype,d,etype,e) \
static inline type sys_##name(atype a,btype b,ctype c,dtype d,etype e) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (SYS_CALL_NO_##name),"b" ((long)(a)),"c" ((long)(b)),"d"((long)(c)), "S"((long)(d)), "D"((long)(e))); \
if (__res>=0) \
	return (type) __res; \
errno=-__res; \
return -1; \
}

extern addr SysCallTable[];

void SysCallInit(void);

#endif
