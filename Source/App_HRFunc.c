
#include "App_HRFunc.h"
#include "CMUtil.h"
#include "Dev_ADS1x9x.h"
#include "QRSDET.h"

// the flag of the initial beat
static uint8 initBeatFlag = 1 ;
// the sample count between RR interval
static uint16 rrSampleCount = 0 ;
// RR interval buffer, the max size of the buffer is 9
static uint16 rrBuf[9] = {0};
// the current RR interval number in rrBuf
static uint8 rrNum = 0;

static uint16 calRRInterval(int16 x);
static void processEcgData(int16 x, uint8 status);
static void process1mVData(int16 x, uint8 status);
static uint16 median(uint16 *array, uint8 datnum);


extern void HRFunc_Init()
{
  QRSDet(0, 1);
  initBeatFlag = 1;
  rrSampleCount = 0;
  rrNum = 0;  
  // initilize the ADS1x9x and set the ecg data process callback function
  ADS1x9x_Init(processEcgData);  
  delayus(1000);
}

extern void HRFunc_Start()
{  
  ADS1x9x_WakeUp();
  // 这里一定要延时，否则容易死机
  delayus(1000);
  ADS1x9x_StartConvert();
  delayus(1000);
}

extern void HRFunc_Stop()
{
  ADS1x9x_StopConvert();
  ADS1x9x_StandBy();
  delayus(2000);
}

// copy HR data to point p and return the length of data
extern uint8 HRFunc_CopyHRDataInto(uint8* p)
{
  if(rrNum == 0) return 0;
  
  // calculate BPM with average method
  /*
  int32 sum = 0;
  for(i = 0; i < rrNum; i++)
  {
    sum += rrBuf[i];
  }
  int16 BPM = (7500L*rrNum + (sum>>1))/sum; // BPM = (60*1000ms)/(RRInterval*8ms) = 7500/RRInterval, the round op is done
  if(BPM > 255) BPM = 255;
  */
  
  // calculate BPM with median method
  uint16 rrMedian = median(rrBuf, rrNum);
  int16 BPM = 7500L/rrMedian; // BPM = (60*1000ms)/(RRInterval*8ms) = 7500/RRInterval
  if(BPM > 255) BPM = 255;
  
  uint8* pTmp = p;
  
  /*
  //include bpm only
  *p++ = 0x00;
  *p++ = (uint8)BPM;
  */

  //include bpm and RRInterval
  *p++ = 0x10;
  *p++ = (uint8)BPM;
  uint16 MS1024 = 0;
  for(int i = 0; i < rrNum; i++)
  {
    // MS1024 = (uint16)(rrBuf[i]*8.192); // transform into the number with 1/1024 second unit, which is required in BLE.
    // *p++ = LO_UINT16(MS1024);
    // *p++ = HI_UINT16(MS1024);
    *p++ = LO_UINT16(rrBuf[i]);
    *p++ = HI_UINT16(rrBuf[i]);
  }
  
  /*
  // include bpm and Q&N as RRInterval for debug
  *p++ = 0x10;
  *p++ = (uint8)BPM;
  int* pQRS = getQRSBuffer();
  int* pNoise = getNoiseBuffer();
  *p++ = LO_UINT16(*pQRS);
  *p++ = HI_UINT16(*pQRS++);
  *p++ = LO_UINT16(*pQRS);
  *p++ = HI_UINT16(*pQRS++);
  *p++ = LO_UINT16(*pQRS);
  *p++ = HI_UINT16(*pQRS++);
  *p++ = LO_UINT16(*pQRS);
  *p++ = HI_UINT16(*pQRS++);
  *p++ = LO_UINT16(*pNoise);
  *p++ = HI_UINT16(*pNoise++);
  *p++ = LO_UINT16(*pNoise);
  *p++ = HI_UINT16(*pNoise++);
  *p++ = LO_UINT16(*pNoise);
  *p++ = HI_UINT16(*pNoise++);
  *p++ = LO_UINT16(*pNoise);
  *p++ = HI_UINT16(*pNoise++);  
  */
  
  rrNum = 0;
  return (uint8)(p-pTmp);
}

static void processEcgData(int16 x, uint8 status)
{
  if(!status)
  {
    uint16 RR = calRRInterval(x);
    if(RR == 0) return;
    rrBuf[rrNum++] = RR;
    if(rrNum >= 9) rrNum = 8;
  }
}

static uint16 calRRInterval(int16 x)
{
  uint16 RR = 0;
  int16 detectDelay = 0;
  
  rrSampleCount++;
  detectDelay = QRSDet(x, 0);
  
  if(detectDelay != 0)
  {
    if(initBeatFlag)
    {
      initBeatFlag = 0;
    }
    else
    {
      RR = (uint16)(rrSampleCount - detectDelay);
    }
    rrSampleCount = detectDelay;
    return RR;
  }
 
  return 0;
}

static uint16 median(uint16 *array, uint8 datnum)
{
  uint8 i, j;
  uint8 half = (datnum>>1);
  uint16 tmp, sort[9] ;
  osal_memcpy(sort, array, 2*datnum);
  // only half of data need to be sorted for finding out the median
  for(i = 0; i <= half; ++i)
  {
    for(j = i+1; j < datnum; j++)
    {
      if(sort[j] < sort[i])
      {
        tmp = sort[i];
        sort[i] = sort[j];
        sort[j] = tmp;
      }
    }
  }
  return(sort[half]);
}

static void process1mVData(int16 x, uint8 status)
{
  static int16 data1mV[125] = {0};
  static uint8 index = 0;
  
  if(index < 125)
  {
    data1mV[index++] = x;
  }
  else
  {
    
  }
}