#include "sched.h"
#include "stdlib.h"
#include "exception.h"
#include "asm/pm.h"
#include "asm/asm.h"
#include "drivers/drivers.h"
#include "drivers/i8259a/i8259a.h"
#include "drivers/terminal/rs232.h"
#include "drivers/terminal/tty.h"

void RS232InitChannel(int port)
{
	ASMOutByte(port + 3, 0x80);
	ASMOutByte(port, 0x30);
	ASMOutByte(port + 1, 0x0);
	ASMOutByte(port + 3, 0x3);
	ASMOutByte(port + 4, 0xb);
	ASMOutByte(port + 1, 0xd);
	
	ASMInByte(port);
}

void RS232Init(void)
{
	SetupIDTGate(INT_NO_IRQ_S1, SelectorFlatC, (addr)RS1IntHandle);
	SetupIDTGate(INT_NO_IRQ_S2, SelectorFlatC, (addr)RS2IntHandle);
	RS232InitChannel(RS232_CHANNEL1_BASE);
	RS232InitChannel(RS232_CHANNEL2_BASE);
	EnableIRQ(IRQ_MASTER_MASK_SERI_S1);
	EnableIRQ(IRQ_MASTER_MASK_SERI_S2);
}

void RS232Write(struct tty_struct *tty)
{
	int channel = TTY_DEV_NO_TO_TTY_NO(tty->dev) - TTYXX_NO_TTYS + 1;
	int port;
	if (channel == 1)
		port = RS232_CHANNEL1_BASE;
	else if(channel == 2)
		port = RS232_CHANNEL2_BASE;
	else
		panic("serial port is error.");
		
	ASMCli();
	if (!EMPTY(tty->write_q))
		ASMOutByte(port + 1, ASMInByte(port + 1) | 0x2);
	ASMSti();
}

void RS232ReadCharC(int port)
{
	char ch;
	struct tty_struct *tty;
	
	if (port == RS232_CHANNEL1_BASE)
		tty = ttys[TTYXX_NO_TTYS];
	else if(port == RS232_CHANNEL2_BASE)
		tty = ttys[TTYXX_NO_TTYS + 1];
	else
		panic("serial port is error.");
	
	ch = ASMInByte(port);
	PUTCH(ch, tty->read_q);
	if (FULL(tty->read_q))
		CopyToCooked(tty);
}

void RS232WriteCharC(int port)
{
	char ch;
	struct tty_struct *tty;
	
	if (port == RS232_CHANNEL1_BASE)
		tty = ttys[TTYXX_NO_TTYS];
	else if(port == RS232_CHANNEL2_BASE)
		tty = ttys[TTYXX_NO_TTYS + 1];
	else
		panic("serial port is error.");
	
	GETCH(tty->write_q, ch);
	ASMOutByte(port, ch);
	
	if (EMPTY(tty->write_q))
	{
		if (tty->write_q.proc_list != NULL)
			tty->write_q.proc_list->state = PROCESS_STATE_RUNNING;
		ASMOutByte(port + 1, ASMInByte(port + 1) & 0xd);
	}
}

void RS232ChangeSpeed(int port, unsigned speed)
{
	ASMCli();
	ASMOutByte(port + 3, 0x80);
	ASMOutByte(port, speed & 0xff);
	ASMOutByte(port + 1, speed >> 8);
	ASMSti();
}
