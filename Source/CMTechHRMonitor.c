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
 * 常量
*/
#define INVALID_CONNHANDLE                    0xFFFF

// 停止测量状态
#define STATUS_STOP           0     

// 开始测量状态
#define STATUS_START          1   


/*********************************************************************
 * 局部变量
*/
// 任务ID
static uint8 HRM_TaskID;   

// 连接句柄
uint16 gapConnHandle = INVALID_CONNHANDLE;

// GAP状态
static gaprole_States_t gapProfileState = GAPROLE_INIT;

// 广告数据
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

// 扫描响应数据
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

// GGS 设备名
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN] = "CM Heart Rate Monitor";

// 当前测量状态
static uint8 status = STATUS_STOP;

// 测量时间间隔，单位：毫秒，默认2秒
static uint16 interval = 2000;

// Heart rate measurement value stored in this structure
static attHandleValueNoti_t heartRateMeas;

static void HRMProcessOSALMsg( osal_event_hdr_t *pMsg ); // OSAL消息处理函数
static void HRMGapStateCB( gaprole_States_t newState ); // GAP状态改变回调函数
static void HRMServiceCB( uint8 event ); // 心率服务回调函数
static void HRMStart( void ); // 启动测量
static void HRMStop( void ); // 停止测量
static void HRMNotify(); // notify心率
static void HRMInitIOPin(); // 初始化IO管脚

// GAP Role 回调结构体
static gapRolesCBs_t HRM_GapStateCBs =
{
  HRMGapStateCB,         // Profile State Change Callbacks
  NULL                   // When a valid RSSI is read from controller (not used by application)
};

// 配对与绑定回调
static gapBondCBs_t HRM_BondCBs =
{
  NULL,                   // Passcode callback
  NULL                    // Pairing state callback
};

// 心率服务回调结构体
static HRMServiceCBs_t HRM_ServCBs =
{
  HRMServiceCB    // 心率服务回调函数
};

extern void HRM_Init( uint8 task_id )
{
  HRM_TaskID = task_id;
  
  // Setup the GAP Peripheral Role Profile
  {
    // 设置广告数据和扫描响应数据
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );
    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanResponseData ), scanResponseData );
    
    // 设置广告时间间隔
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, 1600 ); // units of 0.625ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_MIN, 0 ); // 不停地广播
    
    // 设置是否使能广告
    uint8 initial_advertising_enable = TRUE;
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );
    
    // 设置从连接建立到开始更新连接参数之间需要延时的时间(units of second)
    // 主要给主机留出时间完成service discovery任务
    GAP_SetParamValue( TGAP_CONN_PAUSE_PERIPHERAL, 2 ); 
    
    // 设置连接参数以及是否请求更新连接参数
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
  
  // 设置GGS设备名特征值
  GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName );

  // Setup the GAP Bond Manager
  // 设置配对、加密和绑定有关项
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

  // 设置心率服务Characteristic Values
  {
    uint8 sensLoc = HRM_SENS_LOC_CHEST;
    HRM_SetParameter( HRM_SENS_LOC, sizeof ( uint8 ), &sensLoc );
  }
  
  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );         // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES ); // GATT attributes
  HRM_AddService( GATT_ALL_SERVICES ); // 温湿度服务
  DevInfo_AddService( ); // device information service
  
  // 登记温湿度计的服务回调
  HRM_Register( &HRM_ServCBs );
  
  //在这里初始化GPIO
  //第一：所有管脚，reset后的状态都是输入加上拉
  //第二：对于不用的IO，建议不连接到外部电路，且设为输入上拉
  //第三：对于会用到的IO，就要根据具体外部电路连接情况进行有效设置，防止耗电
  HRMInitIOPin();
  
  HRFunc_Init();
  
  HCI_EXT_ClkDivOnHaltCmd( HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT );  

  // 启动设备
  osal_set_event( HRM_TaskID, HRM_START_DEVICE_EVT );
}


// 初始化IO管脚
static void HRMInitIOPin()
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
  // 已连接
  if( newState == GAPROLE_CONNECTED)
  {
    // Get connection handle
    GAPRole_GetParameter( GAPROLE_CONNHANDLE, &gapConnHandle );
  }
  // 断开连接
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

// 启动采样
static void HRMStart( void )
{  
  if(status == STATUS_STOP) {
    status = STATUS_START;
    HRFunc_Start();
    osal_start_timerEx( HRM_TaskID, HRM_START_PERIODIC_EVT, interval);
  }
}

// 停止采样
static void HRMStop( void )
{  
  status = STATUS_STOP;
  HRFunc_Stop();
  osal_stop_timerEx( HRM_TaskID, HRM_START_PERIODIC_EVT ); 
}

// 传输心率
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
