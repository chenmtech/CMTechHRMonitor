/*
 * hal_spi_ADS.h : CC2541��֧��ADS129XоƬ��SPI�ӿڳ���ͷ�ļ�
 * Written by Chenm
 */

#ifndef SPI_ADS_H
#define SPI_ADS_H

#include <iocc2541.h>
#include <bcomdef.h>


#define ADS_DUMMY_CHAR 0x00    //���ַ������ڶ�������ʱ���͵��޹��ֽ�
                               //ע����ЩоƬҪ���ڶ�����ʱ���͵͵�ƽ�źţ���ЩҪ���͸ߵ�ƽ�ź�
#define SPI_SEND(x) U1DBUF = x // ����x
#define SPITXDONE  (U1TX_BYTE == 1)  //SPI 1�������
// �����ܽ� : P1_0
#define ADS_START_LOW()   P1 &= ~(1<<0) // START�͵�ƽ
#define ADS_START_HIGH()  P1 |= (1<<0) // START�ߵ�ƽ
// ���ùܽ� : P1_1
#define ADS_RST_LOW() P1 &= ~(1<<1) // RESET�͵�ƽ
#define ADS_RST_HIGH()  P1 |= (1<<1) // RESET�ߵ�ƽ
// ƬѡCS�ܽ� : P1_2
#define ADS_CS_LOW()  P1 &= ~(1<<2)   // ƬѡCS�͵�ƽ                 
#define ADS_CS_HIGH()   P1 |= (1<<2) // ƬѡCS�ߵ�ƽ


extern void SPI_ADS_Init(); //SPI��ʼ��
extern uint8 SPI_ADS_SendByte(const uint8 data); //���͵����ֽ�
extern uint8 SPI_ADS_ReadByte(); // �������ֽڣ�ֻ�Ƿ���һ��ADS_DUMMY_CHAR
extern void SPI_ADS_SendFrame(const uint8* pBuffer, uint16 size); //���Ͷ���ֽ�
extern void SPI_ADS_ReadFrame(uint8* pBuffer, uint16 size); //��ȡ����ֽ�

#endif

