#include "stdarg.h"
#include "ctype.h"
#include "printk.h"
#include "asm/asm.h"

static int pos = 0;

void display_char(char c)
{
	int n = 4;
	if (c == '\n')
		pos = ((pos / 80) + 1) * 80; 
	else if (c == '\t')
	{
		while (n--)
			*((unsigned short *)CORE_SPACE_VIDEO_LOWER + pos++) = 0xF << 8 | ' ';
	} else
		*((unsigned short *)CORE_SPACE_VIDEO_LOWER + pos++) = 0xF << 8 | c;
}

void display_str(char *str)
{
	while (*str)
		display_char(*str++);
}

char *sprintk_hex(char pool[], int num)
{
	int n = 8;
	int hex_num;
	while (n--)
	{
		hex_num = (num & 0xf0000000) >> 28;
		*pool++ = "0123456789ABCDEF"[hex_num];
		num <<= 4;
	}
	*pool = 0;
	return pool;
}

char *sprintk_dec(char pool[], int num)
{
	static int buf[10];
	static int top;
	int t, i;
	int sign = num < 0;

	top = -1;
	
	if (sign)
		t = -num;
	else
		t = num;
	while (t != 0)
	{
		buf[++top] = t % 10;
		t /= 10;
	}

	// special: num == 0
	if (top == -1)
		buf[++top] = 0;
	
	if (sign)
		*pool++ = '-';
	
	for (i = top; i >= 0; --i)
		*pool++ = buf[i] + '0';
	*pool = 0;
	return pool;
}


char *sprintk_uint(char pool[], unsigned int num)
{
	static int buf[10];
	static int top;
	unsigned t;
	int i;

	top = -1;
	t = num;
	while (t != 0)
	{
		buf[++top] = t % 10;
		t /= 10;
	}

	// special: num == 0
	if (top == -1)
		buf[++top] = 0;
	
	for (i = top; i >= 0; --i)
		*pool++ = buf[i] + '0';
	*pool = 0;
	return pool;
}

int sprintk(char *str, const char *format, ...)
{
	va_list ap;
	char *pstr;
	int num;
	unsigned unum;
	char *tmp_str = str;
	va_start(ap, format);
	
	while (*format)
	{
		if (*format == '%')
		{
			++format;
			if (*format == '%')
				*str++ = '%';
			else if(*format == 'd')
			{
				num = va_arg(ap, int);
				str = sprintk_dec(str, num);
			} else if(*format == 'u')
			{
				unum = va_arg(ap, unsigned int);
				str = sprintk_uint(str, num);
			}else if(*format == 'x')
			{
				num = va_arg(ap, int);
				str = sprintk_hex(str, num);
			} else if(*format == 's')
			{
				pstr = va_arg(ap, char *);
				while (*pstr)
					*str++ = *pstr++;
			} else if(*format == 'c')
			{
				num = va_arg(ap, int);
				*str++ = num;
			}
			++format;
		} else
			*str++ = *format++;
	}
	*str = '\0';
	
	return str - tmp_str;
}

void printk(const char *format, ...)
{
	int nbytes = 0;
	char buffer[256];
	const char *cp = format;
	
	while (*cp)
	{
		if (*cp == '%')
		{
			++cp;
			if (*cp == '%')
				++cp;
			else
			{
				++cp;
				nbytes += 4;
			}
		}else
			++cp;
	}
	
	int tmp_es;
	__asm__("subl %%eax, %%esp;" : : "a"(nbytes) :);
	__asm__("movw %%es, %%ax;" : "=a"(tmp_es) ::);
	__asm__("movw %%ss, %%ax;"
			"movw %%ax, %%es;" :::);
	__asm__("movl %%esp, %%edi; "
			"rep movsb;" : :  "c"(nbytes), "S"(&format + 1) :);
	sprintk(buffer, format);
	__asm__("addl  %%eax, %%esp;" : : "a"(nbytes) :);
	__asm__("movw  %%ax, %%es;" : : "a"(tmp_es) :);
	display_str(buffer);
}

void e9_output(char *str)
{
	while (*str)
	{
		__asm__("outb %%al, %%dx": :"a"(*str), "d"(0xe9):);
		++str;
	}
}

void debug_printk(const char *format, ...)
{
#ifdef NO_BOCHS_DEBUG_PORT
	return;
#endif
	static int debug_msg_no = 0;
	int nbytes = 0;
	char buffer[250];
	const char *cp = format;
	
	while (*cp)
	{
		if (*cp == '%')
		{
			++cp;
			if (*cp == '%')
			{
				++cp;
				continue;
			}
			else
			{
				++cp;
				nbytes += 4;
			}
		}
		++cp;
	}
	
	int tmp_es;
	__asm__("subl %%eax, %%esp;" : : "a"(nbytes) :);
	__asm__("movw %%es, %%ax;" : "=a"(tmp_es) ::);
	__asm__("movl %%esp, %%edi; "
			"rep movsb;" : :  "c"(nbytes), "S"(&format + 1) :);
	sprintk(buffer, format);
	__asm__("addl  %%eax, %%esp;" : : "a"(nbytes) :);
	__asm__("movw  %%ax, %%es;" : : "a"(tmp_es) :);
	
	char int_buf[10];
	sprintk_hex(int_buf, debug_msg_no);
	e9_output("[");
	e9_output(int_buf);
	e9_output("]: ");
	e9_output(buffer);
	e9_output("\n");
	++debug_msg_no;
}
