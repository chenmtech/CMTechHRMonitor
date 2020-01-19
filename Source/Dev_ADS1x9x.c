
#include "Dev_ADS1x9x.H"
#include "hal_mcu.h"
#include "CMUtil.h"

// ADSоƬ����
#define TYPE_ADS1191    0
#define TYPE_ADS1192    1
#define TYPE_ADS1291    2
#define TYPE_ADS1292    3

// ADS1291ÿ�ν������ݳ���Ϊ6���ֽڣ�״̬3�ֽڣ�ͨ��1Ϊ3�ֽڣ�ͨ��2û�����
#define DATA_LEN  6               


/*ʹ���ڲ������ź�����*/
const static uint8 test1mVRegs[12] = {  
  0x52,
  //CONFIG1
#if (SR_SPS == 125)
  0x00,                     //contineus sample,125sps
#elif (SR_SPS == 250)
  0x01,                     //contineus sample,250sps
#endif
  //CONFIG2
  0xA3,                     //0xA3: 1mV ���������ź�, 0xA2: 1mV DC�����ź�
  //LOFF
  0x10,                     //
  //CH1SET 
  0x65,                     //PGA=12, �����ź�
  //CH2SET
  0x80,                     //�ر�CH2
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

/*�ɼ������ĵ��ź�����*/
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
  0x60,                     //PGA=12���缫����
  //CH2SET
  0x80,                     //�ر�CH2
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

static uint8 defaultRegs[12]; // �������������ȱʡ�Ĵ���ֵ
static uint8 type; // оƬ����
static ADS_DataCB_t ADS_DataCB; // ������Ļص�����
static uint8 status[3] = {0}; // ���յ���3�ֽ�״̬
static uint8 data[4]; //��ȡ��ͨ�������ֽ�


static void execute(uint8 cmd); // ִ������
static void ADS1291_ReadOneSample(void); // ��һ������ֵ

/******************************************************************************
//ִ��һ������
******************************************************************************/
static void execute(uint8 cmd)
{
  ADS_CS_LOW();
  
  delayus(100);
  
  // ����ֹͣ��������
  SPI_ADS_SendByte(SDATAC);
  
  // ���͵�ǰ����
  SPI_ADS_SendByte(cmd);
  
  delayus(100);
  
  ADS_CS_HIGH();
}

/******************************************************************************
 * ��һ������
 * ADS1291�Ǹ߾��ȣ�24bit����ͨ��оƬ
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

// ADS ��ʼ��
extern void ADS1x9x_Init(ADS_DataCB_t pfnADS_DataCB_t)
{
  // ��ʼ��ADSģ��
  SPI_ADS_Init();  
  
  ADS1x9x_Reset();
  
  // ��ȱʡ�Ĵ���
  ADS1x9x_ReadAllRegister(defaultRegs); 
  
  type = (defaultRegs[0] & 0x03);
  
  // ���������ɼ��Ĵ���ֵ
  ADS1x9x_SetRegsAsNormalECGSignal();
  // ���òɼ��ڲ������ź�ʱ�ļĴ���ֵ
  //ADS1x9x_SetRegsAsTestSignal();
  
  //���ò������ݺ�Ļص�����
  ADS_DataCB = pfnADS_DataCB_t;
  
  // �������״̬
  ADS1x9x_StandBy();
}

// ����
extern void ADS1x9x_WakeUp(void)
{
  execute(WAKEUP);
}

// ����
extern void ADS1x9x_StandBy(void)
{
  execute(STANDBY);
}


extern void ADS1x9x_Reset(void)
{
  ADS_RST_LOW();     //PWDN/RESET �͵�ƽ
  delayus(50);
  ADS_RST_HIGH();    //PWDN/RESET �ߵ�ƽ
  delayus(50);
}

// ��������
extern void ADS1x9x_StartConvert(void)
{
  //������������ģʽ
  ADS_CS_LOW();  
  delayus(100);
  SPI_ADS_SendByte(SDATAC);
  delayus(100);
  SPI_ADS_SendByte(RDATAC);  
  delayus(100);
  ADS_CS_HIGH();  
  
  delayus(100);   
  
  //START �ߵ�ƽ
  ADS_START_HIGH();    
  delayus(32000); 
}

// ֹͣ����
extern void ADS1x9x_StopConvert(void)
{
  ADS_CS_LOW();  
  delayus(100);
  SPI_ADS_SendByte(SDATAC);
  delayus(100);
  ADS_CS_HIGH(); 

  delayus(100);  
  
  //START �͵�ƽ
  ADS_START_LOW();
  delayus(160); 
}

// ������12���Ĵ���ֵ
extern void ADS1x9x_ReadAllRegister(uint8 * pRegs)
{
  ADS1x9x_ReadMultipleRegister(0x00, pRegs, 12);
}

// ������Ĵ���ֵ
extern void ADS1x9x_ReadMultipleRegister(uint8 beginaddr, uint8 * pRegs, uint8 len)
{
  ADS_CS_LOW();
  delayus(100);
  SPI_ADS_SendByte(SDATAC);
  delayus(100);
  SPI_ADS_SendByte(beginaddr | 0x20);               //����RREG����
  SPI_ADS_SendByte(len-1);                          //����-1
  
  for(uint8 i = 0; i < len; i++)
    *(pRegs+i) = SPI_ADS_SendByte(ADS_DUMMY_CHAR);

  delayus(100);
  ADS_CS_HIGH();
}

// ��һ���Ĵ���ֵ
extern uint8 ADS1x9x_ReadRegister(uint8 address)
{
  uint8 result = 0;

  ADS_CS_LOW();
  delayus(100);
  SPI_ADS_SendByte(SDATAC);  
  delayus(100);
  SPI_ADS_SendByte(address | 0x20);         //����RREG����
  SPI_ADS_SendByte(0);                      //����Ϊ1
  result = SPI_ADS_SendByte(ADS_DUMMY_CHAR);   //���Ĵ���
  
  delayus(100);
  ADS_CS_HIGH();
  return result;
}

// д����12���Ĵ���
extern void ADS1x9x_WriteAllRegister(const uint8 * pRegs)
{
  ADS1x9x_WriteMultipleRegister(0x00, pRegs, 12);
}

//ʹ���ڲ������ź�
extern void ADS1x9x_SetRegsAsTestSignal()
{
  ADS1x9x_WriteAllRegister(test1mVRegs); 
}

extern void ADS1x9x_SetRegsAsNormalECGSignal()
{
  ADS1x9x_WriteAllRegister(normalECGRegs);   
}

// д����Ĵ���ֵ
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

// дһ���Ĵ���
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
  
  if(P0IFG & 0x02)  //P0_1�ж�
  {
    P0IFG &= ~(1<<1);   //clear P0_1 IFG 
    P0IF = 0;   //clear P0 interrupt flag
    
    ADS1291_ReadOneSample();
  }
  
  HAL_EXIT_CRITICAL_SECTION( intState );   // Re-enable interrupts.  
}

/*
// ����Ϊ�ɼ�1mV�����ź�
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

// ����Ϊ�ɼ�ECG�ź�
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