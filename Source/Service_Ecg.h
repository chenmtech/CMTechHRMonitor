
#ifndef SERVICE_ECG_H
#define SERVICE_ECG_H

// Ecg Service Parameters
#define ECG_MEAS                      0  // ecg measurement
#define ECG_MEAS_CHAR_CFG             1  // 
#define ECG_1MV_CALI                  2  // 1mV calibration value
#define ECG_SAMPLE_RATE               3  // sample rate
#define ECG_LEAD_TYPE                 4  // lead type

// Ecg Service UUIDs
#define ECG_SERV_UUID                 0xAA40
#define ECG_MEAS_UUID                 0xAA41
#define ECG_1MV_CALI_UUID             0xAA42
#define ECG_SAMPLE_RATE_UUID          0xAA43
#define ECG_LEAD_TYPE_UUID            0xAA44

// Values for Ecg Lead Type
#define ECG_LEAD_TYPE_I            0x00
#define ECG_LEAD_TYPE_II           0x01
#define ECG_LEAD_TYPE_III          0x03

// Ecg Service bit fields
#define ECG_SERVICE                   0x00000001

// Callback events
#define ECG_MEAS_NOTI_ENABLED         1 // measurement enabled
#define ECG_MEAS_NOTI_DISABLED        2 // measurement disabled

// Ecg Service callback function
typedef void (*ECGServiceCB_t)(uint8 event);

typedef struct
{
  ECGServiceCB_t    pfnECGServiceCB;  
} ECGServiceCBs_t;


extern bStatus_t ECG_AddService( uint32 services );
extern void ECG_Register( ECGServiceCBs_t* pfnServiceCBs );
extern bStatus_t ECG_SetParameter( uint8 param, uint8 len, void *value );
extern bStatus_t ECG_GetParameter( uint8 param, void *value );
extern bStatus_t ECG_MeasNotify( uint16 connHandle, attHandleValueNoti_t *pNoti );


#endif /* ECGSERVICE_H */
