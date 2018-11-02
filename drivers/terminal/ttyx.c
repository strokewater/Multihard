#include "string.h"
#include "stdlib.h"
#include "ctype.h"
#include "errno.h"
#include "sched.h"
#include "printk.h"
#include "asm/asm.h"
#include "mm/varea.h"
#include "drivers/drivers.h"
#include "drivers/terminal/tty.h"
#include "drivers/terminal/keyboard.h"
#include "drivers/terminal/console.h"
#include "drivers/terminal/rs232.h"

struct tty_struct *ttys[MAX_TTY];

static void TTYSendIntr(struct tty_struct *tty, int mask)
{
	int i;
	struct Process *p;

	if (tty->pgrp <= 0)
		return;
	for (i = 0; i < NR_PRIOPRITIES; ++i)
	{
		if (Priorities[i].head == NULL)
			continue;
		for (p = Priorities[i].head; p; p = p->prio_next)
		{
			if (p->pgrp == tty->pgrp)
				p->signal |= mask;
		}
	}
}

static void SleepIfEmpty(struct tty_queue * queue)
{
	ASMCli();
	while (!Current->signal && EMPTY(*queue))
		Sleep(&(queue->proc_list), SLEEP_TYPE_INTABLE);
	ASMSti();
}

static void SleepIfFull(struct tty_queue * queue)
{
	if (!FULL(*queue))
		return;
	ASMCli();
	while (!Current->signal && LEFT(*queue)<128)
		Sleep(&(queue->proc_list), SLEEP_TYPE_INTABLE);
	ASMSti();
}

static void WaitForKeyPress(struct tty_struct *tty)
{
	while (1)
	{
		int nchars = CHARS(tty->secondary);
		Sleep(&(tty->secondary.proc_list), SLEEP_TYPE_INTABLE);
		if (nchars != CHARS(tty->secondary))
			break;
	}
}

void CopyToCooked(struct tty_struct * tty)
{
	signed char c;

	while (!EMPTY(tty->read_q) && !FULL(tty->secondary)) {
		GETCH(tty->read_q,c);
		if (c==13)
			if (I_CRNL(tty))
				c=10;
			else if (I_NOCR(tty))
				continue;
			else ;
		else if (c==10 && I_NLCR(tty))
			c=13;
		if (I_UCLC(tty))
			c=tolower(c);
		if (L_CANON(tty)) {
			if (c==KILL_CHAR(tty)) {
				/* deal with killing the input line */
				while(!(EMPTY(tty->secondary) ||
				        (c=LAST(tty->secondary))==10 ||
				        c==EOF_CHAR(tty))) {
					if (L_ECHO(tty)) {
						if (c<32)
							PUTCH(127,tty->write_q);
						PUTCH(127,tty->write_q);
						tty->write(tty);
					}
					DEC(tty->secondary.head);
				}
				continue;
			}
			if (c==ERASE_CHAR(tty)) {
				if (EMPTY(tty->secondary) ||
				   (c=LAST(tty->secondary))==10 ||
				   c==EOF_CHAR(tty))
					continue;
				if (L_ECHO(tty)) {
					if (c<32)
						PUTCH(127,tty->write_q);
					PUTCH(127,tty->write_q);
					tty->write(tty);
				}
				DEC(tty->secondary.head);
				continue;
			}
			if (c==STOP_CHAR(tty)) {
				tty->stopped=1;
				continue;
			}
			if (c==START_CHAR(tty)) {
				tty->stopped=0;
				continue;
			}
		}
		if (L_ISIG(tty)) {
			if (c==INTR_CHAR(tty)) {
				TTYSendIntr(tty,INTMASK);
				continue;
			}
			if (c==QUIT_CHAR(tty)) {
				TTYSendIntr(tty,QUITMASK);
				continue;
			}
		}
		if (L_ECHO(tty)) {
			if (c==10) {
				PUTCH(10,tty->write_q);
				PUTCH(13,tty->write_q);
			} else if (c<32) {
				if (L_ECHOCTL(tty)) {
					PUTCH('^',tty->write_q);
					PUTCH(c+64,tty->write_q);
				}
			} else
				PUTCH(c,tty->write_q);
			tty->write(tty);
		}
		PUTCH(c,tty->secondary);
	}
	WakeUp(&(tty->secondary.proc_list));
}


#define TTY_RW_CHECK_SIGNAL	do \
							{ \
								if (Current->signal & ~Current->blocked) \
								{ \
									ret = nreaded == 0 ? -EINTR : nreaded; \
									goto RetCleanRes; \
								} \
							} while (0)

