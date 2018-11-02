#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"
#include "drivers/terminal/tty.h"

#define NR_MAX_CONSOLE 7

#pragma pack(1)

struct VgaInfoBlock
{
	u8 VESASignature[4];
	u16	VESAVersion;
	u32 OEMStringPtr;
	u8	Capabilities[4];
	u32 VideoModePtr;
	u16	TotalMemory;
	u8	Reserved[236];
};

struct ModeInfoBlock
{
	u16	ModeAttributes;
	u8	WinAAttributes;
	u8	WinBAttributes;
	u16	WinGranularity;
	u16	WinSize;
	u16	WinASegment;
	u16	WinBSegment;
	u32 WinFuncPtr;
	u16	BytesPerScanLine;
	u16	XResolution;
	u16	YResolution;
	u8	XCharSize;
	u8	YCharSize;
	u8	NumberOfPlanes;
	u8	BitsPerPixel;
	u8	NumberOfBanks;
	u8	MemoryModel;
	u8	BankSize;
	u8	NumberOfImagePages;
	u8	Reserved;
	u8	RedMaskSize;
	u8	RedFieldPosition;
	u8	GreenMaskSize;
	u8	GreenFieldPosition;
	u8	BlueMaskSize;
	u8	BlueFieldPosition;
	u8	RsvdMaskSize;
	u8	RsvdFieldPosition;
	u8	DirectColorModeInfo;
	u8	Reserved1[216];
};

#pragma pack()

#define NPAR	16

struct ConsoleInfo
{
	int VideoMemStart;
	int VideoMemEnd;
	int OriginOfVideoMem;
	int Top;
	int Bottom;	// Scroll range:[Top.start, Bottom.end]
	int CursorX;
	int CursorY;
	int SavedX;
	int SavedY;
	int AttrOfChar;
	int State;
	int Par[NPAR];
	int NPar;
};

#define VideoMemStart		con_infos[con_no].VideoMemStart
#define VideoMemEnd			con_infos[con_no].VideoMemEnd
#define OriginOfVideoMem	con_infos[con_no].OriginOfVideoMem
#define Top					con_infos[con_no].Top
#define Bottom				con_infos[con_no].Bottom
#define CursorX				con_infos[con_no].CursorX
#define CursorY				con_infos[con_no].CursorY
#define SavedX				con_infos[con_no].SavedX
#define SavedY				con_infos[con_no].SavedY
#define AttrOfChar			con_infos[con_no].AttrOfChar
#define State				con_infos[con_no].State
#define Par					con_infos[con_no].Par
#define NPar				con_infos[con_no].NPar

struct VideoModeDoer
{
	void (*DrawChar)(int con_no, u32 ch, int n);	// powerful!!!, do with insert_char() and delete_char()
	void (*DrawCharInsert)(int con_no, u32 ch, int n);
	void (*ReDrawCursor)(int con_no);
	void (*ScrollUp)(int con_no);
	void (*ScrollDown)(int con_no);
	void (*SetOrigin)(u32 origin);
};

extern int CurConsoleNo;
extern int NRCon;
extern int VideoMemBase;
extern struct ConsoleInfo *con_infos;
extern int ConXChars;
extern int ConYChars;
void ConWrite(struct tty_struct *tty);
void ConInit();
void ResetConsoleOnScreen();
#endif
