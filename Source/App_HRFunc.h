/*
 * App_HRFunc.h : Heart Rate Calculation Function Model header file
 * Written by Chenm
 */

#ifndef APP_HRFUNC_H
#define APP_HRFUNC_H

#include "hal_types.h"

extern void HRFunc_Init();
extern void HRFunc_Start();
extern void HRFunc_Stop();
extern uint8 HRFunc_CopyHRDataInto(uint8* p); // copy HR data into p and return the length of data


#endif