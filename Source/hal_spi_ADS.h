/*
 * hal_spi_ADS.h : CC2541上支持ADS129X芯片的SPI接口程序头文件
 * Written by Chenm
 */

#ifndef SPI_ADS_H
#define SPI_ADS_H

#include <iocc2541.h>
#include <bcomdef.h>


#define ADS_DUMMY_CHAR 0x00    //哑字符，用于读出数据时发送的无关字节
                               //注意有些芯片要求在读数据时发送低电平信号，有些要求发送高电平信号
#define SPI_SEND(x) U1DBUF = x // 发送x
#define SPITXDONE  (U1TX_BYTE == 1)  //SPI 1发送完毕
// 启动管脚 : P1_0
#define ADS_START_LOW()   P1 &= ~(1<<0) // START低电平
#define ADS_START_HIGH()  P1 |= (1<<0) // START高电平
// 重置管脚 : P1_1
#define ADS_RST_LOW() P1 &= ~(1<<1) // RESET低电平
#define ADS_RST_HIGH()  P1 |= (1<<1) // RESET高电平
// 片选CS管脚 : P1_2
#define ADS_CS_LOW()  P1 &= ~(1<<2)   // 片选CS低电平                 
#define ADS_CS_HIGH()   P1 |= (1<<2) // 片选CS高电平


extern void SPI_ADS_Init(); //SPI初始化
extern uint8 SPI_ADS_SendByte(const uint8 data); //发送单个字节
extern uint8 SPI_ADS_ReadByte(); // 读单个字节，只是发送一个ADS_DUMMY_CHAR
extern void SPI_ADS_SendFrame(const uint8* pBuffer, uint16 size); //发送多个字节
extern void SPI_ADS_ReadFrame(uint8* pBuffer, uint16 size); //读取多个字节

#endif

