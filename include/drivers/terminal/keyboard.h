#ifndef DRIVERS_KEYBOARD_H
#define DRIVERS_KEYBOARD_H

#define NR_SCAN_CODE	0x80
#define IS_KP_NUM_DOT_KEY(cd)	(((cd & ~0x80) >= 0x47 && (cd & ~0x80) <= 0x49) \
								|| ((cd & ~0x80) >= 0x4b && (cd & ~0x80) <= 0x4d) \
								|| ((cd & ~0x80) >= 0x4f && (cd & ~0x80) <= 0x53))
#define KB_WAIT_TYPE_IBUF_EMPTY	0
#define KB_WAIT_TYPE_OBUF_FULL	1

#define KB_STATUS_REG_IBUF_FULL	2
#define KB_STATUS_REG_OBUF_FULL	1

#define KB_CMD_SET_LED			0xed
#define KB_ACK					0xfa

void KBInit();

#if defined(KEYBOARD_C)
enum
{
	EXT_KEY = 0x0100,
	EXT_KEY_NUL = 0x0100,
	EXT_KEY_SOH,
	EXT_KEY_STX,
	EXT_KEY_ETX,
	EXT_KEY_EOT,
	EXT_KEY_ENQ,
	EXT_KEY_ACK,
	EXT_KEY_BEL,
	EXT_KEY_BS,
	EXT_KEY_HT,
	EXT_KEY_LF,
	EXT_KEY_VT,
	EXT_KEY_FF,
	EXT_KEY_CR,
	EXT_KEY_SO,
	EXT_KEY_SI,
	EXT_KEY_DLE,
	EXT_KEY_XON,
	EXT_KEY_DC2,
	EXT_KEY_XOFF,
	EXT_KEY_DC4,
	EXT_KEY_NAK,
	EXT_KEY_SYN,
	EXT_KEY_ETB,
	EXT_KEY_CAN,
	EXT_KEY_EM,
	EXT_KEY_SUB,
	EXT_KEY_ESC,
	EXT_KEY_FS,
	EXT_KEY_GS,
	EXT_KEY_RS,
	EXT_KEY_US,
	EXT_KEY_DEL,
	EXT_KEY_BACKSPACE,
	EXT_KEY_TAB, 
	EXT_KEY_ENTER,
	EXT_KEY_CTRL,
	EXT_KEY_SHIFT,
	EXT_KEY_ALT,
	EXT_KEY_CAPS_LOCK,
	EXT_KEY_F1,
	EXT_KEY_F2,
	EXT_KEY_F3,
	EXT_KEY_F4,
	EXT_KEY_F5,
	EXT_KEY_F6,
	EXT_KEY_F7,
	EXT_KEY_F8,
	EXT_KEY_F9,
	EXT_KEY_F10,
	EXT_KEY_F11,
	EXT_KEY_F12,
	EXT_KEY_NUM_LOCK,
	EXT_KEY_SCROLL_LOCK,
	EXT_KEY_HOME,
	EXT_KEY_UP,
	EXT_KEY_PAGE_UP,
	EXT_KEY_LEFT,
	EXT_KEY_RIGHT,
	EXT_KEY_END,
	EXT_KEY_DOWN,
	EXT_KEY_PAGE_DOWN,
	EXT_KEY_INSERT,
	EXT_KEY_DELETE,
	EXT_KEY_GUI,
};

