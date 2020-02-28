/*
 * hal_spi_ADS.h : SPI interface for supportting the ADS1x9x chips in the CC2541
 * Written by Chenm
 */

#ifndef SPI_ADS_H
#define SPI_ADS_H

#include <iocc2541.h>
#include <bcomdef.h>


#define ADS_DUMMY_CHAR 0x00    //dummy char sent for reading out the data
                               //注意有些芯片要求在读数据时发送低电平信号，有些要求发送高电平信号
#define SPI_SEND(x) U1DBUF = x // send x
#define SPITXDONE  (U1TX_BYTE == 1)  //if finishing send by SPI 1
// START pin : P1_0
#define ADS_START_LOW()   P1 &= ~(1<<0) // set START pin low
#define ADS_START_HIGH()  P1 |= (1<<0)  // set START pin high
// RESET pin : P1_1
#define ADS_RST_LOW() P1 &= ~(1<<1) // set RESET pin low
#define ADS_RST_HIGH()  P1 |= (1<<1) // set RESET pin high
// CS pin : P1_2
#define ADS_CS_LOW()  P1 &= ~(1<<2)   // set CS pin low                 
#define ADS_CS_HIGH()   P1 |= (1<<2) // set CS pin high


extern void SPI_ADS_Init(); //SPI init for ADS
extern uint8 SPI_ADS_SendByte(const uint8 data); //send one byte
extern uint8 SPI_ADS_ReadByte(); // read one byte by sending a ADS_DUMMY_CHAR
extern void SPI_ADS_SendFrame(const uint8* pBuffer, uint16 size); //send multibytes
extern void SPI_ADS_ReadFrame(uint8* pBuffer, uint16 size); //read multibytes

#endif

