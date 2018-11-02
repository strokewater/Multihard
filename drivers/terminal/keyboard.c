#define KEYBOARD_C

#include "stdlib.h"
#include "printk.h"
#include "asm/pm.h"
#include "asm/asm.h"
#include "mm/varea.h"
#include "drivers/drivers.h"
#include "drivers/terminal/keyboard.h"
#include "drivers/terminal/tty.h"
#include "drivers/terminal/console.h"
#include "drivers/i8259a/i8259a.h"


extern void KeyboardInt();

static int CtrlMake, ShiftMake, AltMake;
static int CapsLock, NumLock, ScrollLock;
static int E0, E1;

void DoWithScanCode(unsigned char make_cd);
static void SetLEDByLockVar();
static void SetLED(int cd);

static void SendStrToTTY(const char *str);
static void SendCharToTTY(char c);
static void KBWait();
static void RestartPC();

void KBInit()
{
	SetupIDTGate(INT_NO_IRQ_KEYBOARD, SelectorFlatC, (addr)KeyboardInt);
	EnableIRQ(IRQ_MASTER_MASK_KEYBOARD);
	NumLock = 1;
	CapsLock = ScrollLock = 0;
	SetLEDByLockVar();
}
void DoWithScanCode(unsigned char make_cd)
{
	u32 key;
	
	if (make_cd == 0xe0)
	{
		E0 = 1;
		return;
	}else if(make_cd == 0xe1)
	{
		E1 = 1;
		return;
	}
	
	if (ShiftMake)
		key = KeyMap[(make_cd & ~0x80) * 3 + 1];
	else if(E0)
		key = KeyMap[(make_cd & ~0x80) * 3 + 2];
	else
		key = KeyMap[(make_cd & ~0x80) * 3];
	
	if (CapsLock)
	{
		if (key >= 'a' && key <= 'z')
			key = key - 'a' + 'A';
		else if (key >= 'A' && key <= 'Z')
			key = key - 'A' + 'a';
	} else if(CtrlMake && (key >= 0x60 && key <= 0x7f))
		key = key - 0x60 + EXT_KEY;
	if (IS_KP_NUM_DOT_KEY(make_cd) && NumLock && !ShiftMake && !E0)
		key = KeyMap[(make_cd & ~0x80) * 3 + 1];
	
	if (CtrlMake && key >= EXT_KEY_F1 && key <= EXT_KEY_F7)
	{
		int DestConsoleNo = key - EXT_KEY_F1;
		debug_printk("Catch Ctrl-F%d!", DestConsoleNo);
		//if (DestConsoleNo >= 0 && DestConsoleNo < NRCon)
		//{
		//	debug_printk("InVaild Console No!");
		//	return;
		//}
		if (DestConsoleNo != CurConsoleNo)
		{
			CurConsoleNo = DestConsoleNo;
			ResetConsoleOnScreen();
		}
		return;
	}
	
	const char *val;
	// 已经初步处理了，不是大于等于EXT_KEY的控制键，就是可显示的字符.
	if (key < EXT_KEY)
	{
		if (!(make_cd & 0x80))
			SendCharToTTY(key);
	} else
	{
		val = ExtKeyValue[key - EXT_KEY];
		if (val[0] == '_' && val[1] == '_')
		{
			switch (make_cd)
			{
				case 0x1d:
					CtrlMake = 1;
					break;
				case 0x9d:
					CtrlMake = 0;
					break;
				case 0x36: case 0x2a:
					ShiftMake = 1;
					break;
				case 0xb6: case 0xaa:
					ShiftMake = 0;
					break;
				case 0x38:
					AltMake = 1;
					break;
				case 0xb8:
					AltMake = 0;
					break;
				case 0x3a: case 0xba:
				case 0x45: case 0xc5:
				case 0x46: case 0xc6:
					SetLED(make_cd);
					break;
				case 0xdb: case 0xdc:
					RestartPC();
					break;
				default:
					// panic.
					break;
			}
		} else
		{
			if (!(make_cd & 0x80))
				SendStrToTTY(val);
		}
	}
	E0 = E1 = 0;
}

void SendStrToTTY(const char *str)
{
	int tty_no = TTYXX_NO_TTY + CurConsoleNo;
	while (*str)
	{
		if (!FULL(ttys[tty_no]->read_q))
			PUTCH(*str++, ttys[tty_no]->read_q);
	}
	CopyToCooked(ttys[tty_no]);
}

void SendCharToTTY(char c)
{
	int tty_no = TTYXX_NO_TTY + CurConsoleNo;
	if (!FULL(ttys[tty_no]->read_q))
		PUTCH(c, ttys[tty_no]->read_q);
	CopyToCooked(ttys[tty_no]);		
}

void SetLEDByLockVar()
{
	ASMOutByte(0x60, 0xed);
	debug_printk("[KBD] LED Set, Caps = %d, Num = %d, Scroll = %d", CapsLock, NumLock, ScrollLock);
	ASMOutByte(0x60, (CapsLock << 2) | (NumLock << 1) | ScrollLock);
}
void SetLED(int cd)
{
	static int *LEDLocks[] = {&NumLock, &CapsLock, &ScrollLock};
	int i;
	if ((cd & 0x7f) == 0x45)
		i = 0;
	else if((cd & 0x7f) == 0x3a)
		i = 1;
	else
		i = 2;
	
	debug_printk("%d", *(LEDLocks[i]));
	if (cd & 0x80)
		SetLEDByLockVar();
	else
		*(LEDLocks[i]) = *(LEDLocks[i]) == 0 ? 1 : 0;
}

void KBWait(int wait_type)
{
	int status;
	while (1)
	{
		status = ASMInByte(0x64);
		debug_printk("[KBD] Status Register: %x, type = %d", status, wait_type);
		if (wait_type == KB_WAIT_TYPE_IBUF_EMPTY && !(status & KB_STATUS_REG_IBUF_FULL))
			return;
		else if (wait_type == KB_WAIT_TYPE_OBUF_FULL && (status & KB_STATUS_REG_OBUF_FULL))
			return;
	}
}

void RestartPC()
{
	debug_printk("RestartPC");
	KBWait(KB_WAIT_TYPE_IBUF_EMPTY);
	ASMOutByte(0x64, 0xfe);
	while (1)
		;
}
