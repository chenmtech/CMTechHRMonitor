/*
 * written by chenm
*/

#ifndef DEV_SI7021_H
#define DEV_SI7021_H

#include "hal_i2c.h"

// ��ʪ�Ƚṹ�嶨��
typedef struct SI7021_HumiAndTemp {
  uint16 humid;          // ʪ��ֵ����λ��0.01%R.H.
  int16 temp;            // �¶�ֵ����λ��0.01���϶�
} SI7021_HumiAndTemp;



//��ʪ��
extern uint16 SI7021_MeasureHumidity();

//��ʪ��֮����¶�
extern int16 SI7021_ReadTemperature();

//���¶�
extern int16 SI7021_MeasureTemperature();

//ͬʱ��ʪ�Ⱥ��¶�
extern SI7021_HumiAndTemp SI7021_Measure();

// ͬʱ��ʪ�Ⱥ��¶ȣ�����������
extern void SI7021_MeasureData(uint8* pData);

 
#endif

