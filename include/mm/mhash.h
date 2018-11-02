#ifndef MHASH_H
#define MHASH_H

#include "mm/mm.h"
#include "types.h"

void *MHashPage(void *pa);
void UnHashPage(void *va);

#endif
