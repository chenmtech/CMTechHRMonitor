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
extern void HRFunc_SetEcgSent(bool send); // is the ecg data sent?
extern void HRFunc_SetHRCalculated(bool calculate); // is the Heart rate calculated?
extern void HRFunc_SendHRData(uint16 connHandle); // copy HR data into p and return the length of data
extern void HRFunc_SendEcgPacket(uint16 connHandle);

#endif