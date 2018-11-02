TOP_DIR = ..
SRC_ROOT= ..

include ../Makefile.config

LDFLAGS = -m elf_i386

DEP_TOOL = $(TOP_DIR)/tools/coredep.py
KERNEL = $(TOP_DIR)/kernel

all: disk

disk: disk.c disk_dep.dep $(DEP_TOOL) $(KERNEL)
	$(DEP_TOOL) $(KERNEL) disk_dep
	$(ASM) $(ASMFLAGS) disk_dep.asm -o disk_dep.o 2>/dev/null
	$(CC) $(CFLAGS) -c disk.c -o disk.o
	$(LD) $(LDFLAGS) disk.o disk_dep.o  -o disk
	
clean:
	rm -rf $
	rm -rf disk_dep.asm disk_dep.o disk.o disk
