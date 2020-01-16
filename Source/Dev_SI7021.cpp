/*
* Dev_Si7021: 温湿度传感器Si7021的驱动
* Written by Chenm 2017-04-29

* 发现每次调用测量湿度或温度的函数，必须用HalI2CEnable和HalI2CDisable括起来
* 否则只能读一次，就退出
* Written by Chenm 2017-05-10

* 搞清楚了I2C就是要每次读数据之前都要调用HalI2CInit(其中Enable I2C)初始化
* 否则就会出现上述错误
* Written by Chenm 2018-03-06
*/


#include "Dev_SI7021.h"
#include "osal.h"

/*
* 常量
*/
#define I2C_ADDR 0x40   //Si7021的I2C地址

// I2C命令
#define I2CCMD_MEASURE_HUMIDITY	    0xE5      //测湿度

#define I2CCMD_READ_TEMPERATURE	    0xE0      //紧跟测湿度之后读温度

#define I2CCMD_MEASURE_TEMPERATURE  0xE3      //测温度

/*
* 局部变量
*/
// 发送的命令值
static uint8 cmd = 0;

// 读取的2字节数据
static uint8 data[2] = {0};

/*
* 局部函数
*/

//启动：设置Slave Address和SCLK频率
static void SI7021_Start()
{
  HalI2CInit(I2C_ADDR, i2cClock_267KHZ);
}

//停止SI7021
static void SI7021_Stop()
{
  HalI2CDisable();
}

/*
* 公共函数
*/

// 测量湿度值
// 返回单位：0.01%R.H.
extern uint16 SI7021_MeasureHumidity()
{
  SI7021_Start();
  
  //先发命令，再读数据
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

// 必须在测湿度之后紧接着读温度
// 返回单位：0.01摄氏度
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

// 单独测温度
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

//同时测湿度和温度
extern SI7021_HumiAndTemp SI7021_Measure()
{
  SI7021_HumiAndTemp rtn;
  rtn.humid = SI7021_MeasureHumidity();
  rtn.temp = SI7021_ReadTemperature();
  return rtn;
}
