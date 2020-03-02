/**************************************************************************************************
* CMTechHRMonitor.c: 应用主源文件
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "osal_snv.h"
#include "linkdb.h"
#include "OnBoard.h"
#include "gatt.h"
#include "hci.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "Service_DevInfo.h"
#include "Service_HRMonitor.h"
#include "service_battery.h"
#include "service_ecg.h"
#include "App_HRFunc.h"

#if defined ( PLUS_BROADCASTER )
  #include "peripheralBroadcaster.h"
#else
  #include "peripheral.h"
#endif
#include "gapbondmgr.h"
#include "CMTechHRMonitor.h"
#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif


#define INVALID_CONNHANDLE 0xFFFF
#define STATUS_MEAS_STOP 0     // heart rate measurement stopped
#define STATUS_MEAS_START 1    // heart rate measurement started
#define HR_NOTI_PERIOD 2000 // heart rate notification period, ms
#define BATT_MEAS_PERIOD 15000L // battery measurement period, ms
#define ECG_1MV_CALI_VALUE  164  // ecg 1mV calibration value

static uint8 taskID;   
static uint16 gapConnHandle = INVALID_CONNHANDLE;
static gaprole_States_t gapProfileState = GAPROLE_INIT;

// advertise data
static uint8 advertData[] = 
{ 
  0x02,   // length of this data
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  // service UUID
  0x03,   // length of this data
  GAP_ADTYPE_16BIT_MORE,
  LO_UINT16( HRM_SERV_UUID ),
  HI_UINT16( HRM_SERV_UUID ),

};

// scan response data
static uint8 scanResponseData[] =
{
  0x07,   // length of this data
  GAP_ADTYPE_LOCAL_NAME_SHORT,   
  'C',
  'M',
  '_',
  'H',
  'R',
  'M'
};

// GGS device name
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN] = "CM HR Monitor";

// the current hr measurement status
static uint8 status = STATUS_MEAS_STOP;

// Heart rate measurement value stored in this structure
static attHandleValueNoti_t hrNoti;

static void gapRoleStateCB( gaprole_States_t newState ); // GAP role state change callback
static void hrServiceCB( uint8 event ); // heart rate service callback function
static void batteryServiceCB( uint8 event ); // battery service callback function
static void ecgServiceCB( uint8 event ); // ecg service callback function

// GAP Role callback struct
static gapRolesCBs_t gapRoleStateCBs =
{
  gapRoleStateCB,         // Profile State Change Callbacks
  NULL                   // When a valid RSSI is read from controller (not used by application)
};

static gapBondCBs_t bondCBs =
{
  NULL,                   // Passcode callback
  NULL                    // Pairing state callback
};

static HRMServiceCBs_t hrServCBs =
{
  hrServiceCB   
};

static batteryServiceCBs_t batteryServCBs =
{
  batteryServiceCB    
};

static ECGServiceCBs_t ecgServCBs =
{
  ecgServiceCB    
};

static void processOSALMsg( osal_event_hdr_t *pMsg ); // OSAL message process function
static void initIOPin(); // initialize IO pins
static void startHRMeas( void ); // start the heart rate measurement
static void stopHRMeas( void ); // stop the heart rate measurement
static void notifyHR(); // notify heart rate

extern void HRM_Init( uint8 task_id )
{ 
  taskID = task_id;
  
  // Setup the GAP Peripheral Role Profile
  {
    // set the advertising data and scan response data
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );
    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanResponseData ), scanResponseData );
    
    // set the advertising parameters
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_MIN, 0 ); // advertising forever
    
    // enable advertising
    uint8 advertising = TRUE;
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &advertising );
    
    // set the pause time from the connection and the update of the connection parameters
    // during the time, client can finish the tasks e.g. service discovery 
    // the unit of time is second
    GAP_SetParamValue( TGAP_CONN_PAUSE_PERIPHERAL, 2 ); 
    
    // set the connection parameter
    uint16 desired_min_interval = 200;  // units of 1.25ms 
    uint16 desired_max_interval = 720; // units of 1.25ms, Note: the ios device require the interval including the latency must be less than 2s
    uint16 desired_slave_latency = 1;
    uint16 desired_conn_timeout = 600; // units of 10ms, Note: the ios device require the timeout <= 6s
    GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &desired_min_interval );
    GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &desired_max_interval );
    GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &desired_slave_latency );
    GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );
    uint8 enable_update_request = TRUE;
    GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &enable_update_request );
  }
  
  // set GGS device name
  GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName );

  // Setup the GAP Bond Manager
  {
    uint32 passkey = 0; // passkey "000000"
    uint8 pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8 mitm = TRUE;
    uint8 ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
    uint8 bonding = TRUE;
    GAPBondMgr_SetParameter( GAPBOND_DEFAULT_PASSCODE, sizeof ( uint32 ), &passkey );
    GAPBondMgr_SetParameter( GAPBOND_PAIRING_MODE, sizeof ( uint8 ), &pairMode );
    GAPBondMgr_SetParameter( GAPBOND_MITM_PROTECTION, sizeof ( uint8 ), &mitm );
    GAPBondMgr_SetParameter( GAPBOND_IO_CAPABILITIES, sizeof ( uint8 ), &ioCap );
    GAPBondMgr_SetParameter( GAPBOND_BONDING_ENABLED, sizeof ( uint8 ), &bonding );
  }  

  // set characteristic in heart rate service
  {
    uint8 sensLoc = HRM_SENS_LOC_CHEST;
    HRM_SetParameter( HRM_SENS_LOC, sizeof ( uint8 ), &sensLoc );
  }
  
  // set characteristic in ecg service
  {
    uint8 ecg1mVCali = ECG_1MV_CALI_VALUE;
    ECG_SetParameter( ECG_1MV_CALI, sizeof ( uint16 ), &ecg1mVCali );
  }  
  
  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );         // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES ); // GATT attributes
  DevInfo_AddService( ); // device information service
  HRM_AddService( GATT_ALL_SERVICES ); // heart rate monitor service
  Battery_AddService(GATT_ALL_SERVICES); // battery service
  ECG_AddService(GATT_ALL_SERVICES); // ecg service
  
  // register heart rate service callback
  HRM_Register( &hrServCBs );
  
  // register battery service callback
  Battery_RegisterAppCBs(&batteryServCBs);
  
  // register ecg service callback
  ECG_Register( &ecgServCBs );  
  
  //在这里初始化GPIO
  //第一：所有管脚，reset后的状态都是输入加上拉
  //第二：对于不用的IO，建议不连接到外部电路，且设为输入上拉
  //第三：对于会用到的IO，就要根据具体外部电路连接情况进行有效设置，防止耗电
  initIOPin();
  
  HRFunc_Init();
  
  HCI_EXT_ClkDivOnHaltCmd( HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT );  

  // 启动设备
  osal_set_event( taskID, HRM_START_DEVICE_EVT );
}


// 初始化IO管脚
static void initIOPin()
{
  // 全部设为GPIO
  P0SEL = 0; 
  P1SEL = 0; 
  P2SEL = 0; 

  // 全部设为输出低电平
  P0DIR = 0xFF; 
  P1DIR = 0xFF; 
  P2DIR = 0x1F; 

  P0 = 0; 
  P1 = 0;   
  P2 = 0; 
}

extern uint16 HRM_ProcessEvent( uint8 task_id, uint16 events )
{
  VOID task_id; // OSAL required parameter that isn't used in this function

  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( taskID )) != NULL )
    {
      processOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & HRM_START_DEVICE_EVT )
  {    
    // Start the Device
    VOID GAPRole_StartDevice( &gapRoleStateCBs );

    // Start Bond Manager
    VOID GAPBondMgr_Register( &bondCBs );

    return ( events ^ HRM_START_DEVICE_EVT );
  }
  
  if ( events & HRM_MEAS_PERIODIC_EVT )
  {
    if(gapProfileState == GAPROLE_CONNECTED && status == STATUS_MEAS_START)
    {
      notifyHR();
      osal_start_timerEx( taskID, HRM_MEAS_PERIODIC_EVT, HR_NOTI_PERIOD );
    }      

    return (events ^ HRM_MEAS_PERIODIC_EVT);
  }
  
  if ( events & HRM_BATT_PERIODIC_EVT )
  {
    if (gapProfileState == GAPROLE_CONNECTED)
    {
      // perform battery level check and send notification
      Battery_MeasLevel(gapConnHandle);
      
      // Restart timer
      osal_start_timerEx( taskID, HRM_BATT_PERIODIC_EVT, BATT_MEAS_PERIOD );
    }

    return (events ^ HRM_BATT_PERIODIC_EVT);
  }
  
  // Discard unknown events
  return 0;
}

static void processOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    default:
      // do nothing
      break;
  }
}

static void gapRoleStateCB( gaprole_States_t newState )
{
  // connected
  if( newState == GAPROLE_CONNECTED)
  {
    // Get connection handle
    GAPRole_GetParameter( GAPROLE_CONNHANDLE, &gapConnHandle );
  }
  // disconnected
  else if(gapProfileState == GAPROLE_CONNECTED && 
            newState != GAPROLE_CONNECTED)
  {
    stopHRMeas();
    //initIOPin();
    HRFunc_Init();
    osal_stop_timerEx( taskID, HRM_BATT_PERIODIC_EVT );
    
    // enable advertising
    uint8 advertising = TRUE;
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &advertising );
  }
  // if started
  else if (newState == GAPROLE_STARTED)
  {
    // Set the system ID from the bd addr
    uint8 systemId[DEVINFO_SYSTEM_ID_LEN];
    GAPRole_GetParameter(GAPROLE_BD_ADDR, systemId);
    
    // shift three bytes up
    systemId[7] = systemId[5];
    systemId[6] = systemId[4];
    systemId[5] = systemId[3];
    
    // set middle bytes to zero
    systemId[4] = 0;
    systemId[3] = 0;
    
    DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);
  }
  
  gapProfileState = newState;

}

static void hrServiceCB( uint8 event )
{
  switch (event)
  {
    case HRM_MEAS_NOTI_ENABLED:
      startHRMeas();  
      break;
        
    case HRM_MEAS_NOTI_DISABLED:
      stopHRMeas();
      break;

    case HRM_CTRL_PT_SET:
      
      break;
      
    default:
      // Should not get here
      break;
  }
}

// start measuring heart rate
static void startHRMeas( void )
{  
  if(status == STATUS_MEAS_STOP) {
    status = STATUS_MEAS_START;
    HRFunc_Start();
    osal_start_timerEx( taskID, HRM_MEAS_PERIODIC_EVT, HR_NOTI_PERIOD);
  }
}

// stop measuring heart rate
static void stopHRMeas( void )
{  
  status = STATUS_MEAS_STOP;
  HRFunc_Stop();
  osal_stop_timerEx( taskID, HRM_MEAS_PERIODIC_EVT ); 
}

// send heart rate notification
static void notifyHR()
{
  uint8 *p = hrNoti.value;
  uint8 len = HRFunc_CopyHRDataInto(p);
  if(len == 0) return;  
  hrNoti.len = len;
  HRM_MeasNotify( gapConnHandle, &hrNoti );
}

static void batteryServiceCB( uint8 event )
{
  if (event == BATTERY_LEVEL_NOTI_ENABLED)
  {
    // if connected start periodic measurement
    if (gapProfileState == GAPROLE_CONNECTED)
    {
      osal_start_timerEx( taskID, HRM_BATT_PERIODIC_EVT, BATT_MEAS_PERIOD );
    } 
  }
  else if (event == BATTERY_LEVEL_NOTI_DISABLED)
  {
    // stop periodic measurement
    osal_stop_timerEx( taskID, HRM_BATT_PERIODIC_EVT );
  }
}

static void ecgServiceCB( uint8 event )
{
  switch (event)
  {
    case ECG_MEAS_NOTI_ENABLED:
      startHRMeas();  
      break;
        
    case ECG_MEAS_NOTI_DISABLED:
      stopHRMeas();
      break;
      
    default:
      // Should not get here
      break;
  }
}