#ifndef TIMER_H
#define TIMER_H

#include "types.h"

struct Timer
{
	u32 act_time;
	void (*action)(void *addition_info);
	void *addition_info;
	
	struct Timer *next;
	struct Timer *prev;
};

struct Timer *TimerCreate(int act_tick_after, void (*action)(void *), void *addition_info);
void TimerDelete(struct Timer *timer);
int TimerGetActTick(struct Timer *timer);

void CheckTimer();

#endif
