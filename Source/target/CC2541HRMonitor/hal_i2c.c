
/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
//#include "cc_global.h"
//#include <ioCC2541.h>
#include "hal_board_cfg.h"
#include "hal_assert.h"
#include "hal_i2c.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

#define I2C_ENS1            BV(6)
#define I2C_STA             BV(5)
#define I2C_STO             BV(4)
#define I2C_SI              BV(3)
#define I2C_AA              BV(2)

#define I2C_READ_BIT        1   // 读操作位
#define I2C_WRITE_BIT       0   // 写操作位

#define I2C_CLOCK_MASK      0x83

#define I2C_PXIFG           P2IFG
#define I2C_IF              P2IF
#define I2C_IE              BV(1)

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

typedef enum {
  // HAL_I2C_MASTER mode statuses.
  mstStarted   = 0x08,
  mstRepStart  = 0x10,
  mstAddrAckW  = 0x18,
  mstAddrNackW = 0x20,
  mstDataAckW  = 0x28,
  mstDataNackW = 0x30,
  mstLostArb   = 0x38,
  mstAddrAckR  = 0x40,
  mstAddrNackR = 0x48,
  mstDataAckR  = 0x50,
  mstDataNackR = 0x58,

  // HAL_I2C_MASTER/SLAVE mode statuses.
  i2cIdle      = 0xF8
} i2cStatus_t;

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

#define I2C_WRAPPER_DISABLE() st( I2CWC    =     0x00;              )
#define I2C_CLOCK_RATE(x)     st( I2CCFG  &=    ~I2C_CLOCK_MASK;    \
                                  I2CCFG  |=     x;                 )

#define I2C_SET_NACK()  st( I2CCFG &= ~I2C_AA; )
#define I2C_SET_ACK()   st( I2CCFG |=  I2C_AA; )

// Master to Ack to every byte read from the Slave; Slave to Ack Address & Data from the Master.
#define I2C_ENABLE()          st( I2CCFG |= (I2C_ENS1); )
#define I2C_DISABLE()         st( I2CCFG &= ~(I2C_ENS1); )

// Must clear SI before setting STA and then STA must be manually cleared.
#define I2C_STRT() st (             \
  I2CCFG &= ~I2C_SI;                \
  I2CCFG |= I2C_STA;                \
  while ((I2CCFG & I2C_SI) == 0);   \
  I2CCFG &= ~I2C_STA;               \
)

// Must set STO before clearing SI.
#define I2C_STOP() st (             \
  I2CCFG |= I2C_STO;                \
  I2CCFG &= ~I2C_SI;                \
  while ((I2CCFG & I2C_STO) != 0);  \
)

// Stop clock-stretching and then read when it arrives.
#define I2C_READ(_X_) st (          \
  I2CCFG &= ~I2C_SI;                \
  while ((I2CCFG & I2C_SI) == 0);   \
  (_X_) = I2CDATA;                  \
)

// First write new data and then stop clock-stretching.
#define I2C_WRITE(_X_) st (         \
  I2CDATA = (_X_);                  \
  I2CCFG &= ~I2C_SI;                \
  while ((I2CCFG & I2C_SI) == 0);   \
)


/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables and Functions
 * ------------------------------------------------------------------------------------------------
 */
static uint8 i2cAddr;  // 用来保存右移一位之后的地址


/**************************************************************************************************
 * @fn          i2cMstStrt
 *
 * @brief       启动I2C主模式，包括发送START，以及Slave Address+R/~W
 *
 * input parameters
 *
 * @param       address - 从机地址，一定要7位的。如果文档提供的是8位地址，要右移一位
 * @param       RD_WRn - 1：表示后面为读操作；0：表示后面为写操作
 *
 * output parameters
 *
 * None.
 *
 * @return      I2C的状态.
 */
static uint8 i2cMstStrt(uint8 RD_WRn)
{
  I2C_STRT();      

  if (I2CSTAT == mstStarted)
  {
    I2C_WRITE(i2cAddr | RD_WRn);
  }

  return I2CSTAT;
}

/* ------------------------------------------------------------------------------------------------
 *                                       Glocal Functions
 * ------------------------------------------------------------------------------------------------
 */

/********************************************************************************************
* 初始化I2C模块，设置I2C从机的地址，以及通信的时钟频率
* 不是说开始初始化一次就可以
* 而是，每次读取数据之前，都要调用此函数初始化
* @param address : 设备地址，是指没有右移一位的地址，内部会右移一位
* @param clockRate : I2C的时钟频率
*/
extern void HalI2CInit(uint8 address, i2cClock_t clockRate)
{
  i2cAddr = address << 1;
  
  I2C_WRAPPER_DISABLE();
  I2CADDR = 0; // no multi master support at this time
  I2C_CLOCK_RATE(clockRate);
  I2C_ENABLE();
}

/**************************************************************************************************
 * @fn          HalI2CRead
 *
 * @brief       读指定长度的数据

 * @param       len - 读数据字节长度
 * @param       pBuf - 存储数据的缓冲区指针
 *
 * @return      成功读取的字节长度
 */
extern uint8 HalI2CRead(uint8 len, uint8 *pBuf)
{
  uint8 cnt = 0;

  if (i2cMstStrt(I2C_READ_BIT) != mstAddrAckR)
  {
    //地址发送错误
    I2C_STOP();
    return 0;
  }

  // 根据I2C的协议，每次读取一个字节数据时要发送ACK Bit，而读取最后一个字节时要发送NACK Bit
  // 因此，如果len > 1，则设置为ACK Bit
  if (len > 1)
  {
    I2C_SET_ACK();
  }

  while (len > 0)
  {
    // 剩下最后一个字节，设置为NACK Bit
    if (len == 1)
    {
      I2C_SET_NACK();
    }

    // 读一个字节
    I2C_READ(*pBuf++);
    cnt++;
    len--;

    if (I2CSTAT != mstDataAckR)
    {
      if (I2CSTAT != mstDataNackR)
      {
        // 出问题了
        cnt--;
      }
      break;
    }
  }

  I2C_STOP();
  return cnt;
}

/**************************************************************************************************
 * @fn          HalI2CWrite
 *
 * @brief       写一定长度的数据
 *
 * @param       address - 从机7位地址
 * @param       len - 写数据字节长度
 * @param       pBuf - 数据缓冲区指针
 *
 *
 * @return      成功写入的数据字节长度
 */
extern uint8 HalI2CWrite(uint8 len, uint8 *pBuf)
{
  if (i2cMstStrt(I2C_WRITE_BIT) != mstAddrAckW)
  {
    I2C_STOP();
    return 0;
  }

  for (uint8 cnt = 0; cnt < len; cnt++)
  {
    // 写入一个字节
    I2C_WRITE(*pBuf++);

    if (I2CSTAT != mstDataAckW)
    {
      if (I2CSTAT == mstDataNackW)
      {
        // 写最后一个字节
        len = cnt + 1;
      }
      else
      {
        // 最后一个写错了
        len = cnt;
      }
      break;
    }
  }

  I2C_STOP();
  return len;
}

// 一定要把I2C的接口设为GPIO才能省电
extern void HalI2CSetAsGPIO()
{
  // I2C的SDA, SCL设置为GPIO
  I2CWC = 0x8C;   //GPIO,000,SCL pullup Eable, SDA pullup Eable, SCL output disable, SDA output disable
  I2CIO = 0x03;   //000000, SCL Output register=1, SDA Output register=1
}

// disable I2C模块
extern void HalI2CDisable(void)
{
  I2C_DISABLE();
}


/**************************************************************************************************
*/
