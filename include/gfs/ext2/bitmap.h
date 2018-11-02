#ifndef FS_EXT2_BITMAP_H
#define FS_EXT2_BITMAP_H

int GetFreeBit(s8 *bitmap, size_t n);
void FreeBit(s8 *bitmap, int pos);


#endif
