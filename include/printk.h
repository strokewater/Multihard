#ifndef LIB_PRINTK_H
#define LIB_PRINTK_H

#define XCHARS	80
#define YCHARS	26

void printk(const char *format, ...);
void debug_printk(const char *format, ...);
extern int debug_msg_no;

#endif
