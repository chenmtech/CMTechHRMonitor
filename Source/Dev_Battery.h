/**
* 通过电阻分压和ADC采样，获取电池电量百分比数据
*/

#ifndef DEV_BATTERY_H
#define DEV_BATTERY_H

#include "hal_adc.h"

// measure battery level
extern uint8 Battery_Measure();

#endif
