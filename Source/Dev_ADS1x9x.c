
#include "Dev_ADS1x9x.H"
#include "hal_mcu.h"
#include "CMUtil.h"
    
// all registers for outputing the test signal
const static uint8 test1mVRegs[12] = {  
  0x52,
  //CONFIG1
#if (SAMPLERATE == 125)
  0x00,                     //contineus sample,125sps
#elif (SAMPLERATE == 250)
  0x01,                     //contineus sample,250sps
#elif (SAMPLERATE == 500)
  0x02,                     //contineus sample,500sps  
#endif
  //CONFIG2
  0xA3,                     //0xA3: 1mV square test signal, 0xA2: 1mV DC test signal
  //LOFF
  0x10,                     //
  //CH1SET 
  0x65,                     //PGA=12, and test signal
  //CH2SET
  0x80,                     //close CH2
  //RLD_SENS (default)      
  0x23,                     //default
  //LOFF_SENS (default)
  0x00,                     //default
  //LOFF_STAT
  0x00,                     //default
  //RESP1
  0x02,                     //
  //RESP2
  0x07,                     //
  //GPIO
  0x0C                      //
};	

// all registers for outputing the normal ECG signal
const static uint8 normalECGRegs[12] = {  
  //DEVID
  0x52,
#if (SAMPLERATE == 125)
  0x00,                     //contineus sample,125sps
#elif (SAMPLERATE == 250)
  0x01,                     //contineus sample,250sps
#elif (SAMPLERATE == 500)
  0x02,                     //contineus sample,500sps  
#endif
  //CONFIG2
  0xE0,                     //
  //LOFF
  0x10,                     //
  //CH1SET 
  0x60,                     //PGA=12，and ECG input
  //CH2SET
  0x80,                     //close CH2
  //RLD_SENS     
  0x23,                     //
  //LOFF_SENS (default)
  0x03,                     //enable channel 1 lead-off detect
  //LOFF_STAT
  0x00,                     //default
  //RESP1
  0x02,                     //
  //RESP2
  0x07,
  //GPIO
  0x0C                      //
};

static ADS_DataCB_t pfnADSDataCB; // callback function processing data 
static uint8 data[2];
static int16 * pEcg = (int16*)data;

static void execute(uint8 cmd); // execute command
static void setRegsAsTestSignal(); //set registers as outputing test signal
static void setRegsAsNormalECGSignal(); // set registers as outputing normal ECG signal
static void readOneSampleUsingADS1291(void); // read one data with ADS1291
static void readOneSampleUsingADS1191(void); // read one data with ADS1191

// ADS init
extern void ADS1x9x_Init(ADS_DataCB_t pfnADS_DataCB_t)
{
  // init ADS1x9x chip
  SPI_ADS_Init();  
  
  ADS1x9x_Reset();
  
  pfnADSDataCB = pfnADS_DataCB_t;
}

// wakeup
extern void ADS1x9x_WakeUp(void)
{
  execute(WAKEUP);
}

// enter in standby mode
extern void ADS1x9x_StandBy(void)
{
  execute(STANDBY);
}

// reset chip
extern void ADS1x9x_Reset(void)
{  
  ADS_RST_LOW();     //PWDN/RESET 低电平
  delayus(50);
  ADS_RST_HIGH();    //PWDN/RESET 高电平
  delayus(50);
  
//#if defined(CALIBRATE_1MV) 
//  setRegsAsTestSignal();
//#else
//  setRegsAsNormalECGSignal();
//#endif  
  setRegsAsNormalECGSignal();
}

// start continuous sampling
extern void ADS1x9x_StartConvert(void)
{
  //设置连续采样模式
  ADS_CS_LOW();  
  delayus(100);
  SPI_ADS_SendByte(SDATAC);
  delayus(100);
  SPI_ADS_SendByte(RDATAC);  
  delayus(100);
  ADS_CS_HIGH();  
  
  delayus(100);   
  
  //START 高电平
  ADS_START_HIGH();    
  delayus(32000); 
}

// stop continuous sampling
extern void ADS1x9x_StopConvert(void)
{
  ADS_CS_LOW();  
  delayus(100);
  SPI_ADS_SendByte(SDATAC);
  delayus(100);
  ADS_CS_HIGH(); 

  delayus(100);  
  
  //START 低电平
  ADS_START_LOW();
  delayus(160); 
}

// read len registers into pRegs from the address beginaddr
extern void ADS1x9x_ReadMultipleRegister(uint8 beginaddr, uint8 * pRegs, uint8 len)
{
  ADS_CS_LOW();
  delayus(100);
  SPI_ADS_SendByte(SDATAC);
  delayus(100);
  SPI_ADS_SendByte(beginaddr | 0x20);               //发送RREG命令
  SPI_ADS_SendByte(len-1);                          //长度-1
  
  for(uint8 i = 0; i < len; i++)
    *(pRegs+i) = SPI_ADS_SendByte(ADS_DUMMY_CHAR);

  delayus(100);
  ADS_CS_HIGH();
}

