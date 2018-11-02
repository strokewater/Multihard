#include "config.h"
#include "ctype.h"
#include "string.h"
#include "types.h"
#include "stdlib.h"
#include "exception.h"
#include "drivers/ramdisk/ramdisk.h"
#include "gfs/gfs.h"


/* kernel start parameter
 *		--enable-ramdisk
 *		--rd:	(RAMDisk Settings)
 *			--rd-number=N
 ¡Á			--rd[n]-size=N[M/G]	// default K
 *
 *		--root-dev: (about root directory)
 *			--root-dev-dno=root_device_dev_no
 *			--root-dev-fs-type="FileSystemName"
 * 		--video-mode=N
 */

 // in: cmdline
 // out: config variable all ready.
 
 // "--rd", "%d", "-size" "=" "%d" "[%c]"
/* kernel start parameter formal:
 * 1.	--%s
 * 2.	--%s=%d
 * 3.	--%s=%d,%d,%d ...
 * 4.	--%s=%s
 * 5.	--%s=%s,%s,%s ...
 *
 */

int DaemonInterfereVer = 1;

char CMDLineOption[MAX_CHARS_CMDLINE_OPTION + 1];
char CMDLineOptionParaStr[MAX_PARA_PER_OPTION][MAX_CHARS_PER_STR_PARA + 1];
int  CMDLineOptionParaInt[MAX_PARA_PER_OPTION];
int  NRCMDLineOptionPara = 0;
static int  TacklingOptionIdx = -1;

struct CMDLineOption ALLCMDLineOptions[MAX_CMDLINE_OPTION];

const char *ScanIntInCMDLine(int *ret, const char *cmdline)
{
	int is_neg = 0;
	*ret = 0;

	if (*cmdline == '-')
	{
		++cmdline;
		is_neg = 1;
	}

	if (*cmdline == '0')
	{
		++cmdline;
		if (*cmdline == 'x' || *cmdline == 'X')
		{
			// hex
			++cmdline;
			while (isxdigit(*cmdline))
			{
				if (isdigit(*cmdline))
					*ret = *ret * 16 + *cmdline - '0';
				else if(islower(*cmdline))
					*ret = *ret * 16 + *cmdline - 'a';
				else if(isupper(*cmdline))
					*ret = *ret * 16 + *cmdline - 'A';
				else
					panic("Cannot recognize hex char.");

				++cmdline;
			}
		} else
		{
			// oct
			++cmdline;
			while (*cmdline >= '0' && *cmdline < '8')
			{
				*ret = *ret * 8 + *cmdline - '0';
				++cmdline;
			}
				
		}
	} else
	{
		// dec
		while (isdigit(*cmdline))
		{
			*ret = *ret * 10 + *cmdline - '0';
			++cmdline;
		}		
	}

	*ret = is_neg ? -*ret : *ret;
	return cmdline;
}

const char *ScanStrInCMDLine(char *ret, const char *cmdline)
{
	// 转义字符\,后面一个字符当普通字符处理
	if (isspace(*cmdline) || *cmdline == ',' || *cmdline == '\0')
	{
		*ret = '\0';
		return cmdline;
	}

	do
	{
		if (*cmdline == '\\')
			++cmdline;

		*ret++ = *cmdline++;
	} while (!isspace(*cmdline) && *cmdline != ',' && *cmdline);
	*ret = '\0';
	return  cmdline;
}

int TackleCMDLineOption()
{
	int i;
	int tackle_ret;

	if (TacklingOptionIdx == -1)
	{
		for (i = 0; ALLCMDLineOptions[i].option; ++i)
		{
			if (strcmp(ALLCMDLineOptions[i].option, CMDLineOption) == 0)
			{
				TacklingOptionIdx = i;
				break;
			}
		}
		if (ALLCMDLineOptions[i].option == NULL)
			panic("Unrecognized option.");
	}
	tackle_ret = ALLCMDLineOptions[TacklingOptionIdx].tackle();
	if (tackle_ret == CMDLINE_OPTION_FINISH)
		TacklingOptionIdx = -1;
	return tackle_ret;
}

void ScanCMDLine(const char *cmdline)
{
	NRCMDLineOptionPara = 0;

	while (*cmdline)
	{
		while (isspace(*cmdline))
			++cmdline;
		if (*cmdline != '-' || *(cmdline + 1) != '-')
			panic("Illegal option: option must start with '--'");
		cmdline += 2;
		
		int option_idx = -1;
		CMDLineOption[++option_idx] = '-';
		CMDLineOption[++option_idx] = '-';
		while (*cmdline && *cmdline != '=' && !isspace(*cmdline))
		{
			CMDLineOption[++option_idx] = *cmdline;
			++cmdline;
		}
		CMDLineOption[++option_idx] = '\0';

		int result = TackleCMDLineOption();
		if (result == CMDLINE_OPTION_FINISH)
			continue;
		else if(result == CMDLINE_OPTION_INT_SEQU)
		{
			++cmdline;		// skip '='
			while (*cmdline && !isspace(*cmdline))
			{
				cmdline = ScanIntInCMDLine(CMDLineOptionParaInt + NRCMDLineOptionPara, cmdline);
				NRCMDLineOptionPara++;
				if (*cmdline != ',')
				{
					if (*cmdline == '\0' || isspace(*cmdline))
						break;
					else
						panic("unreconized char while scanning int");
				}
				++cmdline;
			}
			TackleCMDLineOption();
			NRCMDLineOptionPara = 0;
		} else if(result == CMDLINE_OPTION_STR_SEQU)
		{
			++cmdline;		// skip '='
			while (*cmdline && !isspace(*cmdline))
			{
				cmdline = ScanStrInCMDLine(CMDLineOptionParaStr[NRCMDLineOptionPara], cmdline);
				++NRCMDLineOptionPara;
				if (*cmdline != ',')
				{
					if (*cmdline == '\0' || isspace(*cmdline))
						break;
					else
						panic("unreconized char while scanning string");
				}
				++cmdline;
			}
			TackleCMDLineOption();
			NRCMDLineOptionPara = 0;
		}else
			panic("unrecoginzed recognize respond.");
	}
}

void GetConfig(const char *cmdline)
{
	struct CMDLineOption *AllOptionsGrp[] = {
		RAMDiskCMDLineOptions,
		FSCMDLineOptions,
	};

	int i = -1;
	for (int j = 0; j < sizeof(AllOptionsGrp) / sizeof(AllOptionsGrp[0]); ++j)
	{
		for (int k = 0; AllOptionsGrp[j][k].option; ++k)
			ALLCMDLineOptions[++i] = AllOptionsGrp[j][k];
	}
	ALLCMDLineOptions[++i].option = NULL;

	ScanCMDLine(cmdline);
}
