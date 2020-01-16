
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

#define I2C_READ_BIT        1   // ������λ
#define I2C_WRITE_BIT       0   // д����λ

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
static uint8 i2cAddr;  // ������������һλ֮��ĵ�ַ


/**************************************************************************************************
 * @fn          i2cMstStrt
 *
 * @brief       ����I2C��ģʽ����������START���Լ�Slave Address+R/~W
 *
 * input parameters
 *
 * @param       address - �ӻ���ַ��һ��Ҫ7λ�ġ�����ĵ��ṩ����8λ��ַ��Ҫ����һλ
 * @param       RD_WRn - 1����ʾ����Ϊ��������0����ʾ����Ϊд����
 *
 * output parameters
 *
 * None.
 *
 * @return      I2C��״̬.
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
* ��ʼ��I2Cģ�飬����I2C�ӻ��ĵ�ַ���Լ�ͨ�ŵ�ʱ��Ƶ��
* ����˵��ʼ��ʼ��һ�ξͿ���
* ���ǣ�ÿ�ζ�ȡ����֮ǰ����Ҫ���ô˺�����ʼ��
* @param address : �豸��ַ����ָû������һλ�ĵ�ַ���ڲ�������һλ
* @param clockRate : I2C��ʱ��Ƶ��
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
 * @brief       ��ָ�����ȵ�����

 * @param       len - �������ֽڳ���
 * @param       pBuf - �洢���ݵĻ�����ָ��
 *
 * @return      �ɹ���ȡ���ֽڳ���
 */
extern uint8 HalI2CRead(uint8 len, uint8 *pBuf)
{
  uint8 cnt = 0;

  if (i2cMstStrt(I2C_READ_BIT) != mstAddrAckR)
  {
    //��ַ���ʹ���
    I2C_STOP();
    return 0;
  }

  // ����I2C��Э�飬ÿ�ζ�ȡһ���ֽ�����ʱҪ����ACK Bit������ȡ���һ���ֽ�ʱҪ����NACK Bit
  // ��ˣ����len > 1��������ΪACK Bit
  if (len > 1)
  {
    I2C_SET_ACK();
  }

  while (len > 0)
  {
    // ʣ�����һ���ֽڣ�����ΪNACK Bit
    if (len == 1)
    {
      I2C_SET_NACK();
    }

    // ��һ���ֽ�
    I2C_READ(*pBuf++);
    cnt++;
    len--;

    if (I2CSTAT != mstDataAckR)
    {
      if (I2CSTAT != mstDataNackR)
      {
        // ��������
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
 * @brief       дһ�����ȵ�����
 *
 * @param       address - �ӻ�7λ��ַ
 * @param       len - д�����ֽڳ���
 * @param       pBuf - ���ݻ�����ָ��
 *
 *
 * @return      �ɹ�д��������ֽڳ���
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
    // д��һ���ֽ�
    I2C_WRITE(*pBuf++);

    if (I2CSTAT != mstDataAckW)
    {
      if (I2CSTAT == mstDataNackW)
      {
        // д���һ���ֽ�
        len = cnt + 1;
      }
      else
      {
        // ���һ��д����
        len = cnt;
      }
      break;
    }
  }

  I2C_STOP();
  return len;
}

// һ��Ҫ��I2C�Ľӿ���ΪGPIO����ʡ��
extern void HalI2CSetAsGPIO()
{
  // I2C��SDA, SCL����ΪGPIO
  I2CWC = 0x8C;   //GPIO,000,SCL pullup Eable, SDA pullup Eable, SCL output disable, SDA output disable
  I2CIO = 0x03;   //000000, SCL Output register=1, SDA Output register=1
}

// disable I2Cģ��
extern void HalI2CDisable(void)
{
  I2C_DISABLE();
}


/**************************************************************************************************
*/
