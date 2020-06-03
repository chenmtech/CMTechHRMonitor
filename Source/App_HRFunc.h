/*
 * App_HRFunc.h : Heart Rate application Function Model header file
 * Written by Chenm
 */

#ifndef APP_HRFUNC_H
#define APP_HRFUNC_H

#include "hal_types.h"

extern void HRFunc_Init(uint8 taskID); //init
extern void HRFunc_SetEcgSampling(bool start); // is the ecg sampling started
extern void HRFunc_SetHRCalcing(bool calc); // is the Heart rate calculated?
extern void HRFunc_SetEcgSending(bool send); // is the ecg data sent?
extern void HRFunc_SendHRPacket(uint16 connHandle); // send HR packet
extern void HRFunc_SendEcgPacket(uint16 connHandle); // send ecg packet

#endif