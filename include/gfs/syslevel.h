#ifndef FS_FS_SYSLEVEL_H
#define FS_FS_SYSLEVEL_H

#include "types.h"
#include "gfs/stat.h"
#include "gfs/utime.h"

int SysCreat(char *name, mode_t mode);
int SysOpen(char *name, int oflags, mode_t mode);
ssize_t SysRead(int fd, void *buf, size_t count);
ssize_t SysWrite(int fd, void *buf, size_t count);
int SysClose(int fd);
int SysMount(char *dev, char *dir, char *fs_type_str, int flags);
int SysUMount(char *dir);
int SysChMod(const char *filename, mode_t mode);
int SysChOwn(const char *filename, uid_t uid, gid_t gid);
int SysAccess(const char *filename, int mode);
int SysStat(const char *filename, struct stat *buf);
int SysFStat(int fd, struct stat *buf);
int SysLseek(int fd, off_t offset, int origin);
int SysLStat(const char *filename, struct stat *buf);
int SysMkNod(const char *path, mode_t mode, dev_no dev);
int SysMkDir(const char *path, int mode);
int SysRmDir(const char *path);
int SysReadDir(int fd, struct dirent *dirp, int cnt);
int SysLink(const char *old_path, const char *new_path);
int SysUnLink(const char *path);
int SysChDir(const char *filename);
int SysUTime(const char *filename, struct utime_buf *times);
int SysIoctl(int fd, unsigned long cmd, unsigned long arg);
int SysFcntl(int fd, int cmd, int arg);
int SysDup(int fd);
int SysDup2(int fd, int newfd);
int SysPipe(int fds[]);
int SysChRoot(const char *filename);
char *SysGetCWD(char *buf, size_t size);

#endif