const char *ExtKeyValue[] = 
{
	"\x0",	// EXT_KEY_NUL
	"\x01",	// EXT_KEY_SOH
	"\x02",	// EXT_KEY_STX
	"\x03",	// EXT_KEY_ETX
	"\x04",	// EXT_KEY_EOT
	"\x05", // EXT_KEY_ENQ
	"\x06",	// EXT_KEY_ACK
	"\x07",	// EXT_KEY_BEL
	"\x08",	// EXT_KEY_BS
	"\x09",	// EXT_KEY_HT
	"\x0a",	// EXT_KEY_LF
	"\x0b",	// EXT_KEY_VT
	"\x0c",	// EXT_KEY_FF
	"\x0d",	// EXT_KEY_CR
	"\x0e",	// EXT_KEY_S0
	"\x0f",	// EXT_KEY_SI
	"\x10",	// EXT_KEY_DLE
	"\x11", // EXT_KEY_XON
	"\x12", // EXT_KEY_DC2
	"\x13",	// EXT_KEY_XOFF
	"\x14",	// EXT_KEY_DC4
	"\x15",	// EXT_KEY_NAK
	"\x16",	// EXT_KEY_SYN
	"\x17",	// EXT_KEY_ETB
	"\x18",	// EXT_KEY_CAN
	"\x19", // EXT_KEY_EM
	"\x1a", // EXT_KEY_SUB
	"\x1b",	// EXT_KEY_ESC
	"\x1c",	// EXT_KEY_FS
	"\x1d",	// EXT_KEY_GS
	"\x1e",	// EXT_KEY_RS
	"\x1f",	// EXT_KEY_US
	"\x7f",	// EXT_KEY_DEL
	"\x7f", // EXT_KEY_BACKSPACE
	"\t",	// EXT_KEY_TAB
	"\r",	// EXT_KEY_ENTER
	"__ctrl", 	// EXT_KEY_CTRL
	"__shift", 	// EXT_KEY_SHIFT
	"__alt",	// EXT_KEY_ALT
	"__caps",	// EXT_KEY_CAPS_LOCK
	"\x1b[[A",	// EXT_KEY_F1
	"\x1b[[B",	// EXT_KEY_F2
	"\x1b[[C",	// EXT_KEY_F3
	"\x1b[[D",	// EXT_KEY_F4
	"\x1b[[E",	// EXT_KEY_F5
	"\x1b[[F",	// EXT_KEY_F6
	"\x1b[[G",	// EXT_KEY_F7
	"\x1b[[H",	// EXT_KEY_F8
	"\x1b[[I",	// EXT_KEY_F9
	"\x1b[[J",	// EXT_KEY_F10
	"\x1b[[K",	// EXT_KEY_F11
	"\x1b[[L",	// EXT_KEY_F12
	"__num",	// EXT_KEY_NUM_LOCK
	"__scroll",	// EXT_KEY_SCROLL_LOCK
	"\x1b[H",	// EXT_KEY_HOME
	"\x1b[A",	// EXT_KEY_UP
	"\x1b[5~",	// EXT_KEY_PAGE_UP
	"\x1b[D",	// EXT_KEY_LEFT
	"\x1b[C",	// EXT_KEY_RIGHT
	"\x1b[Y",	// EXT_KEY_END
	"\x1b[B",	// EXT_KEY_DOWN
	"\x1b[6~",	// EXT_KEY_PAGE_DOWN
	"\x1b[2~",	// EXT_KEY_INSERT
	"\x1b[3~",	// EXT_KEY_DELETE
	"__gui",	// EXT_KEY_GUI
};

