#include "string.h"

void *memset(void *vsrc, int c, size_t n)
{
	char *src = vsrc;
	
	while (n--)
		*src++ = c;
	
	return vsrc;
}

void *memcpy(void *vdest, const void *vsrc, size_t n)
{
	char *dest = vdest;
	const char *src = vsrc;
	
	while (n--)
		*dest++ = *src++;
	
	return vdest;
}

void *memmove(void *vdest, const void *vsrc, size_t n)
{
	char const *src = vsrc;
	char *dest = vdest;
	char *ret =vdest;
	if (dest <= src || dest >= src + n)
	{
		while (n--)
			*dest++ = *src++;
	} else
	{
		dest += n - 1;
		src += n - 1;
		while (n--)
			*dest-- = *src--;
	}
	return ret;
}

int memcmp(const void *p1, const void *p2, size_t n)
{
	size_t i;
	for (i = 0; i < n && *(char *)p1 == *(char *)p2; ++i, ++p1, ++p2)
		;
	
	if (i == n)
		return 0;
	else
		return *(char *)p1 - *(char *)p2;
}

size_t strlen(const char *s)
{
		int i = 0;
		while (s[i])
			++i;
		return i;
}


char *strcpy(char *dest, const char *src)
{
	char * dest_copy = dest; 
	
	while ((*dest++ = *src++)!='\0')
		; 
	
	return dest_copy;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	char *dest_copy = dest;
	while (*src && n-- > 0)
		*dest++ = *src++;
	return dest_copy;
}

int strcmp(const char *str1,const char *str2)  
{  
	while(*str1 && *str2 && *str1==*str2)
	{
		++str1;
		++str2;
	}
	return *str1 - *str2;
} 

int strncmp(const char *str1,const char *str2, size_t n)
{
	int i;
	for (i = 0; i < n; ++i)
	{
		if (str1[i] != str2[i])
			return str1[i] - str2[i];
	}
	return 0;	
}

void itoa(int i, char a[])
{
	if (i == 0)
	{
		a[0] = '0';
		a[1] = 0;
		return;
	}
	while (i != 0)
	{
		*a++ = i + '0';
		i /= 10;
	}
	*a = 0;
}
