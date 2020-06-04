
#include "hal_spi_ADS.h"

static void setADSCtrlPin(); //set ctrl pins for the ADS chip, e.g. DRDY, START, CS, PWDN
static void setADSSpiPin();  //set SPI pin for the ADS chip. here use SPI 1, alt.2，that is：MI:P17, MO:P16, SCLK:P15

//ADS chip initialization
extern void SPI_ADS_Init()
{
  setADSCtrlPin();
  setADSSpiPin();
}

// send one byte data, return the sent byte
extern uint8 SPI_ADS_SendByte(const uint8 data)
{
  SPI_SEND(data); 
  while (!U1TX_BYTE);
  U1TX_BYTE = 0;
  return (U1DBUF);
}

// read one byte data
extern uint8 SPI_ADS_ReadByte()
{
  return SPI_ADS_SendByte(ADS_DUMMY_CHAR);
}

// send a frame data in pBuffer
extern void SPI_ADS_SendFrame(const uint8* pBuffer, uint16 size)
{
  uint16 i = 0;
  for (i = 0; i < size; i++){
    SPI_SEND(pBuffer[i]);
    while (!SPITXDONE);
    U1TX_BYTE = 0;
  }  
  return;
}

// read a frame data to pBuffer
extern void SPI_ADS_ReadFrame(uint8* pBuffer, uint16 size)
{
  uint16 i = 0;
  for (i = 0; i < size; i++){
    SPI_SEND(ADS_DUMMY_CHAR);
    while (!SPITXDONE);
    U1TX_BYTE = 0;
    pBuffer[i] = U1DBUF;
  } 
  return; 
}

//
static void setADSCtrlPin()
{
  //P0.1 DRDY管脚配置  
  //先关P0.1即DRDY中断
  P0IEN &= ~(1<<1);
  P0IFG &= ~(1<<1);   // clear P0_1 interrupt status flag
  P0IF = 0;   //clear P0 interrupt flag  
  
  //配置P0.1即DRDY 中断
  P0SEL &= ~(1<<1); //GPIO
  P0DIR &= ~(1<<1); //Input
  PICTL |= (1<<0);  //下降沿触发
  //////////////////////////
  
  //P1.0 START管脚配置
  P1SEL &= ~(1<<0); //GPIO
  P1 &= ~(1<<0);    //初始为低电平
  P1DIR |= (1<<0);  //Output
  
  //P1.1 ~PWDN/RESET管脚配置
  P1SEL &= ~(1<<1); //GPIO
  P1 |= (1<<1);    //初始为高电平
  P1DIR |= (1<<1);  //Output

  //P1.2 ~CS管脚配置  
  P1SEL &= ~(1<<2); //GPIO
  P1 |= (1<<2);     //初始为高电平
  P1DIR |= (1<<2);  //Output
  
  //开P0.1 DRDY中断
  P0IEN |= (1<<1);    // P0_1 interrupt enable
  P0IE = 1;           // P0 interrupt enable
}

//
static void setADSSpiPin()
{    
  //SPI 1 管脚配置
  P1SEL |= ((1<<5)|(1<<6)|(1<<7)); //P1.5 SCLK、P1.6 MOSI和P1.7 MISO为外设
  P1DIR |= ((1<<5)|(1<<6)); //P1.5 SCLK, P1.6 MOSI Output
  P1DIR &= ~(1<<7); //P1.7 MISO Input  
  
  PERCFG |= (1<<1); //SPI 1 使用 Alt.2
  P2SEL |= (1<<6);  //USART 1 has priority than USART 0
  P2SEL &= ~(1<<5); //USART 1 has priority than TIMER 3
  
  URX1IE = 0; //disable SPI 1 RX interrupt
  URX1IF = 0;
  
  IEN2 &= ~(1<<3);  //disable SPI 1 TX interrupt
  UTX1IF = 0;  

  U1CSR = 0x00; //SPI 1 master mode
  
  // SPI时钟频率为512KHz
  //U1GCR = 0x6E; // CPOL = 0, CPHA = 1, order = MSB, BAUD_E = 14
  //U1BAUD = 0;   //BAUD_M = 0, SCLK = 512KHz
  
  // SPI时钟频率为1MHz
  U1GCR = 0x6F; // CPOL = 0, CPHA = 1, order = MSB, BAUD_E = 15
  U1BAUD = 0x00;   //BAUD_M = 0, SCLK = 1MHz  
  
  //U1DBUF = 0x00;
}


