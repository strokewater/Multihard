#ifndef GFS_RW_H
#define GFS_RW_H

ssize_t GFSReadCharDev(struct File *fp, void *buf, size_t count);
ssize_t GFSWriteCharDev(struct File *fp, void *buf, size_t count);

ssize_t GFSReadBlockDev(struct File *fp, void *buf, size_t count);
ssize_t GFSWriteBlockDev(struct File *fp, void *buf, size_t count);

ssize_t GFSReadPipe(struct File *fp, void *buf, size_t count);
ssize_t GFSWritePipe(struct File *fp, void *buf, size_t count);

#endif