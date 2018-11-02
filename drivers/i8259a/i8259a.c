#include "types.h"
#include "printk.h"
#include "drivers/i8259a/i8259a.h"


static void IOWait(void)
{
	return;
}

void EnableIRQ(int mask)
{
	debug_printk("[IO] EnableIRQ mask = %x", mask);
	if ((mask >> 8) != 0)
		__asm__("inb $0xA1, %%al;" \
			  "andb  %%bl, %%al;" \
			  "outb  %%al, $0xA1;" : :"b"(~(mask >> 8)) : );
	else
		__asm__("inb $0x21, %%al;" \
			  "andb  %%bl, %%al;" \
			  "outb  %%al, $0x21;" : :"b"(~mask) : );
}

void DisableIRQ(int mask)
{
	debug_printk("[IO] DisableIRQ mask = %x", mask);
	if ((mask >> 8) != 0)
		__asm__("inb $0xA1, %%al;" \
				  "orb  %%bl, %%al;" \
				  "outb %%al, $0xA1;" : : "b"(mask >> 8));
	else
		__asm__("inb $0x21, %%al;" \
				  "orb  %%bl, %%al;" \
				  "outb %%al, $0x21;" : : "b"(mask));
}

void Init8259A()
{
	// Master 8259, ICW1
	__asm__("movb $0x11, %%al;":::);
	__asm__("outb  %%al, $0x20;":::);
	IOWait();

	// Slave 8259, ICW1
	__asm__("movb	$0x11, %%al;":::);
	__asm__("outb	%%al, $0xA0;":::);
	IOWait();

	// Master 8259, ICW2. IR0's int vector = INT_NO_TIMER
	__asm__("movb	%0, %%al;"::"K"(INT_NO_IRQ_TIMER):);
	__asm__("outb	%%al, $0x21;":::);
	IOWait();

	// Slave 8259, ICW2
	__asm__("movb	%0, %%al;"::"K"(INT_NO_IRQ_REAL_CLOCK):);
	__asm__("outb	%%al, $0xA1;":::);
	IOWait();

	// Master 8259, ICW3
	__asm__("movb	$4, %%al;" :::);
	__asm__("outb	%%al, $0x21;" :::);
	IOWait();
	
	// Slave 8259, ICW3
	__asm__("movb	$2, %%al;"  :::);
	__asm__("outb	%%al, $0xA1;" :::); 
	IOWait(); 

	// Master 8259, ICW4
	__asm__("movb	$0x1, %%al;" :::);
	__asm__("outb	%%al, $0x21;" :::);
	IOWait();

	// Slave 8259, ICW4
	__asm__("movb	$0x1, %%al;" :::);
	__asm__("outb	%%al, $0xA1;" :::);
	IOWait();

	// Mask all interupt from Master 8259
	__asm__("movb	$0xff, %%al;" :::);
	__asm__("outb	%%al, $0x21;" :::);
	IOWait(); 
	
	// Mask all interupt from Slave 8259
	__asm__("movb	$0xff, %%al;" :::);
	__asm__("outb	%%al, $0xA1;" :::);
	IOWait();
}
