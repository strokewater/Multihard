#ifndef GFS_SELECT_H
#define GFS_SELECT_H

#include "limits.h"
#include "time.h"

typedef struct str_wait_entry{
	struct Process * old_task;
	struct Process ** wait_address;
} wait_entry;

typedef struct str_select_tbl{
	int nr;
	wait_entry entry[NR_OPEN*3];
} select_table;

void add_wait(struct Process ** wait_address, select_table * p);
void free_wait(select_table * p);

enum SelectCheckType
{
	SELECT_READ,
	SELECT_WRITE,
	SELECT_EXCEPT
};

int SysSelect(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *tvp);

#endif
