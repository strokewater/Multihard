%include "mm/mm.inc"

%define NMAP_PAGES			(CORE_SPACE_MAP_UPPER - CORE_SPACE_MAP_LOWER + 1) / PAGE_SIZE

[SECTION .data]
ALIGN		PAGE_SIZE
global		MapPages
global		KerPDIR
global		OneMPDIR

MapPages:	times		NMAP_PAGES			dd	0
KerPDIR:	times		NR_PDIRS_STATIC * PAGE_SIZE	db	0
OneMPDIR:	times		PAGE_SIZE			dd	0