static ssize_t DTTYXReadCanon(struct tty_struct *tty, char *buf, size_t size)
{
	char ch;
	int nreaded = 0;
	ssize_t ret;
	while (nreaded < size)
	{
		TTY_RW_CHECK_SIGNAL;
		if (EMPTY(tty->secondary))
		{
				WaitForKeyPress(tty);
				continue;
		}
		while (!EMPTY(tty->secondary))
		{
			GETCH(tty->secondary, ch);
			if (nreaded >= size)
			{
				ret = nreaded;
				goto RetCleanRes;
			} else
			{
				if(ch == EOF_CHAR(tty))
				{
					ret = nreaded;
					goto RetCleanRes;
				} else if(ch == EOL_CHAR(tty) || ch == EOL2_CHAR(tty) || ch == 10 ||
						(I_CRNL(tty) && I_NOCR(tty) && ch == 13))
				{
					*buf = ch;
					ret = ++nreaded;
					goto RetCleanRes;
				}
				*buf++ = ch;
				++nreaded;
			}
		}
	}
RetCleanRes:
	return ret;
}

struct TTYTimeOutInfo
{
	struct Process *p;
	int flag;
};

void TTYTimeOutAct(void *res)
{
	struct TTYTimeOutInfo *p = (struct TTYTimeOutInfo *)res;
	if (p->p->state == PROCESS_STATE_INTERRUPTIBLE)
		p->p->state = PROCESS_STATE_RUNNING;
	p->flag = 1;
}

static ssize_t DTTYXReadRaw(struct tty_struct *tty, char *buf, size_t size)
{
	int min_ch;
	int time;
	int ch;
	int nreaded = 0;
	size_t ret;
	
	min_ch = tty->termios.c_cc[VMIN];
	time = tty->termios.c_cc[VTIME];

	struct Timer *read_timer = NULL;
	struct TTYTimeOutInfo *info = NULL;
	
	TTY_RW_CHECK_SIGNAL;
	if (min_ch > 0 && time > 0)
	{
		while (EMPTY(tty->secondary))
			SleepIfEmpty(&(tty->secondary));
		TTY_RW_CHECK_SIGNAL;
		if (!EMPTY(tty->secondary))
		{
			GETCH(tty->secondary, ch);
			*buf++ = ch;
			++nreaded;
		}
		
		info = KMalloc(sizeof(*info));
		info->flag = 0;
		info->p = Current;
		read_timer = TimerCreate(time * SCHED_FREQ / 10, TTYTimeOutAct, (void *)info);
		
		while (nreaded < min_ch)
		{
			if (info->flag)
				break; // times up, return now.
			
			// check signals including alarm set by user.
			TTY_RW_CHECK_SIGNAL;
			if (EMPTY(tty->secondary))
			{
				SleepIfEmpty(&(tty->secondary));
				continue;
			}
			while (!EMPTY(tty->secondary) && nreaded < min_ch)
			{
				GETCH(tty->secondary, ch);
				*buf++ = ch;
				++nreaded;
			}
		}
		ret = nreaded;
		goto RetCleanRes;
	} else if(min_ch > 0 && time == 0)
	{
		while (nreaded < min_ch)
		{
			TTY_RW_CHECK_SIGNAL;
			if (EMPTY(tty->secondary))
			{
				SleepIfEmpty(&(tty->secondary));
				continue;
			}
			while (!EMPTY(tty->secondary) && nreaded < min_ch)
			{
				GETCH(tty->secondary, ch);
				*buf++ = ch;
				++nreaded;
			}
		}
		ret = nreaded;
		goto RetCleanRes;
	} else if(min_ch == 0 && time > 0)
	{
		info = KMalloc(sizeof(*info));
		info->flag = 0;
		info->p = Current;
		read_timer = TimerCreate(time * SCHED_FREQ / 10, TTYTimeOutAct, (void *)info);

		while (EMPTY(tty->secondary))
		{
			SleepIfEmpty(&(tty->secondary));
			if (info->flag == 1)
				break;
			TTY_RW_CHECK_SIGNAL;
		}

		if (info->flag == 0)
		{
			GETCH(tty->secondary, ch);
			*buf = ch;
		}

		nreaded = info->flag == 0 ? 1 : 0;
		ret = nreaded;
		goto RetCleanRes;
	} else
	{
		while (!EMPTY(tty->secondary))
		{
			GETCH(tty->secondary, ch);
			*buf++ = ch;
			++nreaded;
		}
		ret = nreaded;
		goto RetCleanRes;
	}
	
RetCleanRes:
	if (info != NULL)
	{
		if (info->flag == 0)
		{
			ASMCli();
			TimerDelete(read_timer);
			ASMSti();
		}
		KFree(info, sizeof(*info));
	}
	return ret;
}


static ssize_t DTTYXRead(dev_no dev, void *buf, off_t offset, size_t size)
{
	int tty_no;

	if (DEV_NO_GET_MINOR_NO(dev) == 0)
		tty_no = TTYXX_NO_TTY + CurConsoleNo;	// /dev/tty0
	else
 		tty_no = TTY_DEV_NO_TO_TTY_NO(dev) - 1;

	struct tty_struct *tty = ttys[tty_no];
	
	if (L_CANON(tty))
		return DTTYXReadCanon(tty, (char *)buf, size);
	else
		return DTTYXReadRaw(tty, (char *)buf, size);
}


