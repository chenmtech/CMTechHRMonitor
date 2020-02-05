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
extern uint8 HRFunc_SetData(uint8* p);

#endif