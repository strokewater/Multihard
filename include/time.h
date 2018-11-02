#ifndef LIB_TIME_H
#define LIB_TIME_H

#include "types.h"

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

struct tms {
	int tms_utime;  /* user time */
	int tms_stime;  /* system time */
	int tms_cutime; /* user time of children */
	int tms_cstime; /* system time of children */
};

struct timezone
{	
	int tz_minuteswest;     /* minutes west of Greenwich */
	int tz_dsttime;         /* type of DST correction */
};

struct timeval
{
	time_t      tv_sec;     /* seconds */
	suseconds_t tv_usec;    /* microseconds */
};


extern long jiffies;
extern long startup_time;
extern struct timezone sys_tz;

#define MS_RATE_FOR_S			(1000UL)
#define US_RATE_FOR_S			(1000UL * 1000)
#define NS_RATE_FOR_S			(1000UL * 1000 * 1000)

#define	SCHED_FREQ				100
#define TSC_TAIL_TIME_NS		(NS_RATE_FOR_S * 1 / US_RATE_FOR_S)
// TSC精度为1微秒
#define TSC_FULL_TIME_NS		(NS_RATE_FOR_S * 1 / SCHED_FREQ)
// TSC补充PIT无法达到的精度
		
#define PIT_PERIOD_US		(1 * 1000 * 1000  / SCHED_FREQ )
// Timer 0 Frequency: 100 HZ, 0.01s=10ms a timer.
#define CURRENT_UTC_TIME  	(startup_time + jiffies / SCHED_FREQ)
#define CURRENT_LOCAL_TIME	(CURRENT_UTC_TIME + sys_tz.tz_minuteswest * 60)
#define CURRENT_TIME		CURRENT_UTC_TIME

void InitTime();
void CheckAlarm();

unsigned int SysAlarm(unsigned int seconds);
time_t SysTime(time_t *t);
int SysSTime(long *tptr);
int SysTimes(struct tms *tbuf);
int SysGetTimeOfDay(struct timeval *tv, struct timezone *tz);
int SysSetTimeOfDay(const struct timeval *tv, const struct timezone *tz);

#endif
