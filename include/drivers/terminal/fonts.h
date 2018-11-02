#ifndef DRIVERS_TTY_FONTS_H
#define DRIVERS_TTY_FONTS_H

extern int XCharSize;
extern int YCharSize;

int ReadFont(u32 ch, u8 shape[]);
void FontsInit();

#endif
