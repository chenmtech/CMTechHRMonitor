/*
* Dev_Battery: 通过ADC测量电池电量
* Written by Chenm 2019-02-24
*/


#include "Dev_Battery.h"

// 测量电池电量百分比
extern uint8 Battery_Measure()
{
  uint8 percent = 0;
  HalAdcSetReference( HAL_ADC_REF_125V );
  uint16 adc = HalAdcRead( HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_10 );
  
  // convert adc to percent of battery level
  // The internal reference voltage is 1.24V in CC2541
  // if V is input voltage, then the output adc is:
  // adc = (V/3)/1.24*511
  // MAXadc = (3/3)/1.24*511 = 412
  // MINadc = (2.7/3)/1.24*511 = 370 , where 2.7V is the lowest voltage of ADS1291
  // so the percent value is equal to:
  // percent = (adc - MINadc)*100/(412-370) = (adc - 370)*100/42
  
  if(adc >= 412)
  {
    percent = 100;
  } 
  else if(adc <= 370)
  {
    percent = 0;
  }
  else
  {
    percent = (uint8)( (adc-370)*100/42 );
  }  
  
  return percent;
}