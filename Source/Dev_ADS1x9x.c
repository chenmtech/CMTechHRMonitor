
#include "Dev_ADS1x9x.H"
#include "hal_mcu.h"
#include "CMUtil.h"

// ADS芯片类型
#define TYPE_ADS1191    0
#define TYPE_ADS1192    1
#define TYPE_ADS1291    2
#define TYPE_ADS1292    3

// ADS1291每次接收数据长度为6个字节：状态3字节，通道1为3字节，通道2没有输出
#define DATA_LEN  6               


/*使用内部测试信号配置*/
const static uint8 test1mVRegs[12] = {  
  0x52,
  //CONFIG1
#if (SR_SPS == 125)
  0x00,                     //contineus sample,125sps
#elif (SR_SPS == 250)
  0x01,                     //contineus sample,250sps
#endif
  //CONFIG2
  0xA3,                     //0xA3: 1mV 方波测试信号, 0xA2: 1mV DC测试信号
  //LOFF
  0x10,                     //
  //CH1SET 
  0x65,                     //PGA=12, 测试信号
  //CH2SET
  0x80,                     //关闭CH2
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

/*采集正常心电信号配置*/
const static uint8 normalECGRegs[12] = {  
  //DEVID
  0x52,
#if (SR_SPS == 125)
  0x00,                     //contineus sample,125sps
#elif (SR_SPS == 250)
  0x01,                     //contineus sample,250sps
#endif
  //CONFIG2
  0xA0,                     //
  //LOFF
  0x10,                     //
  //CH1SET 
  0x60,                     //PGA=12，电极输入
  //CH2SET
  0x80,                     //关闭CH2
  //RLD_SENS     
  0x23,                     //
  //LOFF_SENS (default)
  0x00,                     //default
  //LOFF_STAT
  0x00,                     //default
  //RESP1
  0x02,                     //
  //RESP2
  0x07,
  //GPIO
  0x0C                      //
};

static uint8 defaultRegs[12]; // 重启后读出来的缺省寄存器值
static uint8 type; // 芯片类型
static ADS_DataCB_t ADS_DataCB; // 采样后的回调函数
static uint8 status[3] = {0}; // 接收到的3字节状态
static uint8 data[4]; //读取的通道数据字节


static void execute(uint8 cmd); // 执行命令
static void ADS1291_ReadOneSample(void); // 读一个采样值

/******************************************************************************
//执行一个命令
******************************************************************************/
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

/******************************************************************************
 * 读一个样本
 * ADS1291是高精度（24bit）单通道芯片
******************************************************************************/
static void ADS1291_ReadOneSample(void)
{  
  ADS_CS_LOW();
  
  status[0] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);
  status[1] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);
  status[2] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);  
  
  data[3] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);   //MSB
  data[2] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);
  data[1] = SPI_ADS_SendByte(ADS_DUMMY_CHAR);   //LSB
  
  ADS_CS_HIGH();
  
  if(ADS_DataCB != 0)
    ADS_DataCB(data[2], data[3]);
}

// ADS 初始化
extern void ADS1x9x_Init(ADS_DataCB_t pfnADS_DataCB_t)
{
  // 初始化ADS模块
  SPI_ADS_Init();  
  
  ADS1x9x_Reset();
  
  // 读缺省寄存器
  ADS1x9x_ReadAllRegister(defaultRegs); 
  
  type = (defaultRegs[0] & 0x03);
  
  // 设置正常采集寄存器值
  ADS1x9x_SetRegsAsNormalECGSignal();
  // 设置采集内部测试信号时的寄存器值
  //ADS1x9x_SetRegsAsTestSignal();
  
  //设置采样数据后的回调函数
  ADS_DataCB = pfnADS_DataCB_t;
  
  // 进入待机状态
  ADS1x9x_StandBy();
}

// 唤醒
extern void ADS1x9x_WakeUp(void)
{
  execute(WAKEUP);
}

// 待机
extern void ADS1x9x_StandBy(void)
{
  execute(STANDBY);
}


extern void ADS1x9x_Reset(void)
{
  ADS_RST_LOW();     //PWDN/RESET 低电平
  delayus(50);
  ADS_RST_HIGH();    //PWDN/RESET 高电平
  delayus(50);
}

// 启动采样
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

// 停止采样
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

// 读所有12个寄存器值
extern void ADS1x9x_ReadAllRegister(uint8 * pRegs)
{
  ADS1x9x_ReadMultipleRegister(0x00, pRegs, 12);
}

// 读多个寄存器值
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

// 读一个寄存器值
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

// 写所有12个寄存器
extern void ADS1x9x_WriteAllRegister(const uint8 * pRegs)
{
  ADS1x9x_WriteMultipleRegister(0x00, pRegs, 12);
}

//使用内部测试信号
extern void ADS1x9x_SetRegsAsTestSignal()
{
  ADS1x9x_WriteAllRegister(test1mVRegs); 
}

extern void ADS1x9x_SetRegsAsNormalECGSignal()
{
  ADS1x9x_WriteAllRegister(normalECGRegs);   
}

// 写多个寄存器值
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

// 写一个寄存器
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

#pragma vector = P0INT_VECTOR
__interrupt void PORT0_ISR(void)
{ 
  halIntState_t intState;
  HAL_ENTER_CRITICAL_SECTION( intState );  // Hold off interrupts.
  
  if(P0IFG & 0x02)  //P0_1中断
  {
    P0IFG &= ~(1<<1);   //clear P0_1 IFG 
    P0IF = 0;   //clear P0 interrupt flag
    
    ADS1291_ReadOneSample();
  }
  
  HAL_EXIT_CRITICAL_SECTION( intState );   // Re-enable interrupts.  
}

/*
// 设置为采集1mV测试信号
extern void ADS1x9x_ChangeToTestSignal() 
{
  //ADS1x9x_WriteRegister(0x02, 0xA3);
  //ADS1x9x_WriteRegister(0x04, 0x65);
  ADS_CS_LOW();
  delayus(100);
  SPI_ADS_SendByte(SDATAC);  
  delayus(100);
  SPI_ADS_SendByte(0x02 | 0x40);
  SPI_ADS_SendByte(0);  
  SPI_ADS_SendByte(0xA3);
  SPI_ADS_SendByte(0x04 | 0x40);
  SPI_ADS_SendByte(0);  
  SPI_ADS_SendByte(0x65);
  
  delayus(100);
  ADS_CS_HIGH();
}

// 设置为采集ECG信号
extern void ADS1x9x_ChangeToEcgSignal() 
{
  //ADS1x9x_WriteRegister(0x02, 0xA0);
  //ADS1x9x_WriteRegister(0x04, 0x60);
  ADS_CS_LOW();
  delayus(100);
  SPI_ADS_SendByte(SDATAC);  
  delayus(100);
  SPI_ADS_SendByte(0x02 | 0x40);
  SPI_ADS_SendByte(0);  
  SPI_ADS_SendByte(0xA0);
  SPI_ADS_SendByte(0x04 | 0x40);
  SPI_ADS_SendByte(0);  
  SPI_ADS_SendByte(0x60);
  
  delayus(100);
  ADS_CS_HIGH();
}
*/