// 假设全部灯都是灭的.
const u32 KeyMap[NR_SCAN_CODE * 3] = 
{
/* scan-code			!Shift				Shift			E0 XX	*/
/* ==================================================================== */
/* 0x00 - none		*/	0,					0,					0,
/* 0x01 - ESC		*/	EXT_KEY_ESC,		EXT_KEY_ESC,		0,
/* 0x02 - '1'		*/	'1',				'!',				0,
/* 0x03 - '2'		*/	'2',				'@',				0,
/* 0x04 - '3'		*/	'3',				'#',				0,
/* 0x05 - '4'		*/	'4',				'$',				0,
/* 0x06 - '5'		*/	'5',				'%',				0,
/* 0x07 - '6'		*/	'6',				'^',				0,
/* 0x08 - '7'		*/	'7',				'&',				0,
/* 0x09 - '8'		*/	'8',				'*',				0,
/* 0x0A - '9'		*/	'9',				'(',				0,
/* 0x0B - '0'		*/	'0',				')',				0,
/* 0x0C - '-'		*/	'-',				'_',				0,
/* 0x0D - '='		*/	'=',				'+',				0,
/* 0x0E - BS		*/	EXT_KEY_BS,			EXT_KEY_BS,			0,
/* 0x0F - TAB		*/	EXT_KEY_TAB,		EXT_KEY_TAB,		0,
/* 0x10 - 'q'		*/	'q',				'Q',				0,
/* 0x11 - 'w'		*/	'w',				'W',				0,
/* 0x12 - 'e'		*/	'e',				'E',				0,
/* 0x13 - 'r'		*/	'r',				'R',				0,
/* 0x14 - 't'		*/	't',				'T',				0,
/* 0x15 - 'y'		*/	'y',				'Y',				0,
/* 0x16 - 'u'		*/	'u',				'U',				0,
/* 0x17 - 'i'		*/	'i',				'I',				0,
/* 0x18 - 'o'		*/	'o',				'O',				0,
/* 0x19 - 'p'		*/	'p',				'P',				0,
/* 0x1A - '['		*/	'[',				'{',				0,
/* 0x1B - ']'		*/	']',				'}',				0,
/* 0x1C - CR/LF		*/	EXT_KEY_ENTER,		EXT_KEY_ENTER,		EXT_KEY_ENTER,
/* 0x1D - l. Ctrl	*/	EXT_KEY_CTRL,		EXT_KEY_CTRL,		EXT_KEY_CTRL,
/* 0x1E - 'a'		*/	'a',				'A',				0,
/* 0x1F - 's'		*/	's',				'S',				0,
/* 0x20 - 'd'		*/	'd',				'D',				0,
/* 0x21 - 'f'		*/	'f',				'F',				0,
/* 0x22 - 'g'		*/	'g',				'G',				0,
/* 0x23 - 'h'		*/	'h',				'H',				0,
/* 0x24 - 'j'		*/	'j',				'J',				0,
/* 0x25 - 'k'		*/	'k',				'K',				0,
/* 0x26 - 'l'		*/	'l',				'L',				0,
/* 0x27 - ';'		*/	';',				':',				0,
/* 0x28 - '\''		*/	'\'',				'"',				0,
/* 0x29 - '`'		*/	'`',				'~',				0,
/* 0x2A - l. SHIFT	*/	EXT_KEY_SHIFT,		EXT_KEY_SHIFT,		0,
/* 0x2B - '\'		*/	'\\',				'|',				0,
/* 0x2C - 'z'		*/	'z',				'Z',				0,
/* 0x2D - 'x'		*/	'x',				'X',				0,
/* 0x2E - 'c'		*/	'c',				'C',				0,
/* 0x2F - 'v'		*/	'v',				'V',				0,
/* 0x30 - 'b'		*/	'b',				'B',				0,
/* 0x31 - 'n'		*/	'n',				'N',				0,
/* 0x32 - 'm'		*/	'm',				'M',				0,
/* 0x33 - ','		*/	',',				'<',				0,
/* 0x34 - '.'		*/	'.',				'>',				0,
/* 0x35 - '/'		*/	'/',				'?',				'/',//PAD_SLASH,
/* 0x36 - r. SHIFT	*/	EXT_KEY_SHIFT,		EXT_KEY_SHIFT,		0,
/* 0x37 - '*'		*/	'*',				'*',    			0,
/* 0x38 - ALT		*/	EXT_KEY_ALT,		EXT_KEY_ALT,  		EXT_KEY_ALT,
/* 0x39 - ' '		*/	' ',				' ',				0,
/* 0x3A - CapsLock	*/	EXT_KEY_CAPS_LOCK,	EXT_KEY_CAPS_LOCK,	0,
/* 0x3B - F1		*/	EXT_KEY_F1,			EXT_KEY_F1,			0,
/* 0x3C - F2		*/	EXT_KEY_F2,			EXT_KEY_F2,			0,
/* 0x3D - F3		*/	EXT_KEY_F3,			EXT_KEY_F3,			0,
/* 0x3E - F4		*/	EXT_KEY_F4,			EXT_KEY_F4,			0,
/* 0x3F - F5		*/	EXT_KEY_F5,			EXT_KEY_F5,			0,
/* 0x40 - F6		*/	EXT_KEY_F6,			EXT_KEY_F6,			0,
/* 0x41 - F7		*/	EXT_KEY_F7,			EXT_KEY_F7,			0,
/* 0x42 - F8		*/	EXT_KEY_F8,			EXT_KEY_F8,			0,
/* 0x43 - F9		*/	EXT_KEY_F9,			EXT_KEY_F9,			0,
/* 0x44 - F10		*/	EXT_KEY_F10,		EXT_KEY_F10,		0,
/* 0x45 - NumLock	*/	EXT_KEY_NUM_LOCK,	EXT_KEY_NUM_LOCK,	0,
/* 0x46 - ScrLock	*/	EXT_KEY_SCROLL_LOCK,EXT_KEY_SCROLL_LOCK,0,
/* 0x47 - Home		*/	EXT_KEY_HOME,		'7',				EXT_KEY_HOME,
/* 0x48 - CurUp		*/	EXT_KEY_UP,			'8',				EXT_KEY_UP,
/* 0x49 - PgUp		*/	EXT_KEY_PAGE_UP,	'9',				EXT_KEY_PAGE_UP,
/* 0x4A - '-'		*/	'-',				'-',				0,
/* 0x4B - Left		*/	EXT_KEY_LEFT,		'4',				EXT_KEY_LEFT,
/* 0x4C - MID		*/	0,					'5',				0,
/* 0x4D - Right		*/	EXT_KEY_RIGHT,		'6',				EXT_KEY_RIGHT,
/* 0x4E - '+'		*/	'+',				'+',				0,
/* 0x4F - End		*/	EXT_KEY_END,		'1',				EXT_KEY_END,
/* 0x50 - Down		*/	EXT_KEY_DOWN,		'2',				EXT_KEY_DOWN,
/* 0x51 - PgDown	*/	EXT_KEY_PAGE_DOWN,	'3',				EXT_KEY_PAGE_DOWN,
/* 0x52 - Insert	*/	EXT_KEY_INSERT,		'0',				EXT_KEY_INSERT,
/* 0x53 - Delete	*/	'.',				'.',				EXT_KEY_DELETE,
/* 0x54 - Enter		*/	0,					0,					0,
/* 0x55 - ???		*/	0,					0,					0,
/* 0x56 - ???		*/	0,					0,					0,
/* 0x57 - F11		*/	EXT_KEY_F11,		EXT_KEY_F11,		0,	
/* 0x58 - F12		*/	EXT_KEY_F12,		EXT_KEY_F12,		0,	
/* 0x59 - ???		*/	0,					0,					0,	
/* 0x5A - ???		*/	0,					0,					0,	
/* 0x5B - ???		*/	0,					0,					EXT_KEY_GUI,	
/* 0x5C - ???		*/	0,					0,					EXT_KEY_GUI,	
/* 0x5D - ???		*/	0,					0,					0,//APPS	
/* 0x5E - ???		*/	0,					0,					0,	
/* 0x5F - ???		*/	0,					0,					0,
/* 0x60 - ???		*/	0,					0,					0,
/* 0x61 - ???		*/	0,					0,					0,	
/* 0x62 - ???		*/	0,					0,					0,	
/* 0x63 - ???		*/	0,					0,					0,	
/* 0x64 - ???		*/	0,					0,					0,	
/* 0x65 - ???		*/	0,					0,					0,	
/* 0x66 - ???		*/	0,					0,					0,	
/* 0x67 - ???		*/	0,					0,					0,	
/* 0x68 - ???		*/	0,					0,					0,	
/* 0x69 - ???		*/	0,					0,					0,	
/* 0x6A - ???		*/	0,					0,					0,	
/* 0x6B - ???		*/	0,					0,					0,	
/* 0x6C - ???		*/	0,					0,					0,	
/* 0x6D - ???		*/	0,					0,					0,	
/* 0x6E - ???		*/	0,					0,					0,	
/* 0x6F - ???		*/	0,					0,					0,	
/* 0x70 - ???		*/	0,					0,					0,	
/* 0x71 - ???		*/	0,					0,					0,	
/* 0x72 - ???		*/	0,					0,					0,	
/* 0x73 - ???		*/	0,					0,					0,	
/* 0x74 - ???		*/	0,					0,					0,	
/* 0x75 - ???		*/	0,					0,					0,	
/* 0x76 - ???		*/	0,					0,					0,	
/* 0x77 - ???		*/	0,					0,					0,	
/* 0x78 - ???		*/	0,					0,					0,	
/* 0x78 - ???		*/	0,					0,					0,	
/* 0x7A - ???		*/	0,					0,					0,	
/* 0x7B - ???		*/	0,					0,					0,	
/* 0x7C - ???		*/	0,					0,					0,	
/* 0x7D - ???		*/	0,					0,					0,	
/* 0x7E - ???		*/	0,					0,					0,
/* 0x7F - ???		*/	0,					0,					0
};
#endif

#endif
