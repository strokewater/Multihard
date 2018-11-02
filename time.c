#include "time.h"
#include "sched.h"
#include "stdlib.h"
#include "types.h"
#include "errno.h"
#include "printk.h"
#include "asm/asm.h"
#include "asm/tsc.h"

long jiffies = 0;
long startup_time = 0;
struct timezone sys_tz = {.tz_minuteswest = 0, .tz_dsttime = 0};

int IsLeapYear(int year)
{
	if (year % 4 == 0 && year % 100 != 0)
		return 1;
	else if (year % 400 == 0)
		return 1;
	else
		return 0;
}

int ReadCMOS(int addr)
{
	ASMOutByte(0x70, addr);
	return ASMInByte(0x71);
}

int BCDToBin(int p)
{
	return (p >> 4) * 10 + (p & 0xf);
}

int leap_days[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
int no_leap_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void InitTime()
{
	struct tm time;
	int century;
	int year;	
	int month;
	int day;
	
	int ndays = 0;
	int *days_of_month;
	do
	{
		time.tm_sec = ReadCMOS(0);
		time.tm_min = ReadCMOS(2);
		time.tm_hour = ReadCMOS(4);
		time.tm_mday = ReadCMOS(7);
		time.tm_mon = ReadCMOS(8);
		time.tm_year = ReadCMOS(9);
		century = ReadCMOS(0x32);
	} while (time.tm_sec != ReadCMOS(0));
	
	InitTSC(TSC_TAIL_TIME_NS, TSC_FULL_TIME_NS);
	
	time.tm_sec = BCDToBin(time.tm_sec);
	time.tm_min = BCDToBin(time.tm_min);
	time.tm_hour = BCDToBin(time.tm_hour);
	time.tm_mday = BCDToBin(time.tm_mday);
	time.tm_mon = BCDToBin(time.tm_mon);
	time.tm_year = BCDToBin(time.tm_year);
	century = BCDToBin(century);
	time.tm_year = century * 100 + time.tm_year;
	time.tm_mon--;
	

	for (year = 1970; year < time.tm_year; ++year)
	{
		if (IsLeapYear(year))
			ndays += 366;
		else
			ndays += 365;
	}
	if (IsLeapYear(time.tm_year))
		days_of_month = leap_days;
	else
		days_of_month = no_leap_days;
	for (month = 0; month < time.tm_mon; ++month)
		ndays += days_of_month[month];
	for (day = 1; day < time.tm_mday; ++day)
		ndays++;
	startup_time = ndays * 24 * 60 * 60 + (time.tm_hour * 60 + time.tm_min) * 60 + time.tm_sec;
	debug_printk("[TIME] OS Start Time(UTC) = %x.", startup_time);
}

void DoAlarm(void *addition_info)
{
	struct Process *p = (struct Process *)addition_info;
	p->signal |= ( 1 << (SIGALRM - 1));
	p->alarm = 0;
	if (p->state == PROCESS_STATE_INTERRUPTIBLE)
		p->state = PROCESS_STATE_RUNNING;
}

unsigned int SysAlarm(unsigned int seconds)
{
	unsigned int ret;
	debug_printk("[ALARM] Process %s(PID = %d) Do SysAlarm(%d).", Current->name, Current->pid, seconds);

	if (Current->alarm == NULL)
		ret = 0;
	else
		ret = TimerGetActTick(Current->alarm) / SCHED_FREQ;
	
	if (seconds == 0)
	{
		if (Current->alarm != NULL)
			TimerDelete(Current->alarm);
		Current->alarm = NULL;
	} else
	{
		Current->alarm = TimerCreate(seconds * SCHED_FREQ, DoAlarm, (void *)Current);
	}
		
	return ret;
}

time_t SysTime(time_t *t)
{
	if (t != NULL)
	debug_printk("[TIME] SysTime(%x)", *t);
	if (t)
		*t = CURRENT_TIME;
	return CURRENT_TIME;
}

int SysSTime(long *tptr)
{
	if (tptr != NULL)
		debug_printk("[TIME] SysTime(%x)", *tptr);
	if (Current->euid != 0)
		return -EPERM;
	startup_time = *tptr - jiffies / SCHED_FREQ;
	return 0;
}

int SysTimes(struct tms *tbuf)
{
	debug_printk("[TIME] SysTimes(%x)", tbuf);
	if (tbuf)
	{
		tbuf->tms_utime = Current->utime;
		tbuf->tms_stime = Current->stime;
		tbuf->tms_cutime = Current->cutime;
		tbuf->tms_cstime = Current->cstime;
	}
	return jiffies;
}

int SysGetTimeOfDay(struct timeval *tv, struct timezone *tz)
{
	if (tv)
	{
		tv->tv_sec = CURRENT_UTC_TIME;
		tv->tv_usec = CURRENT_UTC_TIME + GetTSCTailTime() * TSC_TAIL_TIME_NS * US_RATE_FOR_S / NS_RATE_FOR_S;
	}
	if (tz)
		*tz = sys_tz;
	debug_printk("[TIME] SysGetTimeofDay() finish!");
	return 0;
}

int SysSetTimeOfDay(const struct timeval *tv, const struct timezone *tz)
{
	if (tv)
	{
		int sec_diff = tv->tv_sec - CURRENT_UTC_TIME;
		int us_diff = tv->tv_usec - (CURRENT_UTC_TIME + GetTSCTailTime() * TSC_TAIL_TIME_NS * US_RATE_FOR_S / NS_RATE_FOR_S);
		startup_time += sec_diff;
		SetStartupTailTime(us_diff * NS_RATE_FOR_S / US_RATE_FOR_S  / TSC_TAIL_TIME_NS);
	}
	if (tz)
		sys_tz = *tz;
	
	debug_printk("[TIME] SysSetTimeofDay() finish!");
	return 0;
}
