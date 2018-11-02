#include "printk.h"
#include "asm/asm.h"
#include "drivers/terminal/console.h"

static void VBETextDrawChar(int con_no, u32 ch, int n);
static void VBETextDrawCharInsert(int con_no, u32 ch, int n);
static void VBETextReDrawCursor(int con_no);
static void VBETextScrollUp(int con_no);
static void VBETextScrollDown(int con_no);
static void VBETextSetOrigin(u32 origin);

struct VideoModeDoer VBETextVDoer = {.DrawChar = VBETextDrawChar, .DrawCharInsert = VBETextDrawCharInsert, 
									.ReDrawCursor = VBETextReDrawCursor,
									.ScrollUp = VBETextScrollUp, .ScrollDown = VBETextScrollDown,
									.SetOrigin = VBETextSetOrigin};
struct VideoModeDoer VBEIColor16VDoer;
struct VideoModeDoer VBEIColor256VDoer;
struct VideoModeDoer VBEDColorB15VDoer;
struct VideoModeDoer VBEDColorB16VDoer;
struct VideoModeDoer VBEDColorB24VDoer;

/* 提供行内非插入模式的输出字符，处理可显示字符和DEL字符，除DEL外的控制字符不予处理 */
/* 行内非插入模式的意思是，如果在行内不能移动字符就会和插入模式一样吞掉尾部的字符 */
static void VBETextDrawChar(int con_no, u32 ch, int n)
{
	char *origin = (char *)(VideoMemBase + VideoMemStart + OriginOfVideoMem);
	if (ch == 0x127)
	{
		// 删除光标前字符
		// n == 1:
		// [X, ...] <- [X + n, ConXChars)
		// [ConXChars - n - 1, ConChars) <- ''
		if (n > CursorX)
			n = CursorX;
		
		__asm__("cld;" \
				"rep movsb;" : :"S"(origin + (CursorY * ConXChars + CursorX + n) * 2),
							   "D"(origin + (CursorY * ConXChars + CursorX) * 2),
							   "c"(ConXChars - CursorX - n));
		__asm__("movb %%bh, %%ah;"
				"rep stosb;" : :"a"(' '), "b"(AttrOfChar), "c"(n),
								"S"(origin + (CursorY * ConXChars + ConXChars - n - 1) * 2) :);
	} else
	{
		short *cur_ch;
		short *src, *dest;
		short *last;
		int i;
		
		if (n > ConXChars - CursorX - 1)
			n = ConXChars - CursorX - 1;
		
		cur_ch = (short *)(origin + (CursorY * ConXChars + CursorX) * 2);
		dest = cur_ch + n;
		src = cur_ch;
		last = (short *)(origin + (CursorY * ConXChars + ConXChars - 1) * 2);
		while (dest <= last)
		{
			*dest = *src;
			dest++;
			src++;
		}
		
		for (i = 0; i < n; ++i)
		{
			*cur_ch = ch;
			*(cur_ch + 1) = AttrOfChar;
			cur_ch++;
		}
		CursorX += n;
	}
}

/* 提供行间非插入模式的输出字符，只处理普通字符 */
static void VBETextDrawCharInsert(int con_no, u32 ch, int n)
{
	char *origin = (char *)(VideoMemBase + VideoMemStart + OriginOfVideoMem);	
	__asm__("cld;" \
			"rep stosb" : : "a"(AttrOfChar << 8 | ch), "c"(n), "D"(origin + (CursorY * ConXChars + CursorX) * 2));
	CursorX += n;
	if (CursorX >= ConXChars)
	{
		debug_printk("New Line(%d, %d)", CursorX, CursorY);
		CursorX -= ConXChars;
		CursorY++;
	}
}

static void VBETextReDrawCursor(int con_no)
{
	int pos;
	if (con_no != CurConsoleNo)
		return;
	pos = (OriginOfVideoMem + VideoMemStart) / 2 + CursorY * ConXChars + CursorX;
	ASMCli();
	ASMOutByte(0x3d4, 14);
	ASMOutByte(0x3d5, (pos >> 8) & 0xff);
	ASMOutByte(0x3d4, 15);
	ASMOutByte(0x3d5, pos & 0xff);
	ASMSti();
}

static void VBETextScrollUp(int con_no)
{
	char *old_origin = (char *)(VideoMemBase + VideoMemStart + OriginOfVideoMem);
	char *old_scr_end = old_origin + (ConXChars * ConYChars - 1) * 2;
	if (Top == 0 && Bottom == ConYChars - 1)
	{
		if ((addr)(old_scr_end + ConXChars * 2) <= VideoMemBase + VideoMemEnd)
		{
			OriginOfVideoMem += ConXChars * 2;
			__asm__("cld;" \
					"stosw;" : : "a"(AttrOfChar << 8 | ' '), "D"(old_scr_end + 2),
									"c"(ConXChars) :);
		} else
		{
			__asm__("cld;" \
					"rep movsb;" : :"S"(old_origin + ConXChars * 2), "D"(VideoMemBase + VideoMemStart),
									"c"((ConYChars - 1) * ConXChars * 2) :);
			__asm__("cld;"\
					"stosb;" : : "a"(AttrOfChar << 8 | ' '), 
									"D"(VideoMemBase + VideoMemStart + (ConYChars - 1) * ConXChars * 2),
									"c"(ConXChars * 2) :);
			OriginOfVideoMem = VideoMemBase + VideoMemStart;
		}
		if (CurConsoleNo == con_no)
			VBETextSetOrigin(VideoMemStart + OriginOfVideoMem);
		debug_printk("Scroll up finish.\n");
	} else
	{
		__asm__("std;" \
				"rep movsb;" : :"S"(old_origin + (Bottom * ConXChars + ConXChars - 1) * 2),
								"D"(old_origin + ((Bottom - 1) * ConXChars + ConXChars - 1 )* 2),
								"c"((Bottom - Top) * ConXChars * 2) :);
		__asm__("std:" \
				"rep stosb" : : "a"(AttrOfChar << 8 | ' '), 
								"D"(old_origin + (Bottom * ConXChars + ConXChars - 1) * 2),
								"c"(ConXChars * 2) :);
	}
}

static void VBETextScrollDown(int con_no)
{
	char *origin = (char *)(VideoMemBase + VideoMemStart + OriginOfVideoMem);		
	__asm__("std;"\
			"rep movsl;" \
			"addl $2, %%edi;" \
			"movl ConXChars, %%ecx;" \
			"rep stosw;" : : "a"(AttrOfChar << 8 | ' '),
							"c"((Bottom - Top) * ConYChars >> 1),
							"D"(origin + ConXChars * Bottom - 4),
							"S"(origin + ConXChars * (Bottom - 1) - 4) :);
}

void VBETextSetOrigin(u32 origin)
{
	ASMOutByte(0x3d4, 12);
	ASMOutByte(0x3d5, origin >> 9);
	ASMOutByte(0x3d4, 13);
	ASMOutByte(0x3d5, (origin >> 1) & 0xff);
}
