/**************************************************************************************************
* CMTechHRMonitor.c: Ӧ����Դ�ļ�
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

#include "App_HRFunc.h"

/*********************************************************************
 * ����
*/
#define INVALID_CONNHANDLE                    0xFFFF

// ֹͣ����״̬
#define STATUS_STOP           0     

// ��ʼ����״̬
#define STATUS_START          1   


/*********************************************************************
 * �ֲ�����
*/
// ����ID
static uint8 HRM_TaskID;   

// ���Ӿ��
uint16 gapConnHandle = INVALID_CONNHANDLE;

// GAP״̬
static gaprole_States_t gapProfileState = GAPROLE_INIT;

// �������
static uint8 advertData[] = 
{ 
  // Flags; this sets the device to use limited discoverable
  // mode (advertises for 30 seconds at a time) instead of general
  // discoverable mode (advertises indefinitely)
  0x02,   // length of this data
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  // service UUID
  0x03,   // length of this data
  GAP_ADTYPE_16BIT_MORE,
  LO_UINT16( HRM_SERV_UUID ),
  HI_UINT16( HRM_SERV_UUID ),

};

// ɨ����Ӧ����
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

// GGS �豸��
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN] = "CM Heart Rate Monitor";

// ��ǰ����״̬
static uint8 status = STATUS_STOP;

// ����ʱ��������λ�����룬Ĭ��2��
static uint16 interval = 2000;

// Heart rate measurement value stored in this structure
static attHandleValueNoti_t heartRateMeas;

static void HRMProcessOSALMsg( osal_event_hdr_t *pMsg ); // OSAL��Ϣ������
static void HRMGapStateCB( gaprole_States_t newState ); // GAP״̬�ı�ص�����
static void HRMServiceCB( uint8 event ); // ���ʷ���ص�����
static void HRMStart( void ); // ��������
static void HRMStop( void ); // ֹͣ����
static void HRMNotify(); // notify����
static void HRMInitIOPin(); // ��ʼ��IO�ܽ�

// GAP Role �ص��ṹ��
static gapRolesCBs_t HRM_GapStateCBs =
{
  HRMGapStateCB,         // Profile State Change Callbacks
  NULL                   // When a valid RSSI is read from controller (not used by application)
};

// �����󶨻ص�
static gapBondCBs_t HRM_BondCBs =
{
  NULL,                   // Passcode callback
  NULL                    // Pairing state callback
};

// ���ʷ���ص��ṹ��
static HRMServiceCBs_t HRM_ServCBs =
{
  HRMServiceCB    // ���ʷ���ص�����
};

extern void HRM_Init( uint8 task_id )
{
  HRM_TaskID = task_id;
  
  // Setup the GAP Peripheral Role Profile
  {
    // ���ù�����ݺ�ɨ����Ӧ����
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );
    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanResponseData ), scanResponseData );
    
    // ���ù��ʱ����
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_MIN, 0 ); // ��ͣ�ع㲥
    
    // �����Ƿ�ʹ�ܹ��
    uint8 initial_advertising_enable = TRUE;
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );
    
    // ���ô����ӽ�������ʼ�������Ӳ���֮����Ҫ��ʱ��ʱ��(units of second)
    // ��Ҫ����������ʱ�����service discovery����
    GAP_SetParamValue( TGAP_CONN_PAUSE_PERIPHERAL, 2 ); 
    
    // �������Ӳ����Լ��Ƿ�����������Ӳ���
    uint16 desired_min_interval = 200;  // units of 1.25ms 
    uint16 desired_max_interval = 1600; // units of 1.25ms
    uint16 desired_slave_latency = 1;
    uint16 desired_conn_timeout = 1000; // units of 10ms
    GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &desired_min_interval );
    GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &desired_max_interval );
    GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &desired_slave_latency );
    GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );
    uint8 enable_update_request = TRUE;
    GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &enable_update_request );
  }
  
  // ����GGS�豸������ֵ
  GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName );

  // Setup the GAP Bond Manager
  // ������ԡ����ܺͰ��й���
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

  // �������ʷ���Characteristic Values
  {
    uint8 sensLoc = HRM_SENS_LOC_CHEST;
    HRM_SetParameter( HRM_SENS_LOC, sizeof ( uint8 ), &sensLoc );
  }
  
  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );         // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES ); // GATT attributes
  HRM_AddService( GATT_ALL_SERVICES ); // ��ʪ�ȷ���
  DevInfo_AddService( ); // device information service
  
  // �Ǽ���ʪ�ȼƵķ���ص�
  HRM_Register( &HRM_ServCBs );
  
  //�������ʼ��GPIO
  //��һ�����йܽţ�reset���״̬�������������
  //�ڶ������ڲ��õ�IO�����鲻���ӵ��ⲿ��·������Ϊ��������
  //���������ڻ��õ���IO����Ҫ���ݾ����ⲿ��·�������������Ч���ã���ֹ�ĵ�
  HRMInitIOPin();
  
  HRFunc_Init();
  
  HCI_EXT_ClkDivOnHaltCmd( HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT );  

  // �����豸
  osal_set_event( HRM_TaskID, HRM_START_DEVICE_EVT );
}


