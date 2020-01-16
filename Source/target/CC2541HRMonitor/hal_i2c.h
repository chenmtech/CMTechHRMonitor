
/**
* 我接手改写这个I2C模块
* by chenm 2017-04-27

* 所有与Slave mode相关的代码都删除了，仅用于Master mode
* by chenm 2017-05-12

* 在TI提供的SensorTag项目中发现他们编写的I2C代码，借鉴过来进行修改
* 改动比较大，接口函数都变了。
* by chenm 2017-09-16

*/


#ifndef HAL_I2C_H
#define HAL_I2C_H

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

//#include "hal_types.h"
#include "comdef.h"


/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

// CC2541支持的时钟频率
typedef enum {
  i2cClock_123KHZ = 0x00,
  i2cClock_144KHZ = 0x01,
  i2cClock_165KHZ = 0x02,
  i2cClock_197KHZ = 0x03,
  i2cClock_33KHZ  = 0x80,
  i2cClock_267KHZ = 0x81,
  i2cClock_533KHZ = 0x82
} i2cClock_t;


/* ------------------------------------------------------------------------------------------------
 *                                       Global Functions
 * ------------------------------------------------------------------------------------------------
 */

/********************************************************************************************
* 初始化I2C模块，设置I2C从机的地址，以及通信的时钟频率
* 不是说开始初始化一次就可以
* 而是，每次读取数据之前，都要调用此函数初始化
* @param address : 设备地址，是指没有右移一位的地址，内部会右移一位
* @param clockRate : I2C的时钟频率
*/ 
extern void HalI2CInit(uint8 address, i2cClock_t clockRate);

// 读指定长度数据
extern uint8 HalI2CRead(uint8 len, uint8 *pBuf);

// 写指定长度数据
extern uint8 HalI2CWrite(uint8 len, uint8 *pBuf);

// disable I2C模块, 主要用在hal_sleep中在休眠模式下关闭I2C
extern void HalI2CDisable(void);

extern void HalI2CSetAsGPIO();

#endif /*  HAL_I2C_H  */