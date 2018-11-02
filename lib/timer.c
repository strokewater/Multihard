#include "timer.h"
#include "stdlib.h"
#include "sched.h"
#include "time.h"
#include "mm/varea.h"

struct Timer TimersList = {.next = &TimersList, .prev = &TimersList};

struct Timer *TimerCreate(int act_tick_after, void (*action)(void *), void *addition_info)
{
	struct Timer *ret;
	
	ret = KMalloc(sizeof(*ret));
	
	ret->act_time = jiffies + act_tick_after;
	ret->action = action;
	ret->addition_info = addition_info;
	
	struct Timer *i;
	for (i = TimersList.next; i != &TimersList && ret->act_time > i->act_time; i = i->next)
		;
	
	ret->next = i;
	ret->prev = i->prev;
	
	i->prev->next = ret;
	i->prev = ret;
	
	return ret;
}

void TimerDelete(struct Timer *timer)
{
	timer->prev->next = timer->next;
	timer->next->prev = timer->prev;
		
	KFree(timer, sizeof(*timer));
}

int TimerGetActTick(struct Timer *timer)
{
	return (timer->act_time - jiffies);
}

void CheckTimer()
{
	struct Timer *i = TimersList.next;
	while (i != &TimersList)
	{
		if (jiffies != i->act_time)
			break;
		else
		{
			i->action(i->addition_info);
			struct Timer *inext = i->next;
			TimerDelete(i);
			i = inext;
		}
	}
}
