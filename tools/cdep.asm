[SECTION .core_dep]
AsyncBuffers	equ	0x00120824 
global AsyncBuffers 
SyncRequestDevsTop	equ	0x00119008 
global SyncRequestDevsTop 
Schedule	equ	0x00101cf7 
global Schedule 
SyncRequestDevs	equ	0x00122180 
global SyncRequestDevs 
PutBlk	equ	0x00109157 
global PutBlk 
Current	equ	0x00120b60 
global Current 
SyncMInodes	equ	0x0010a8ca 
global SyncMInodes 
WriteBufferToDisk	equ	0x00108ce7 
global WriteBufferToDisk 
HashTable	equ	0x00122380 
global HashTable 
printk	equ	0x00106461 
global printk 
DEP_VERSION dd 0x1 
global DEP_VERSION
REQ_PRIO dd 0x0 
global REQ_PRIO
