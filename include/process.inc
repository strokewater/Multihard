%ifndef PROCESS_INC
%define PROCESS_INC

%include "mm/mm.inc"

PROCESS_STK0_BASE		equ	(CORE_SPACE_LOWER + 0x6000000)
PROCESS_STK0_TOP		equ	(CORE_SPACE_LOWER + 0x6001fe0)

%endif