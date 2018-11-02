#include "gfs/gfslevel.h"
#include "gfs/gfs_utils.h"
#include "gfs/gfs.h"
#include "gfs/minode.h"
#include "string.h"
#include "stdlib.h"
#include "mm/varea.h"
#include "sched.h"


int XCharSize;
int YCharSize;
static struct File *fonts_fp;

int ReadFont(u32 ch, u8 shape[])
{
	fonts_fp->pos = ch * (XCharSize * YCharSize / 8);
	return GFSRead(fonts_fp, shape, XCharSize * YCharSize / 8);
}

void FontsInit()
{
	struct MInode *mi;
	char *path, *size_in_path;
	int i;
	
	path = KMalloc(256);
	strcpy(path, "/multiboot/fonts_");
	size_in_path = path + strlen("/multiboot/fonts");
	
	i = XCharSize;
	while (i != 0)
	{
		*size_in_path++ = i % 10;
		i /= 10;
	}
	*size_in_path++ = 'X';
	i = YCharSize;
	while (i != 0)
	{
		*size_in_path++ = i % 10;
		i /= 10;
	}
	*size_in_path = 0;
	
	mi = GFSReadMInode(Current->root, path);
	fonts_fp = GFSOpenFileByMInode(mi, MAY_READ);
}
