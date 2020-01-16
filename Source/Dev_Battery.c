/*
* Dev_Battery: �����ѹ��ͨ��ADC������ص���
* Written by Chenm 2019-02-24
*/


#include "Dev_Battery.h"

// ������ص����ٷֱ�
extern uint8 Battery_Measure()
{
  HalAdcInit();
  uint16 result = HalAdcRead (HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_14);
  
  //return (uint8)(result>>6);
  return (uint8)((result>>6)*33/128);
}