// read a register with address
extern uint8 ADS1x9x_ReadRegister(uint8 address)
{
  uint8 result = 0;

  ADS_CS_LOW();
  delayus(100);
  SPI_ADS_SendByte(SDATAC);  
  delayus(100);
  SPI_ADS_SendByte(address | 0x20);         //发送RREG命令
  SPI_ADS_SendByte(0);                      //长度为1
  result = SPI_ADS_SendByte(ADS_DUMMY_CHAR);   //读寄存器
  
  delayus(100);
  ADS_CS_HIGH();
  return result;
}

// write all 12 registers
extern void ADS1x9x_WriteAllRegister(const uint8 * pRegs)
{
  ADS1x9x_WriteMultipleRegister(0x00, pRegs, 12);
}


// write multiple registers
extern void ADS1x9x_WriteMultipleRegister(uint8 beginaddr, const uint8 * pRegs, uint8 len)
{
  ADS_CS_LOW();
  delayus(100);
  SPI_ADS_SendByte(SDATAC);  
  delayus(100);
  SPI_ADS_SendByte(beginaddr | 0x40);
  SPI_ADS_SendByte(len-1);
  
  for(uint8 i = 0; i < len; i++)
    SPI_ADS_SendByte( *(pRegs+i) );
     
  delayus(100); 
  ADS_CS_HIGH();
} 

// write one register
extern void ADS1x9x_WriteRegister(uint8 address, uint8 onebyte)
{
  ADS_CS_LOW();
  delayus(100);
  SPI_ADS_SendByte(SDATAC);  
  delayus(100);
  SPI_ADS_SendByte(address | 0x40);
  SPI_ADS_SendByte(0);  
  SPI_ADS_SendByte(onebyte);
  
  delayus(100);
  ADS_CS_HIGH();
}  

//set registers as test mode
static void setRegsAsTestSignal()
{
  ADS1x9x_WriteAllRegister(test1mVRegs); 
}

// set registers as normal ecg mode
static void setRegsAsNormalECGSignal()
{
  ADS1x9x_WriteAllRegister(normalECGRegs);   
}

//execute command
static void execute(uint8 cmd)
{
  ADS_CS_LOW();
  delayus(100);
  // 发送停止采样命令
  SPI_ADS_SendByte(SDATAC);
  // 发送当前命令
  SPI_ADS_SendByte(cmd);
  delayus(100);
  ADS_CS_HIGH();
}

#pragma vector = P0INT_VECTOR
__interrupt void PORT0_ISR(void)
{ 
  halIntState_t intState;
  HAL_ENTER_CRITICAL_SECTION( intState );  // Hold off interrupts.
  
  if(P0IFG & 0x02)  //P0_1中断
  {
    P0IFG &= ~(1<<1);   //clear P0_1 IFG 
    P0IF = 0;   //clear P0 interrupt flag

#if defined(ADS1291)    
    readOneSampleUsingADS1291();
#elif defined(ADS1191)
    readOneSampleUsingADS1191();
#endif
  }
  
  HAL_EXIT_CRITICAL_SECTION( intState );   // Re-enable interrupts.  
}

// ADS1291: high precise(24bits) chip with only one channel
static void readOneSampleUsingADS1291(void)
{  
  ADS_CS_LOW();
  uint8 data[3]; // received channel 1 data buffer
  
  SPI_ADS_SendByte(ADS_DUMMY_CHAR);
  SPI_ADS_SendByte(ADS_DUMMY_CHAR);
  SPI_ADS_SendByte(ADS_DUMMY_CHAR);  
  
  data[2] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);   //MSB
  data[1] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);
  data[0] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);   //LSB
  
  ADS_CS_HIGH();
  
  int16 ecg = (int16)((data[1] & 0x00FF) | ((data[2] & 0x00FF) << 8));
   
  pfnADSDataCB(ecg);
}

// ADS1191: low precise(16bits) chip with only one channel
static void readOneSampleUsingADS1191(void)
{  
  ADS_CS_LOW();
  
  SPI_ADS_SendByte(ADS_DUMMY_CHAR);
  SPI_ADS_SendByte(ADS_DUMMY_CHAR);  
  
  data[1] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);   //MSB
  data[0] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);   //LSB
  //SPI_ADS_SendByte(ADS_DUMMY_CHAR);
  //SPI_ADS_SendByte(ADS_DUMMY_CHAR);
  
  ADS_CS_HIGH();
   
  pfnADSDataCB(*pEcg);
}