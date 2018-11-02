#include "types.h"
#include "stdlib.h"
#include "exception.h"
#include "printk.h"
#include "asm/asm.h"
#include "mm/varea.h"
#include "mm/darea.h"
#include "drivers/terminal/console.h"
#include "drivers/terminal/vbe.h"
#include "drivers/terminal/fonts.h"
#include "drivers/v86/v86bios.h"
#include "drivers/drivers.h"
#include "drivers/terminal/tty.h"


struct VgaInfoBlock	*VGAInfo;
struct ModeInfoBlock *CurVGAModeInfo;
int VideoMode = 0x103;
int CurConsoleNo = 0;

int ConXChars;
int ConYChars;
int NRCon;
static int NRBytesPerCon;
int VideoMemBase;
int TotalMemory;

struct ConsoleInfo *con_infos;
struct VideoModeDoer *cur_video_doer;

void ResetConsoleOnScreen()
{
	int con_no = CurConsoleNo;
	cur_video_doer->SetOrigin(VideoMemStart + OriginOfVideoMem);
	cur_video_doer->ReDrawCursor(con_no);
}

static void SysBeep(void)
{
	return;
}

static void GotoXY(int con_no, u32 x, u32 y)
{
	if (CursorX > ConXChars || CursorY >= ConYChars)
		return;
	CursorX = CursorX;
	CursorY = CursorY;
}

static void LF(int con_no)
{
	if (CursorY < Bottom)
		++CursorY;
	else
		cur_video_doer->ScrollUp(con_no);
}

static void RI(int con_no)
{
	if (CursorY > Top)
		--CursorY;
	else
		cur_video_doer->ScrollDown(con_no);
}

static void CR(int con_no)
{
	CursorX = 0;
}

static void Del(int con_no)
{
	if (CursorX)
	{
		CursorX--;
		cur_video_doer->DrawCharInsert(con_no, ' ', 1);
		CursorX--;
	}
}

static void CSI_J(int con_no, int par)
{
	int TmpX = CursorX, TmpY = CursorY;
	switch (par)
	{
		case 0:
			cur_video_doer->DrawCharInsert(con_no, ' ', 
					ConXChars * ConYChars - (CursorY * ConXChars + CursorX));
			break;
		case 1:
			CursorX = 0;
			CursorY = 0;
			cur_video_doer->DrawCharInsert(con_no, ' ', TmpY * ConXChars + CursorX);
			break;
		case 2:
			CursorX = 0;
			CursorY = 0;
			cur_video_doer->DrawCharInsert(con_no, ' ', 
					ConXChars * ConYChars);
			break;
	}
	CursorX = TmpX;
	CursorY = TmpY;		
}

static void CSI_K(int con_no, int par)
{
	int TmpX = CursorX, TmpY = CursorY;
	switch (par)
	{
		case 0:
			if (CursorX >= ConXChars)
				return;
			cur_video_doer->DrawCharInsert(con_no, ' ',
					ConXChars - CursorX);
			break;
		case 1:
			if (CursorX < ConXChars)
			{
				CursorX = 0;
				cur_video_doer->DrawCharInsert(con_no, ' ',
					CursorX);
			}else
			{
				CursorX = 0;
				cur_video_doer->DrawCharInsert(con_no, ' ',
					ConXChars);
			}
			break;
		case 2:
			CursorX = 0;
			cur_video_doer->DrawCharInsert(con_no, ' ', ConXChars);
	}
	CursorX = TmpX;
	CursorY = TmpY;
}

static void CSI_m(int con_no)
{
	int i;
	for (i = 0; i < NPar; ++i)
	{
		switch (Par[i])
		{
			case 0: AttrOfChar = 0x7; break;
			case 1: AttrOfChar = 0xf; break;
			case 4: AttrOfChar = 0xf; break;
			case 7: AttrOfChar = 0x70; break;
			case 27: AttrOfChar = 0x7; break;
		}
	}
}

static void RespondTTY(struct tty_struct *tty)
{
	char *respond = "\033[?1;2c";

	ASMCli();
	while (*respond)
	{
		PUTCH(*respond, tty->read_q);
		++respond;
	}
	ASMSti();
	// CopyToCooked(tty);
}

