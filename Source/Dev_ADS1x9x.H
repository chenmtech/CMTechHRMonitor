/*
 * Dev_ADS1x9x.h : ADS1x9x chip driver header file
 * the main functions including:
 * 1¡¢read and write registers£»2¡¢control the ADS1x9x; 3¡¢read data
*/

#ifndef DEV_ADS1X9X_H
#define DEV_ADS1X9X_H

#include "hal_spi_ADS.h"
#include <bcomdef.h>

//SPI Command
#define WAKEUP		0x02		//Wake-up from standby mode
#define STANDBY	        0x04	        //Enter standby mode
#define RESET		0x06		//Reset the device
#define START		0x08		//Start/restart (synchronize) conversions
#define STOP		0x0A		//Stop conversion
#define OFFSETCAL       0x1A            //Channel offset calibration

// Data Read Commands
#define RDATAC		0x10		//Enable Read Data Continuous mode.
                                        //This mode is the default mode at power-up.
#define SDATAC		0x11		//Stop Read Data Continuously mode
#define RDATA		0x12		//Read data by command; supports multiple read back.

// Register Read Commands
#define RREG		0x20		//Read n nnnn registers starting at address r rrrr
                                        //first byte 001r rrrr (2xh)(2) - second byte 000n nnnn(2)
#define WREG		0x40		//Write n nnnn registers starting at address r rrrr
                                        //first byte 010r rrrr (2xh)(2) - second byte 000n nnnn(2)

//Device Settings(Read-Only Registers)
#define ADS1x9x_REG_DEVID               (0x0000u)

//Global Settings Across Channels
#define ADS1x9x_REG_CONFIG1             (0x0001u)
#define ADS1x9x_REG_CONFIG2             (0x0002u)
#define ADS1x9x_REG_LOFF                (0x0003u)

//Channel-Specific Settings
#define ADS1x9x_REG_CH1SET              (0x0004u)
#define ADS1x9x_REG_CH2SET              (0x0005u)
#define ADS1x9x_REG_RLD_SENS            (0x0006u)
#define ADS1x9x_REG_LOFF_SENS           (0x0007u)
#define ADS1x9x_REG_LOFF_STAT           (0x0008u)

//GPIO and Other Registers
#define ADS1x9x_REG_RESP1               (0x0009u)
#define ADS1x9x_REG_RESP2               (0x000Au)
#define ADS1x9x_REG_GPIO                (0x000Bu)

typedef void (*ADS_DataCB_t)(int16 data); // callback function to handle one sample data


extern void ADS1x9x_Init(ADS_DataCB_t pfnADS_DataCB_t); // init
extern void ADS1x9x_PowerDown(); // power down
extern void ADS1x9x_WakeUp(void); // wakeup
extern void ADS1x9x_StandBy(void); // standby
extern void ADS1x9x_PowerUp(void); // power up
extern void ADS1x9x_StartConvert(void); // start convert
extern void ADS1x9x_StopConvert(void); // stop convert
extern uint8 ADS1x9x_ReadRegister(uint8 address); // read one register
extern void ADS1x9x_ReadMultipleRegister(uint8 beginaddr, uint8 * pRegs, uint8 len); // read multi registers
extern void ADS1x9x_WriteRegister(uint8 address, uint8 onebyte); // write one register
extern void ADS1x9x_WriteMultipleRegister(uint8 beginaddr, const uint8 * pRegs, uint8 len); // write multi registers
extern void ADS1x9x_WriteAllRegister(const uint8 * pRegs); // write all registers

#endif
