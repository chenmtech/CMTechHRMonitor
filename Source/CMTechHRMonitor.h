/**************************************************************************************************
* CMTechHRM.h : Heart Rate Monitor application header file
**************************************************************************************************/

#ifndef CMTECHHRM_H
#define CMTECHHRM_H


#define HRM_START_DEVICE_EVT 0x0001      // device start event
#define HRM_HR_PERIODIC_EVT 0x0002     // periodic heart rate measurement event
#define HRM_BATT_PERIODIC_EVT 0x0004     // periodic battery measurement event
#define HRM_ECG_NOTI_EVT 0x0008 // ecg packet notification event
/*
 * Task Initialization for the BLE Application
 */
extern void HRM_Init( uint8 task_id );

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 HRM_ProcessEvent( uint8 task_id, uint16 events );


#endif 
