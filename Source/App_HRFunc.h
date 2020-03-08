/*
 * App_HRFunc.h : Heart Rate Calculation Function Model header file
 * Written by Chenm
 */

#ifndef APP_HRFUNC_H
#define APP_HRFUNC_H

#include "hal_types.h"

extern void HRFunc_Init(); //init
extern void HRFunc_StartSamplingEcg(bool start); // is the ecg signal sampled
extern void HRFunc_StartCalcingHR(bool calc); // is the Heart rate calculated?
extern void HRFunc_StartSendingEcg(bool send); // is the ecg data sent?
extern void HRFunc_SendHRPacket(uint16 connHandle); // send HR packet
extern void HRFunc_SendEcgPacket(uint16 connHandle); // send ecg packet

#endif