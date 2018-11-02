#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

void GetConfig(const char *cmdline);

int DaemonInterfereVer;

#define MAX_CMDLINE_OPTION			50
#define MAX_CHARS_CMDLINE_OPTION	50
#define MAX_PARA_PER_OPTION			20
#define MAX_CHARS_PER_STR_PARA		20

extern char CMDLineOption[];
extern char CMDLineOptionParaStr[][MAX_CHARS_PER_STR_PARA + 1];
extern int  CMDLineOptionParaInt[];
extern int  NRCMDLineOptionPara ;

#define CMDLINE_OPTION_FINISH		0
#define CMDLINE_OPTION_INT_SEQU		1
#define CMDLINE_OPTION_STR_SEQU		2

struct CMDLineOption
{
	const char *option;
	int (*tackle)(void);
};

#endif
