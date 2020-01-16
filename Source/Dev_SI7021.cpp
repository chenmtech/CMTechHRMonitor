/*
* Dev_Si7021: ��ʪ�ȴ�����Si7021������
* Written by Chenm 2017-04-29

* ����ÿ�ε��ò���ʪ�Ȼ��¶ȵĺ�����������HalI2CEnable��HalI2CDisable������
* ����ֻ�ܶ�һ�Σ����˳�
* Written by Chenm 2017-05-10

* �������I2C����Ҫÿ�ζ�����֮ǰ��Ҫ����HalI2CInit(����Enable I2C)��ʼ��
* ����ͻ������������
* Written by Chenm 2018-03-06
*/


#include "Dev_SI7021.h"
#include "osal.h"

/*
* ����
*/
#define I2C_ADDR 0x40   //Si7021��I2C��ַ

// I2C����
#define I2CCMD_MEASURE_HUMIDITY	    0xE5      //��ʪ��

#define I2CCMD_READ_TEMPERATURE	    0xE0      //������ʪ��֮����¶�

#define I2CCMD_MEASURE_TEMPERATURE  0xE3      //���¶�

/*
* �ֲ�����
*/
// ���͵�����ֵ
static uint8 cmd = 0;

// ��ȡ��2�ֽ�����
static uint8 data[2] = {0};

/*
* �ֲ�����
*/

//����������Slave Address��SCLKƵ��
static void SI7021_Start()
{
  HalI2CInit(I2C_ADDR, i2cClock_267KHZ);
}

//ֹͣSI7021
static void SI7021_Stop()
{
  HalI2CDisable();
}

/*
* ��������
*/

// ����ʪ��ֵ
// ���ص�λ��0.01%R.H.
extern uint16 SI7021_MeasureHumidity()
{
  SI7021_Start();
  
  //�ȷ�����ٶ�����
  cmd = I2CCMD_MEASURE_HUMIDITY;
  HalI2CWrite(1, &cmd);  
  HalI2CRead(2, data);
  
  SI7021_Stop();
  
  long value = ( (((long)data[0] << 8) + data[1]) & ~3 );  
  value = ( ((value*3125)>>14)-600 );
  if(value < 0)
    value = 0;
  else if(value > 10000)
    value = 10000;
  return (uint16)value;
}

// �����ڲ�ʪ��֮������Ŷ��¶�
// ���ص�λ��0.01���϶�
extern int16 SI7021_ReadTemperature()
{
  SI7021_Start();
  
  cmd = I2CCMD_READ_TEMPERATURE;
  HalI2CWrite(1, &cmd);  
  HalI2CRead(2, data);
  
  SI7021_Stop();
  
  long value = ( (((long)data[0] << 8) + data[1]) & ~3 );    
  value = ((value*4393)>>14)-4685;  
  return (int16)value;
}

// �������¶�
extern int16 SI7021_MeasureTemperature()
{
  SI7021_Start();
  
  cmd = I2CCMD_MEASURE_TEMPERATURE;
  HalI2CWrite(1, &cmd);  
  HalI2CRead(2, data);
  
  SI7021_Stop();
  
  long value = ( (((long)data[0] << 8) + data[1]) & ~3 );    
  value = ((value*4393)>>14)-4685;  
  return (int16)value;  
}

//ͬʱ��ʪ�Ⱥ��¶�
extern SI7021_HumiAndTemp SI7021_Measure()
{
  SI7021_HumiAndTemp rtn;
  rtn.humid = SI7021_MeasureHumidity();
  rtn.temp = SI7021_ReadTemperature();
  return rtn;
}
