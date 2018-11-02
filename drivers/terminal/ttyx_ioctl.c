#include "string.h"
#include "errno.h"
#include "asm/asm.h"
#include "gfs/select.h"
#include "drivers/terminal/tty.h"
#include "drivers/terminal/rs232.h"
#include "drivers/terminal/console.h"
#include "drivers/drivers.h"

static void change_speed(struct tty_struct *tty)
{
	int channel = TTY_DEV_NO_TO_TTY_NO(tty->dev) - TTYXX_NO_TTYS + 1;
	int port;

	if (channel == 1)
		port = RS232_CHANNEL1_BASE;
	else if(channel == 2)
		port = RS232_CHANNEL2_BASE;
	else
		return;	// no searil terminal.
	
	static unsigned short quotient[] = {
		0, 2304, 1536, 1047, 857,
		768, 576, 384, 192, 96,
		64, 48, 24, 12, 6, 3
	};
	RS232ChangeSpeed(port, quotient[tty->termios.c_cflag & CBAUD]);
}

static void flush(struct tty_queue * queue)
{
	ASMCli();
	queue->head = queue->tail;
	ASMSti();
}

static void wait_until_sent(struct tty_struct * tty)
{
	/* do nothing - not implemented */
}

static void send_break(struct tty_struct * tty)
{
	/* do nothing - not implemented */
}

static int get_termios(struct tty_struct * tty, struct termios * termios)
{
	VerifyArea(termios, sizeof (*termios));
	memcpy(termios, &(tty->termios), sizeof(*termios));
	return 0;
}

static int set_termios(struct tty_struct * tty, struct termios * termios)
{
	memcpy(&(tty->termios), termios, sizeof(*termios));
	return 0;
}

static int get_termio(struct tty_struct * tty, struct termio * termio)
{
	int i;
	struct termio tmp_termio;

	VerifyArea(termio, sizeof (*termio));
	
	for (i=0 ; i< (sizeof (*termio)) ; i++)
		((char *)&tmp_termio)[i]=((char *)termio)[i];
	tmp_termio.c_iflag = *(unsigned short *)&tty->termios.c_iflag;
	tmp_termio.c_oflag = *(unsigned short *)&tty->termios.c_oflag;
	tmp_termio.c_cflag = *(unsigned short *)&tty->termios.c_cflag;
	tmp_termio.c_lflag = *(unsigned short *)&tty->termios.c_lflag;
	tmp_termio.c_line = tty->termios.c_line;
	for(i=0 ; i < NCC ; i++)
		tmp_termio.c_cc[i] = tty->termios.c_cc[i];
	memcpy(termio, &tmp_termio, sizeof(tmp_termio));

	return 0;
}

/*
 * This only works as the 386 is low-byt-first
 */
static int set_termio(struct tty_struct * tty, struct termio * termio)
{
	int i;
	struct termio tmp_termio;

	for (i=0 ; i< (sizeof (*termio)) ; i++)
		((char *)&tmp_termio)[i]=((char *)termio)[i];
	*(unsigned short *)&tty->termios.c_iflag = tmp_termio.c_iflag;
	*(unsigned short *)&tty->termios.c_oflag = tmp_termio.c_oflag;
	*(unsigned short *)&tty->termios.c_cflag = tmp_termio.c_cflag;
	*(unsigned short *)&tty->termios.c_lflag = tmp_termio.c_lflag;
	tty->termios.c_line = tmp_termio.c_line;
	for(i=0 ; i < NCC ; i++)
		tty->termios.c_cc[i] = tmp_termio.c_cc[i];
	change_speed(tty);
	return 0;
}

int DTTYXIoctl(dev_no dev, int cmd, unsigned int arg)
{
	struct winsize win;
	struct tty_struct * tty;
	int tty_no = TTY_DEV_NO_TO_TTY_NO(dev);
	if (tty_no == 0)
		tty_no = TTYXX_NO_TTY + CurConsoleNo;
	tty = ttys[tty_no];
	
	switch (cmd) {
		case TCGETS:
			return get_termios(tty,(struct termios *) arg);
		case TCSETSF:
			flush(&tty->read_q); /* fallthrough */
		case TCSETSW:
			wait_until_sent(tty); /* fallthrough */
		case TCSETS:
			return set_termios(tty,(struct termios *) arg);
		case TCGETA:
			return get_termio(tty,(struct termio *) arg);
		case TCSETAF:
			flush(&tty->read_q); /* fallthrough */
		case TCSETAW:
			wait_until_sent(tty); /* fallthrough */
		case TCSETA:
			return set_termio(tty,(struct termio *) arg);
		case TCSBRK:
			if (!arg) {
				wait_until_sent(tty);
				send_break(tty);
			}
			return 0;
		case TCXONC:
			return -EINVAL; /* not implemented */
		case TCFLSH:
			if (arg==0)
				flush(&tty->read_q);
			else if (arg==1)
				flush(&tty->write_q);
			else if (arg==2) {
				flush(&tty->read_q);
				flush(&tty->write_q);
			} else
				return -EINVAL;
			return 0;
		case TIOCEXCL:
			return -EINVAL; /* not implemented */
		case TIOCNXCL:
			return -EINVAL; /* not implemented */
		case TIOCSCTTY:
			return -EINVAL; /* set controlling term NI */
		case TIOCGPGRP:
			VerifyArea((void *) arg,4);
			*(unsigned long *)arg = tty->pgrp;
			return 0;
		case TIOCSPGRP:
			tty->pgrp = *(unsigned long *)arg;
			return 0;
		case TIOCOUTQ:
			VerifyArea((void *) arg,4);
			*(unsigned long *)arg = CHARS(tty->write_q);
			return 0;
		case TIOCINQ:
			VerifyArea((void *) arg,4);
			*(unsigned long *) arg = CHARS(tty->secondary);
			return 0;
		case TIOCSTI:
			return -EINVAL; /* not implemented */
		case TIOCGWINSZ:
			win.ws_row = 25;
			win.ws_col = 80;
			win.ws_xpixel = 25 * 8;
			win.ws_ypixel = 25 * 8;
			*(struct winsize *)arg = win;
			return 0; /* not implemented */
		case TIOCSWINSZ:
			return -EINVAL; /* not implemented */
		case TIOCMGET:
			return -EINVAL; /* not implemented */
		case TIOCMBIS:
			return -EINVAL; /* not implemented */
		case TIOCMBIC:
			return -EINVAL; /* not implemented */
		case TIOCMSET:
			return -EINVAL; /* not implemented */
		case TIOCGSOFTCAR:
			return -EINVAL; /* not implemented */
		case TIOCSSOFTCAR:
			return -EINVAL; /* not implemented */
		default:
			return -EINVAL;
	}
}

int DTTYXSelect(dev_no dev, enum SelectCheckType check_type, select_table *tbl)
{
	unsigned tty_no = TTY_DEV_NO_TO_TTY_NO(dev);
	if (tty_no == 0)
		tty_no = TTYXX_NO_TTY + CurConsoleNo;

	struct tty_struct *tty = ttys[tty_no];

	if (check_type == SELECT_EXCEPT)
		return 0;
	else if (check_type == SELECT_READ)
	{
		if (EMPTY(tty->secondary))
		{
			add_wait(&(tty->secondary.proc_list), tbl);
			return 0;
		} else
			return 1;
	} else
	{
		if (EMPTY(tty->write_q))
		{
			add_wait(&(tty->write_q.proc_list), tbl);
			return 0;
		} else
			return 1;
	}
}