// ��ʼ��IO�ܽ�
static void HRMInitIOPin()
{
  // ȫ����ΪGPIO
  P0SEL = 0; 
  P1SEL = 0; 
  P2SEL = 0; 

  // ȫ����Ϊ����͵�ƽ
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

    if ( (pMsg = osal_msg_receive( HRM_TaskID )) != NULL )
    {
      HRMProcessOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & HRM_START_DEVICE_EVT )
  {    
    // Start the Device
    VOID GAPRole_StartDevice( &HRM_GapStateCBs );

    // Start Bond Manager
    VOID GAPBondMgr_Register( &HRM_BondCBs );

    return ( events ^ HRM_START_DEVICE_EVT );
  }
  
  if ( events & HRM_START_PERIODIC_EVT )
  {
    HRMNotify();

    if(status == STATUS_START)
      osal_start_timerEx( HRM_TaskID, HRM_START_PERIODIC_EVT, ((uint32)interval) );

    return (events ^ HRM_START_PERIODIC_EVT);
  }
  
  // Discard unknown events
  return 0;
}

static void HRMProcessOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    default:
      // do nothing
      break;
  }
}

static void HRMGapStateCB( gaprole_States_t newState )
{
  // ������
  if( newState == GAPROLE_CONNECTED)
  {
    // Get connection handle
    GAPRole_GetParameter( GAPROLE_CONNHANDLE, &gapConnHandle );
  }
  // �Ͽ�����
  else if(gapProfileState == GAPROLE_CONNECTED && 
            newState != GAPROLE_CONNECTED)
  {
    HRMStop();
    HRMInitIOPin();
    HRFunc_Init();
    HRM_HandleConnStatusCB( gapConnHandle, LINKDB_STATUS_UPDATE_REMOVED );
  }
  
  gapProfileState = newState;

}

static void HRMServiceCB( uint8 event )
{
  switch (event)
  {
    case HRM_MEAS_NOTI_ENABLED:
      HRMStart();
      break;
        
    case HRM_MEAS_NOTI_DISABLED:
      HRMStop();
      break;

    case HRM_CTRL_PT_SET:
      
      break;
      
    default:
      // Should not get here
      break;
  }
}

// ��������
static void HRMStart( void )
{  
  if(status == STATUS_STOP) {
    status = STATUS_START;
    HRFunc_Start();
    osal_start_timerEx( HRM_TaskID, HRM_START_PERIODIC_EVT, interval);
  }
}

// ֹͣ����
static void HRMStop( void )
{  
  status = STATUS_STOP;
  HRFunc_Stop();
  osal_stop_timerEx( HRM_TaskID, HRM_START_PERIODIC_EVT ); 
}

// ��������
static void HRMNotify()
{
  uint8 *p = heartRateMeas.value;
  uint8 flags = 0x00;
  
  // build heart rate measurement structure from simulated values
  *p++ = flags;
  *p++ = HRFunc_CalBPM();
  
  heartRateMeas.len = (uint8) (p - heartRateMeas.value);
  HRM_MeasNotify( gapConnHandle, &heartRateMeas );
}

/*********************************************************************
*********************************************************************/
