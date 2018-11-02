#ifndef LIB_STRING_H
#define LIB_STRING_H

#include "types.h"

void *memset(void *vsrc, int c, size_t n);
void *memcpy(void *vdest, const void *vsrc, size_t n);
void *memmove(void *vdest, const void *vsrc, size_t n);
int strcmp(const char *str1,const char *str2);
int strncmp(const char *str1,const char *str2, size_t n);
int memcmp(const void *p1, const void *p2, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
size_t strlen(const char *s);
void itoa(int i, char a[]);

#endif
