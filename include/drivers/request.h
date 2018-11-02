#ifndef DRIVERS_REQUEST_H
#define DRIVERS_REQUEST_H

#include "types.h"
#include "sched.h"

#define NR_DEV_REQ_ITEMS		12

#define DEV_REQ_CMD_READ		0
#define DEV_REQ_CMD_WRITE		1

struct DevRequest
{
	dev_no dev;
	
	int cmd;	// DEV_REQUEST_READ or DEV_REQUEST_WRITE
	off_t offset; // the same as the offset argument of dread of dwrite
	size_t n;
	void *data; // src or dest of DRead or DWrite
	struct Process *waitting;	// the process who make this request
};

struct DevRequestGroup
{
	struct DevRequest items[NR_DEV_REQ_ITEMS];
	int head;
	int tail;
	
	struct Process *wait_free;
};

struct DevRequestGroup *BuildDevRequestGroup();
int IsDevRequestGroupEmpty(struct DevRequestGroup *p);
void CleanDevRequestGroup(struct DevRequestGroup *p);
void FreeDevRequestGroup(struct DevRequestGroup *p);

int MakeDevRequest(int cmd, dev_no dev, void *data, off_t offset, size_t n);
void FinishDevRequest();
void WaitDevRequest(dev_no dev, int dreq_no);

#endif
