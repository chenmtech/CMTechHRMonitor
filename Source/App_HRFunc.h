/*
 * App_HRFunc.h : 心率计算模块
 * Written by Chenm
 */

#ifndef APP_HRFUNC_H
#define APP_HRFUNC_H

#include "hal_types.h"

extern void HRFunc_Init();
extern void HRFunc_Start();
extern void HRFunc_Stop();
// copy HR data to point p and return the length of data
extern uint8 HRFunc_CopyHRData(uint8* p);

#endif