static ssize_t DTTYXWrite(dev_no dev, const void *vbuf, off_t offset, size_t nr)
{
	static int cr_flag=0;
	struct tty_struct * tty;
	const char *buf = vbuf;
	char c;
	const char *b = buf;

	int tty_no;

	if (DEV_NO_GET_MINOR_NO(dev) == 0)
		tty_no = TTYXX_NO_TTY + CurConsoleNo;	// /dev/tty0
	else
 		tty_no = TTY_DEV_NO_TO_TTY_NO(dev);

	tty = ttys[tty_no];
	while (nr>0) {
		SleepIfFull(&tty->write_q);
		if (Current->signal)
			break;
		while (nr>0 && !FULL(tty->write_q)) {
			c=*b;
			if (O_POST(tty)) {
				if (c=='\r' && O_CRNL(tty))
					c='\n';
				else if (c=='\n' && O_NLRET(tty))
					c='\r';
				if (c=='\n' && !cr_flag && O_NLCR(tty)) {
					cr_flag = 1;
					PUTCH(13,tty->write_q);
					continue;
				}
				if (O_LCUC(tty))
					c=toupper(c);
			}
			b++; nr--;
			cr_flag = 0;
			PUTCH(c,tty->write_q);
		}
		tty->write(tty);
		if (nr>0)
			Schedule();
	}
	return (b-buf);
}

static int DTTYXInit(dev_no dev)
{
	return 0;
}

static int DTTYXUnInit(dev_no dev)
{
	return 0;
}

static int DTTYXOpen(dev_no dev, struct File *fp, int flags)
{
	if (Current->leader && Current->tty == 0)
	{
		Current->tty = TTY_DEV_NO_TO_TTY_NO(fp->inode->zone[0]);
		if (Current->tty == 0)
			Current->tty = TTYXX_NO_TTY + CurConsoleNo;
		ttys[Current->tty]->pgrp = Current->pgrp;
	}
	return 0;
}

static int DTTYXClose(dev_no dev, struct File *fp)
{
	return 0;
}

#include "ttyx_ioctl.h"

struct MajorDevice ttyx_major = {.major = MAJOR_DEV_NO_TTYX, .status = DEV_STATUS_UNINIT,
								.name = "ttyx"};
struct MinorDevice ttyx_minor_module = {.minor = 0, .status = DEV_STATUS_UNINIT,
								.name = "ttyX", .type = DEV_TYPE_CHAR,
								.DInit = DTTYXInit, .DRead = DTTYXRead, 
								.DWrite = DTTYXWrite, .DUnInit = DTTYXUnInit,
								.DIoctl = DTTYXIoctl, .DSelect = DTTYXSelect,
								.DOpen = DTTYXOpen, .DClose = DTTYXClose};

void TTYXInit()
{
	dev_no dno;
	int i;
	char tmp[4];
	KBInit();
	ConInit();
	RS232Init();

	RegisterMajorDevice(&ttyx_major);
	strcpy(ttyx_minor_module.name + 3, "0");
	RegisterMinorDevice(DEV_NO_MAKE(MAJOR_DEV_NO_TTYX, 0), &ttyx_minor_module);
	DInit(DEV_NO_MAKE(MAJOR_DEV_NO_TTYX, 0));
	
	for (i = 0; i < NRCon; ++i)
	{
		dno = TTY_NO_TO_DEV_NO(TTYXX_NO_TTY + i);
		ttyx_minor_module.minor = DEV_NO_GET_MINOR_NO(dno);
		itoa(i + 1, tmp);
		strcpy(ttyx_minor_module.name + 3, tmp);
		RegisterMinorDevice(dno, &ttyx_minor_module);
		DInit(dno);
		ttys[TTYXX_NO_TTY + i] = KMalloc(sizeof(*(ttys[0])));
		*(ttys[TTYXX_NO_TTY + i]) = (struct tty_struct) {
				dno,
				{	ICRNL,
					OPOST | ONLCR,
					0,
					ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,
					0,
					INIT_C_CC},
				0,
				0,
				ConWrite,
				{0,0,0,""},
				{0,0,0,""},
				{0,0,0,""}
			};
	}

	for (i = 0;i < NR_RS232_CHANEEL; ++i)
	{
		dno = TTY_NO_TO_DEV_NO(TTYXX_NO_TTYS + i);
		ttyx_minor_module.minor = DEV_NO_GET_MINOR_NO(dno);
		itoa(i, tmp);
		ttyx_minor_module.name[3] = 'S';
		strcpy(ttyx_minor_module.name + 4, tmp);
		RegisterMinorDevice(dno, &ttyx_minor_module);
		DInit(dno);
		ttys[TTYXX_NO_TTYS + i] = KMalloc(sizeof(*(ttys[0])));
		*(ttys[TTYXX_NO_TTYS + i]) = (struct tty_struct) {
				dno,
				{	0,
					0,
					B2400 | CS8,
					0,
					0,
					INIT_C_CC},
				0,
				0,
				RS232Write,
				{0,0,0,""},
				{0,0,0,""},
				{0,0,0,""}
			};
	}
}
