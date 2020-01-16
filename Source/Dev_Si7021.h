/*
 * written by chenm
*/

#ifndef DEV_SI7021_H
#define DEV_SI7021_H

#include "hal_i2c.h"

// 温湿度结构体定义
typedef struct SI7021_HumiAndTemp {
  uint16 humid;          // 湿度值，单位：0.01%R.H.
  int16 temp;            // 温度值，单位：0.01摄氏度
} SI7021_HumiAndTemp;



//测湿度
extern uint16 SI7021_MeasureHumidity();

//测湿度之后读温度
extern int16 SI7021_ReadTemperature();

//测温度
extern int16 SI7021_MeasureTemperature();

//同时测湿度和温度
extern SI7021_HumiAndTemp SI7021_Measure();

// 同时测湿度和温度，并返回数组
extern void SI7021_MeasureData(uint8* pData);

 
#endif