static void InsertChar(int con_no)
{
	cur_video_doer->DrawChar(con_no, ' ', 1); 
}

static void InsertLine(int con_no)
{
	int oldtop, oldbottom;
	
	oldtop = Top;
	oldbottom = Bottom;
	Top = CursorY;
	Bottom = ConYChars - 1;
	cur_video_doer->ScrollDown(con_no);
	Top = oldtop;
	Bottom = oldbottom;
}

static void DeleteChar(int con_no)
{
	cur_video_doer->DrawChar(con_no, 0x127, 1);
}

static void DeleteLine(int con_no)
{
	int oldtop, oldbottom;
	
	oldtop = Top;
	oldbottom = Bottom;
	Top = CursorY;
	Bottom = ConYChars - 1;
	cur_video_doer->ScrollUp(con_no);
	Top = oldtop;
	Bottom = oldbottom;
}

static void CSI_AT(int con_no, u32 nr)
{
	if (nr > ConXChars)
		nr = ConXChars;
	else if(nr == 0)
		nr = 1;
	while (nr--)
		InsertChar(con_no);
}

static void CSI_L(int con_no, u32 nr)
{
	if (nr > ConYChars)
		nr = ConYChars;
	else if(nr == 0)
		nr = 1;
	while (nr--)
		InsertLine(con_no);
}

static void CSI_P(int con_no, u32 nr)
{
	if (nr > ConXChars)
		nr = ConXChars;
	else if(nr == 0)
		nr = 1;
	while (nr--)
		DeleteChar(con_no);	
}

static void CSI_M(int con_no, u32 nr)
{
	if (nr > ConYChars)
		nr = ConYChars;
	else if(nr == 0)
		nr = 1;
	while (nr--)
		DeleteLine(con_no);	
}

static void SaveCur(int con_no)
{
	SavedX = CursorX;
	SavedY = CursorY;
}

static void RestoreCur(int con_no)
{
	GotoXY(con_no, SavedX, SavedY);
}

