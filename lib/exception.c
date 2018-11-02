#include "exception.h"
#include "printk.h"
#include "sched.h"
#include "process.h"
#include "exit.h"
#include "asm/asm.h"

void panic(const char *str)
{
	debug_printk("[PANIC] %s", str);
	printk(str);
	while (1)
		;	
}

void die(const char *str)
{
	printk(str);
	printk("Process %s(PID = %d) was died.", Current->name, Current->pid);
	debug_printk("[PM] Process %s(PID = %d) died.", Current->name, Current->pid);
	while (1)
		;
	SysExit(13);
	while (1)
		;
}
