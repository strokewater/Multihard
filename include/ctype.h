#ifndef CTYPE_H
#define CTYPE_H

#define isspace(c)	((c) == ' ' || (c) == '\t')
#define isxdigit(c)	(isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
#define isdigit(c)	(c >= '0' && c <= '9')
#define islower(c)	(c >= 'a' && c <= 'z')
#define isupper(c)	(c >= 'A' && c <= 'Z')

#define tolower(c)	(isupper((c)) ? c - 'A' + 'a' : c)
#define toupper(c)	(islower((c)) ? c - 'a' + 'A' : c)

#endif
