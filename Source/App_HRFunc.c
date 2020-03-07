
#include "App_HRFunc.h"
#include "CMUtil.h"
#include "Dev_ADS1x9x.h"
#include "QRSDET.h"
#include "Service_HRMonitor.h"
#include "service_ecg.h"
#if defined ( PLUS_BROADCASTER )
  #include "peripheralBroadcaster.h"
#else
  #include "peripheral.h"
#endif

#define ECG_DATA_NUM_PER_PACK 9 // ecg data number per packet

// is the heart rate calculated?
static bool hrCalc = false;
// the flag of the initial beat
static uint8 initBeatFlag = 1 ;
// the sample count between RR interval
static uint16 rrSampleCount = 0 ;
// RR interval buffer, the max number in the buffer is 9
static uint16 rrBuf[9] = {0};
// the current number in rrBuf
static uint8 rrNum = 0;
// 1mV calibration value, only used when CALIBRATE_1MV is set in preprocessing
static uint16 caliValue = 0;
// Heart rate measurement value stored in this structure
static attHandleValueNoti_t hrNoti;

// is the ecg data sent?
static bool ecgSent = false;
// the number of the current packet of ecg data, from 0 to 65535
static uint8 pckNum = 0;
static uint8* pEcgByte;
//static uint8 ecgByteCnt = 0;
static attHandleValueNoti_t ecgNoti;

static uint16 calRRInterval(int16 x);
static void processEcgSignal(int16 x, uint8 status);
static void processTestSignal(int16 x, uint8 status);
static uint16 median(uint16 *array, uint8 datnum);
static void saveEcgSignal(int16 ecg);

extern void HRFunc_Init()
{ 
  // initilize the ADS1x9x and set the data process callback function
#if defined(CALIBRATE_1MV)
  ADS1x9x_Init(processTestSignal);  
#else
  ADS1x9x_Init(processEcgSignal);  
#endif
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

extern void HRFunc_SetHRCalculated(bool calc)
{
  hrCalc = calc;
  if(calc)
  {
    QRSDet(0, 1);
    initBeatFlag = 1;
    rrSampleCount = 0;
    rrNum = 0; 
  }
}

extern void HRFunc_SetEcgSent(bool send)
{
  ecgSent = send;
  if(send)
  {
    pckNum = 0;
    pEcgByte = ecgNoti.value;
    //ecgByteCnt = 0;
    ecgNoti.len = 0;
  }
}

extern void HRFunc_SendEcgPacket(uint16 connHandle)
{
  if(ecgNoti.len <= 1) return;
  
  ECG_PacketNotify( connHandle, &ecgNoti );
  ecgNoti.len = 0;
  pEcgByte = ecgNoti.value; 
}

// copy HR data to point p and return the length of data
extern void HRFunc_SendHRData(uint16 connHandle)
{
  if(rrNum == 0) return;  // No RR interval, return
  
  uint8* p = hrNoti.value;
  
  //////// Two methods to calculate BPM
  // 1. using average method
  /*
  int32 sum = 0;
  for(i = 0; i < rrNum; i++)
  {
    sum += rrBuf[i];
  }
  int16 BPM = (7500L*rrNum + (sum>>1))/sum; // BPM = (60*1000ms)/(RRInterval*8ms) = 7500/RRInterval, the round op is done
  */
  
  // 2. using median method
  uint16 rrMedian = ((rrNum == 1) ? rrBuf[0] : median(rrBuf, rrNum));
  int16 BPM = 7500L/rrMedian; // BPM = (60*1000ms)/(RRInterval*8ms) = 7500/RRInterval
  ////////////////////////////////////////
  
  if(BPM > 255) BPM = 255;
  
  uint8* pTmp = p;
  
  ////////Three different way to output HR data
  /*
  //1. bpm only
  *p++ = 0x00;
  *p++ = (uint8)BPM;
  */

  //2. bpm and RRInterval
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
  // 3. bpm and Q&N as RRInterval for debug
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
  hrNoti.len = (uint8)(p-pTmp);
  HRM_MeasNotify( connHandle, &hrNoti );
  
  return;
}

static void processEcgSignal(int16 x, uint8 status)
{
  if(!status)
  {
    if(hrCalc)
    {
      uint16 RR = calRRInterval(x);
      if(RR != 0)
      {
        rrBuf[rrNum++] = RR;
        if(rrNum >= 9) rrNum = 8;
      }
    }
    
    if(ecgSent)
    {
      saveEcgSignal(x);
    }
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

static void saveEcgSignal(int16 ecg)
{
  if(ecgNoti.len == 0)
  {
    *pEcgByte++ = pckNum;
    pckNum = (pckNum == 255) ? 0 : pckNum+1;
    ecgNoti.len++;
  }
  if(ecgNoti.len < 1+ECG_DATA_NUM_PER_PACK*2)
  {
    *pEcgByte++ = LO_UINT16(ecg);  
    *pEcgByte++ = HI_UINT16(ecg);
    ecgNoti.len += 2;
  }
}

static uint16 median(uint16 *array, uint8 datnum)
{
  uint8 i, j;
  uint8 half = ((datnum+1)>>1);
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

static void processTestSignal(int16 x, uint8 status)
{
  static int16 data1mV[125] = {0};
  static uint8 index = 0;
  uint8 i,j;
  
  data1mV[index++] = x;
  
  if(index >= 125)
  {
    uint16 tmp; 
    for(i = 0; i < 125; ++i)
    {
      for(j = i+1; j < 125; j++)
      {
        if(data1mV[j] < data1mV[i])
        {
          tmp = data1mV[i];
          data1mV[i] = data1mV[j];
          data1mV[j] = tmp;
        }
      }
    }
    long smin = 0;
    long smax = 0;
    for(i = 25; i < 35; i++)
    {
      smin += data1mV[i];
    }
    for(i = 90; i < 100; i++)
    {
      smax += data1mV[i];
    }
    caliValue = (smax-smin)/20;
    index = 0;
  }
}