void ConInit()
{
	struct V86BIOSIntReg vreg;
	int con_no;
	int NRMemPerConBeyScr;
	int TotalVideoMem;
	
	if (VideoMode != 0x103)
	{
		// VESA Extended mode. Only do woth graphcis mode.
		VGAInfo = DMalloc(sizeof(*VGAInfo));
		if (VGAInfo == NULL)
			panic("DMalloc for VGAInfo failed.");
		CurVGAModeInfo = DMalloc(sizeof(*CurVGAModeInfo));
		if (CurVGAModeInfo == NULL)
			panic("DMalloc for CurVGAModeInfo failed.");
		
		vreg.eax = 0x4f00;
		vreg.es = (addr)GetDAreaPa(VGAInfo) >> 4;
		vreg.edi = (addr)GetDAreaPa(VGAInfo) & 0xf;
		V86BIOSInt(&vreg);
		debug_printk("[VBE] OEM: %s.", ((VGAInfo->OEMStringPtr >> 12) & 0xf) + ((addr)VGAInfo->OEMStringPtr & 0xffff));
		debug_printk("[VBE] Video Memory = %d M.", VGAInfo->TotalMemory * 64 / 1024);
	
		vreg.eax = 0x4f02;
		vreg.ebx = VideoMode;
		V86BIOSInt(&vreg);
		if (vreg.eax != 0x004f)
		{
			vreg.eax = 0x4f02;
			vreg.ebx = 0x108;
			VideoMode = 0x108;
			V86BIOSInt(&vreg);
			if (vreg.eax != 0x004f)
				panic("Cannot set video mode.(mode do not exist ? ? ?)");
		}
		debug_printk("[VBE] Video Mode = %x", VideoMode);
		vreg.eax = 0x4f01;
		vreg.ecx = VideoMode;
		vreg.es = (addr)GetDAreaPa(CurVGAModeInfo) >> 4;
		vreg.edi = (addr)GetDAreaPa(CurVGAModeInfo) & 0xf;
		V86BIOSInt(&vreg);
		debug_printk("[VBE] Mode Attribute = %x", CurVGAModeInfo->ModeAttributes);
		debug_printk("[VBE] X * Y = %d X %d", CurVGAModeInfo->XResolution, CurVGAModeInfo->YResolution);
		debug_printk("[VBE] CharSize(Text Mode) = %d X %d" ,CurVGAModeInfo->XCharSize, CurVGAModeInfo->YCharSize);
		debug_printk("[VBE] WinASegment = %x", CurVGAModeInfo->WinASegment);
		debug_printk("[VBE] WinSize = %d", CurVGAModeInfo->WinSize);
		debug_printk("[VBE] Memory Model = %x", CurVGAModeInfo->MemoryModel);
		debug_printk("[VBE] BankFuncc = %x", CurVGAModeInfo->WinFuncPtr);
	
			// 单屏时（不考虑卷动）显存大小,Beyond Scroll = BeyScr
		 TotalVideoMem = VGAInfo->TotalMemory * 64 * 1024;
		ConXChars = CurVGAModeInfo->XResolution / XCharSize;
		ConYChars = CurVGAModeInfo->YResolution / YCharSize;
		FontsInit();
		if (CurVGAModeInfo->NumberOfPlanes > 1)
		{
			// 16色模式才有位平面这个F******的设定
			NRMemPerConBeyScr = CurVGAModeInfo->XResolution * CurVGAModeInfo->YResolution / 8;
			cur_video_doer = &VBEIColor16VDoer;
		}else
		{
			// 256色，和其他直接色彩模式
			NRMemPerConBeyScr = CurVGAModeInfo->XResolution * CurVGAModeInfo->YResolution * (CurVGAModeInfo->BitsPerPixel) / 8;
			switch (CurVGAModeInfo->BitsPerPixel)
			{
				case 8:
					// 256色
					cur_video_doer = &VBEIColor256VDoer;
					break;
				case 15:
					// 15位高彩色
					cur_video_doer = &VBEDColorB15VDoer;
					break;
				case 16:
					// 16位高彩色
					cur_video_doer = &VBEDColorB16VDoer;
				case 24:
					// 24位真彩色
					cur_video_doer = &VBEDColorB24VDoer;
					break;
			}
		}
	} else
	{
		// 80*25*16
		if (VideoMode > 3)
			VideoMode = 3;	// Console不打算支持标准VGA的图形模式，默认切换为文本模式
		vreg.eax = 0x0 << 16 | VideoMode;
		//V86BIOSInt(&vreg);
		ConYChars = 25;
		ConXChars = 80;
		// VGA Mode,only do with text mode.
		cur_video_doer = &VBETextVDoer;
		NRMemPerConBeyScr = ConXChars * ConYChars * 2;
		VideoMemBase = CORE_SPACE_VIDEO_LOWER;
		TotalVideoMem = CORE_SPACE_VIDEO_UPPER - CORE_SPACE_VIDEO_LOWER + 1;
	}
	
	NRCon = TotalVideoMem / (NRMemPerConBeyScr * 3);
	debug_printk("[CONSOLE] NRCon = %d", NRCon);
	NRBytesPerCon = NRMemPerConBeyScr * 4;
	
	if (NRCon > NR_MAX_CONSOLE)
	{
		NRCon = NR_MAX_CONSOLE;
		NRBytesPerCon = TotalVideoMem / NRCon;
	}
	
	con_infos = KMalloc(sizeof(*con_infos) * NRCon);
	
	for (con_no = 0; con_no < NRCon; ++con_no)
	{
		VideoMemStart = con_no * NRBytesPerCon;
		VideoMemEnd = VideoMemStart + NRBytesPerCon - 1;
		OriginOfVideoMem = 0;
		CursorX = CursorY = 0;
		SavedX = SavedY = 0;
		State = 0;
		AttrOfChar = 0x7;
		Top = 0;
		Bottom = ConYChars - 1;
	}
	CurConsoleNo = 0;
}

