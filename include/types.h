#ifndef LIB_TYPE_H
#define LIB_TYPE_H

typedef unsigned long  	u32;
typedef signed long		s32;
typedef unsigned short	u16;
typedef signed short	s16;
typedef unsigned char 	u8;
typedef signed char		s8;
typedef u32				addr;

typedef s32 suseconds_t;
typedef u32 dev_no;
typedef s32 pid_t;
typedef u32 off_t;
typedef u32 size_t;
typedef s32 ssize_t;
typedef s32 time_t;
typedef s32 mode_t;
typedef u32 umode_t;
typedef u32 uid_t;
typedef u32 gid_t;	// should be u8 ?
typedef u32 ino_t;
typedef u16 nlink_t;
typedef unsigned int fd_set;

#endif
