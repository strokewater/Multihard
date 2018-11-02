#ifndef TSC_H
#define TSC_H

#include "types.h"

/*
 * 使用TSC获得高精度时间， 因为TSC为64位，为了避免引入libgcc，因此将时间分块处理
 * 将TSC当作计数器使用，每过 tail纳秒计数器加一，
 * 计数时间超过full纳秒则对计数器取余数(除以full纳秒)
 * TSC可以补充PIT的精度
 */

void InitTSC(u32 tail, u32 full);
void SetStartupTailTime(u32 tail);
u32 GetTSCTailTime();

#endif
