#include "types.h"


int GetFreeBit(s8 *bitmap, size_t n)
{
	int i,j;
	for (i = 0; i < n; ++i)
	{
		if (bitmap[i] == 0xff)
			continue;
		for (j = 0; j < sizeof(s8) * 8 && bitmap[i] & (1 << j); ++j)
			;
		if (j < sizeof(s8) * 8)
		{
			bitmap[i] |= (1 << j);
			return i * sizeof(s8) * 8 + j;
		}
	}
	
	return -1;
}

void FreeBit(s8 *bitmap, int pos)
{
	int i, j;
	i = pos / 8;
	j = pos % 8;
	bitmap[i] &= ~(1 << j);
}
