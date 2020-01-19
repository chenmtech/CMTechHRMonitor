
#include <iocc2541.h>
#include "hal_drivers.h"
#include "hal_mcu.h"
#include "osal.h"
#include "CMUtil.h"
//#include "CMTechECGMonitor.h"
#include "App_ECGFunc.h"
#include "Dev_ADS1x9x.h"
//#include "Service_ECGMonitor.h"
//#include "DCand50HzFilter.h"


#define STATE_STOP 0x00 // 停止状态
#define STATE_START_ECG 0x01 // 启动采集ECG状态
#define STATE_START_1MV 0x02 // 启动采集1mV定标信号状态


static uint8 state = STATE_STOP; // 当前状态

// 数据包，指向Service_ECGMonitor中的ecgData
static uint8 *pBuf;

// 当前数据包中已经保存的数据字节数
static uint8 byteCnt = 0;

static int packNum = 0;




/*****************************************************
 * 局部函数
 */
static void ECGFunc_ProcessDataCB(uint8 low, uint8 high);

// 处理采集到的一个数据
static void ECGFunc_ProcessDataCB(uint8 low, uint8 high)
{
  if(state == STATE_STOP) return;
  
  if(byteCnt == 0) {
    *pBuf++ = (uint8)(packNum & 0x00FF);
    *pBuf++ = (uint8)((packNum >> 8) & 0x00FF);
    byteCnt += 2;
    if(++packNum == 16) packNum = 0;
  }
  
  *pBuf++ = low;  
  *pBuf++ = high;
  byteCnt += 2;

  // 达到数据包长度
  if(byteCnt == ECG_PACKET_LEN)
  {
    byteCnt = 0;
    pBuf = ECGMonitor_GetECGDataPointer();
    ECGMonitor_SetParameter( ECGMONITOR_DATA, ECG_PACKET_LEN, pBuf );  
  }  
  /*
  if(i < 512)
    buf[i++] = data;//DCAND50HZ_IntegerFilter(data);
  else
  {
    i = 0;
  }
  */ 
  //DATATYPE d = (DATATYPE)FPFIRLPF1_Filter( DCAND50HZ_IntegerFilter(data) );

  //d = d >> 2;   //这里的右移主要是适应手机端的屏幕显示尺度，实际上可以不用右移，在手机端调整尺度  
}


/*****************************************************
 * 公共函数
 */

extern void ECGFunc_Init()
{
  state = STATE_STOP;
  
  // 初始化ADS1x9x，设置数据处理回调函数
  ADS1x9x_Init(ECGFunc_ProcessDataCB);
  
  // 获取ECG DATA数据空间指针
  pBuf = ECGMonitor_GetECGDataPointer();
  
  byteCnt = 0;
  
  packNum = 0;
  
  delayus(1000);
  
}

extern void ECGFunc_StartEcg()
{
  switch(state) {
    case STATE_START_ECG:
      return;
    case STATE_START_1MV:
      ADS1x9x_StopConvert();
      break;
    case STATE_STOP:
      ADS1x9x_WakeUp();
      break;
    default:
      return;
  }
  
  // 这里一定要延时，否则容易死机
  delayus(1000);
  
  ADS1x9x_SetRegsAsNormalECGSignal();
  byteCnt = 0;
  packNum = 0;
  pBuf = ECGMonitor_GetECGDataPointer();
  
  delayus(1000);
  
  ADS1x9x_StartConvert();
  state = STATE_START_ECG;
  
  delayus(1000);
}

extern void ECGFunc_Start1mV()
{
  switch(state) {
    case STATE_START_1MV:
      return;
    case STATE_START_ECG:
      ADS1x9x_StopConvert();
      break;
    case STATE_STOP:
      ADS1x9x_WakeUp();
      break;
    default:
      return;
  }
  
  // 这里一定要延时，否则容易死机
  delayus(1000);

  ADS1x9x_SetRegsAsTestSignal();  
  byteCnt = 0;
  packNum = 0;
  pBuf = ECGMonitor_GetECGDataPointer();
  
  delayus(1000);
  
  ADS1x9x_StartConvert();
  state = STATE_START_1MV;
  
  delayus(1000);
}

extern void ECGFunc_Stop()
{
  if(state != STATE_STOP)
  {
    ADS1x9x_StopConvert();
    state = STATE_STOP; 
    ADS1x9x_StandBy();
  }
  
  delayus(2000);
}


