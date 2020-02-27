/*
 * App_HRFunc.h : Heart Rate Calculation Function Model header file
 * Written by Chenm
 */

#ifndef APP_HRFUNC_H
#define APP_HRFUNC_H

#include "hal_types.h"

// initialize the model, including 
extern void HRFunc_Init();
// start 1mV calibration
extern void HRFunc_Start1mVCali();
extern void HRFunc_Start();
extern void HRFunc_Stop();
// copy HR data into p and return the length of data
extern uint8 HRFunc_CopyHRDataInto(uint8* p);

#endif