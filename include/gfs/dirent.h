#ifndef DIRENT_H
#define DIRENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "limits.h"

struct dirent {
	ino_t  d_ino;              /* inode number */
	off_t d_off;              /* offset to this old_linux_dirent */
	unsigned short d_namlen;  /* length of this d_name */
	char  d_name[NAME_MAX+1]; /* filename (null-terminated) */
};


#ifdef __cplusplus
}
#endif

#endif
