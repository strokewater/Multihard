#ifndef MPGS_H
#define MPGS_H

#include "mm/mm.h"
#define NMAP_PAGES	(CORE_SPACE_MAP_UPPER - CORE_SPACE_MAP_LOWER + 1) / PAGE_SIZE
extern PAGE MapPages[NMAP_PAGES];
extern PAGE KerPDIR[NR_PDIRS_STATIC][NR_PAGES_PER_DIR];

#endif