void ConWrite(struct tty_struct *tty)
{
	char c;
	int n = CHARS(tty->write_q);
	int con_no = TTY_DEV_NO_TO_TTY_NO(tty->dev) - TTYXX_NO_TTY;
	while (n--)
	{
		GETCH(tty->write_q, c);
		switch (State)
		{
			case 0:
				if (c > 31 && c < 127)
				{
					cur_video_doer->DrawCharInsert(con_no, c, 1);
				} else if(c == 27)
					State = 1;
				else if((c == 10) | (c == 11) || (c == 12))
					LF(con_no);
				else if(c == 13)
					CR(con_no);
				else if(c == ERASE_CHAR(tty))
					Del(con_no);
				else if(c == 8)
				{
					if (CursorX != 0)
						CursorX--;
				} else if(c == 9)
				{
					c = 8 - CursorX % 8;
					CursorX += c;
					if (CursorX > ConXChars)
					{
						CursorX -= ConXChars;
						LF(con_no);
					}
				} else if(c == 7)
					SysBeep();
				break;
			case 1:
				State = 0;
				if (c == '[')
					State = 2;
				else if(c == 'E')
				{
					CursorX = 0;
					CursorY += 1;
				} else if(c == 'M')
					RI(con_no);
				else if(c == 'D')
					LF(con_no);
				else if(c == 'Z')
					RespondTTY(tty);
				else if(c == '7')
					SaveCur(con_no);
				else if(c == '8')
					RestoreCur(con_no);
				break;
			case 2:
				for (NPar = 0; NPar < NPAR; ++NPar)
					Par[NPar] = 0;
				NPar = 0;
				State = 3;
				if (c == '?')
					break;
			case 3:
				if (c == ';' && NPar < NPAR - 1)
				{
					++NPar;
					break;
				} else if(c >= '0' && c <= '9')
				{
					Par[NPar] *= 10;
					Par[NPar] += c - '0';
				} else
					State = 4;
			case 4:
				State = 0;
				switch(c)
				{
					case 'G': case '\'':
						if (Par[0] != 0)
							Par[0]--;
						CursorX = Par[0];
						break;
					case 'A':
						if (Par[0] == 0)
							Par[0]++;
						CursorY -= Par[0];
						break;
					case 'B': case 'e':
						if (Par[0] == 0)
							Par[0]++;
						CursorY += Par[0];
						break;
					case 'C': case 'a':
						if (Par[0] == 0)
							Par[0]++;
						CursorX += Par[0];
						break;
					case 'D':
						if (Par[0] == 0)
							Par[0]++;
						CursorX -= Par[0];
						break;
					case 'E':
						if (Par[0] == 0)
							Par[0]++;
						CursorY += Par[0];
						break;
					case 'F':
						if (Par[0] == 0)
							Par[0]++;
						CursorY -= Par[0];
						break;
					case 'd':
						if (Par[0] != 0)
							Par[0]--;
						CursorY = Par[0];
						break;
					case 'H': case 'f':
						if (Par[0])
							Par[0]--;
						if (Par[1])
							Par[1]--;
						CursorX = Par[1];
						CursorY = Par[0];
						break;
					case 'J':
						CSI_J(con_no, Par[0]);
						break;
					case 'K':
						CSI_K(con_no, Par[0]);
						break;
					case 'L':
						CSI_L(con_no, Par[0]);
						break;
					case 'M':
						CSI_M(con_no, Par[0]);
						break;
					case 'P':
						CSI_P(con_no, Par[0]);
						break;
					case '@':
						CSI_AT(con_no, Par[0]);
						break;
					case 'm':
						CSI_m(con_no);
						break;
					case 'r':
						if (Par[0])
							Par[0]--;
						if (Par[1] != 0)
							Par[1] = ConYChars;
						if (Par[0] < Par[1] && 
							Par[1] <= ConYChars)
						{
							Top = Par[0];
							Bottom = Par[1];
						}
						break;
					case 's':
						SavedX = CursorX;
						SavedY = CursorY;
						break;
					case 'u':
						CursorX = SavedX;
						CursorY = SavedY;
						break;
				}
		}
	}
	if (CurConsoleNo == con_no)
		cur_video_doer->ReDrawCursor(CurConsoleNo);
}

void SysBeepStop(void)
{
	int status;
	status = ASMInByte(0x61);
	ASMOutByte(status & 0xfc, 0x61);
}
