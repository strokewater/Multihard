[SECTION .core_dep]
AsyncBuffers	equ	0xc013f9cc 
global AsyncBuffers 
SyncRequestDevsTop	equ	0xc012651c 
global SyncRequestDevsTop 
Schedule	equ	0xc0117429 
global Schedule 
SyncRequestDevs	equ	0xc0140200 
global SyncRequestDevs 
PutBlk	equ	0xc010cef3 
global PutBlk 
Current	equ	0xc0142428 
global Current 
SyncMInodes	equ	0xc010aae6 
global SyncMInodes 
WriteBufferToDisk	equ	0xc010ca8f 
global WriteBufferToDisk 
HashTable	equ	0xc0140400 
global HashTable 
debug_printk	equ	0xc010fce8 
global debug_printk 
printk	equ	0xc010fc27 
global printk 
DEP_VERSION dd 0x1 
global DEP_VERSION
REQ_PRIO dd 0x0 
global REQ_PRIO
