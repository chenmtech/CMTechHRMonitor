/**
* ecg service header file: providing the ecg-related info and sending the ecg data packet
*/

#ifndef SERVICE_ECG_H
#define SERVICE_ECG_H

// Ecg Service Parameters
#define ECG_PACK                      0  // ecg data packet
#define ECG_PACK_CHAR_CFG             1  // 
#define ECG_1MV_CALI                  2  // 1mV calibration value
#define ECG_SAMPLE_RATE               3  // sample rate
#define ECG_LEAD_TYPE                 4  // lead type
#define ECG_WORK_MODE                 5  // work mode status

// Ecg Service UUIDs
#define ECG_SERV_UUID                 0xAA40
#define ECG_PACK_UUID                 0xAA41
#define ECG_1MV_CALI_UUID             0xAA42
#define ECG_SAMPLE_RATE_UUID          0xAA43
#define ECG_LEAD_TYPE_UUID            0xAA44
#define ECG_WORK_MODE_UUID            0xAA45

// Values for Ecg Lead Type
#define ECG_LEAD_TYPE_I            0x00
#define ECG_LEAD_TYPE_II           0x01
#define ECG_LEAD_TYPE_III          0x02

// Ecg Service bit fields
#define ECG_SERVICE                   0x00000001

// Callback events
#define ECG_PACK_NOTI_ENABLED         0 // ecg data packet notification enabled
#define ECG_PACK_NOTI_DISABLED        1 // ecg data packet notification disabled
#define ECG_WORK_MODE_CHANGED         2 // ecg work mode changed

// ecg Service callback function
typedef void (*ecgServiceCB_t)(uint8 event);

typedef struct
{
  ecgServiceCB_t    pfnEcgServiceCB;  
} ECGServiceCBs_t;


extern bStatus_t ECG_AddService( uint32 services );
extern void ECG_RegisterAppCBs( ECGServiceCBs_t* pfnServiceCBs );
extern bStatus_t ECG_SetParameter( uint8 param, uint8 len, void *value );
extern bStatus_t ECG_GetParameter( uint8 param, void *value );
extern bStatus_t ECG_PacketNotify( uint16 connHandle, attHandleValueNoti_t *pNoti );// notify the ecg data packet



#endif /* ECGSERVICE_H */
