/*
 * App_HRFunc.h : Heart Rate application Function Model source file
 * Written by Chenm
 */

#include "App_HRFunc.h"
#include "CMUtil.h"
#include "Dev_ADS1x9x.h"
#include "QRSDET.h"
#include "Service_HRMonitor.h"
#include "service_ecg.h"
#include "cmtechhrmonitor.h"


#define ECG_PACK_BYTE_NUM 19 // byte number per ecg packet, 1+9*2
#define ECG_MAX_PACK_NUM 255 // max packet num

static uint8 taskId; // taskId of application

// is the heart rate calculated?
static bool hrCalc = false;
// the flag of the initial beat
static uint8 initBeat = 1 ;
// RR interval buffer, the max number in the buffer is 9
static uint16 rrBuf[9] = {0};
// the current number in rrBuf
static uint8 rrNum = 0;
// HR notification struct
static attHandleValueNoti_t hrNoti;

// 1mV calibration value, only used for getting the calibration value when CALIBRATE_1MV is set in preprocessing
static uint16 caliValue = 0;

// is the ecg data sent?
static bool ecgSend = false;
// the number of the current ecg data packet, from 0 to ECG_MAX_PACK_NUM
static uint8 pckNum = 0;
// ecg packet buffer
static uint8 ecgBuff[ECG_PACK_BYTE_NUM] = {0};
// pointer to the ecg buff
static uint8* pEcgBuff;
// ecg packet structure sent out
static attHandleValueNoti_t ecgNoti;

static void processEcgSignal(int16 x, uint8 status);
static void saveEcgSignal(int16 ecg);
static uint16 median(uint16 *array, uint8 datnum);
static void processTestSignal(int16 x, uint8 status);

extern void HRFunc_Init(uint8 taskID)
{ 
  taskId = taskID;
  
  // initilize the ADS1x9x and set the data process callback function
#if defined(CALIBRATE_1MV)
  ADS1x9x_Init(processTestSignal);  
#else
  ADS1x9x_Init(processEcgSignal);  
#endif
  delayus(1000);
}

extern void HRFunc_SwitchSamplingEcg(bool start)
{
  if(start)
  {
    ADS1x9x_WakeUp();
    // 这里一定要延时，否则容易死机
    delayus(1000);
    ADS1x9x_StartConvert();
    delayus(1000);
  } 
  else
  {
    ADS1x9x_StopConvert();
    ADS1x9x_StandBy();
    delayus(2000);
  }
}

extern void HRFunc_SwitchCalcingHR(bool calc)
{
  if(calc)
  {
    QRSDet(0, 1);
    rrNum = 0; 
  }
  hrCalc = calc;
}

extern void HRFunc_SwitchSendingEcg(bool send)
{
  if(send)
  {
    pckNum = 0;
    pEcgBuff = ecgBuff;
    osal_clear_event(taskId, HRM_ECG_NOTI_EVT);
  }
  ecgSend = send;
}

extern void HRFunc_SendEcgPacket(uint16 connHandle)
{
  ECG_PacketNotify( connHandle, &ecgNoti );
}

// send HR packet
extern void HRFunc_SendHRPacket(uint16 connHandle)
{
  if(rrNum == 0) return;  // No RR interval, return
  
  uint8* p = hrNoti.value;
  uint8* pTmp = p;
  
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
  
  hrNoti.len = (uint8)(p-pTmp);
  HRM_MeasNotify( connHandle, &hrNoti );
  rrNum = 0;
}

static void processEcgSignal(int16 x, uint8 status)
{
  if(!status)
  {
    if(hrCalc) // need calculate HR
    {
      if(QRSDet(x, 0))
      {
        if(initBeat) 
        {
          initBeat = 0;
        }
        else
        {
          rrBuf[rrNum++] = getRRInterval();
          if(rrNum >= 9) rrNum = 8;
        }
      }
    }
    
    if(ecgSend) // need send ecg
    {
      saveEcgSignal(x);
    }
  }
}

static void saveEcgSignal(int16 ecg)
{
  if(pEcgBuff == ecgBuff)
  {
    *pEcgBuff++ = pckNum;
    pckNum = (pckNum == ECG_MAX_PACK_NUM) ? 0 : pckNum+1;
  }
  *pEcgBuff++ = LO_UINT16(ecg);  
  *pEcgBuff++ = HI_UINT16(ecg);
  
  if(pEcgBuff-ecgBuff >= ECG_PACK_BYTE_NUM)
  {
    osal_memcpy(ecgNoti.value, ecgBuff, ECG_PACK_BYTE_NUM);
    ecgNoti.len = ECG_PACK_BYTE_NUM;
    osal_set_event(taskId, HRM_ECG_NOTI_EVT);
    pEcgBuff = ecgBuff;
  }
}

static uint16 median(uint16 *array, uint8 datnum)
{
  uint8 i, j;
  uint8 half = ((datnum-2)>>1);
  uint16 tmp, sort[9] ;
  osal_memcpy(sort, array, 2*datnum);
  // sort array using up-order
  // only half of data need to be sorted for finding out the median
  for(i = 0; i <= half; ++i)
  {
    for(j = i+1; j < datnum; j++)
    {
      if(sort[j] > sort[i])
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