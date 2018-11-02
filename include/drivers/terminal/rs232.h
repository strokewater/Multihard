#ifndef RS232_H
#define RS232_H

#define NR_RS232_CHANEEL	2

#define RS232_CHANNEL1_BASE	0x3f8
#define RS232_CHANNEL2_BASE	0x2f8

struct tty_struct;

extern void RS1IntHandle();
extern void RS2IntHandle();

extern void RS232Init(void);
extern void RS232ChangeSpeed(int port, unsigned speed);
extern void RS232Write(struct tty_struct *tty);

#endif
