/**************************************************************************************************
* CMTechHRM.h : 心率监测仪应用头文件
**************************************************************************************************/

#ifndef CMTECHHRM_H
#define CMTECHHRM_H


/*********************************************************************
 * 常量
 */


#define HRM_START_DEVICE_EVT                   0x0001     // 设备启动事件
#define HRM_START_PERIODIC_EVT                 0x0002     // 周期测量启动事件
#define HRM_BATT_PERIODIC_EVT                  0x0004     // periodic battery measurement event


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void HRM_Init( uint8 task_id );

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 HRM_ProcessEvent( uint8 task_id, uint16 events );



/*********************************************************************
*********************************************************************/


#endif 
