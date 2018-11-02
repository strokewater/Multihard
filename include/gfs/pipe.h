#ifndef PIPE_H
#define PIPE_H

int PipeEmpty(struct MInode *mi);
int PipeFull(struct MInode *mi);
ssize_t GFSReadPipe(struct File *fp, void *buf, size_t count);
ssize_t GFSWritePipe(struct File *fp, void *buf, size_t count);


#endif