
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "cmutil.h"

#include "Service_Battery.h"

/*
* 常量
*/
// 产生服务和特征的128位UUID
// 电池电量服务UUID
CONST uint8 batteryServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(BATTERY_SERV_UUID)
};

// 电池电量数据UUID
CONST uint8 batteryDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(BATTERY_DATA_UUID)
};

// 电池电量测量控制点UUID
CONST uint8 batteryCtrlUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(BATTERY_CTRL_UUID)
};

// 电池电量测量周期UUID
CONST uint8 batteryPeriodUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(BATTERY_PERI_UUID)
};



/*
* 局部变量
*/
// 电池电量服务属性值
static CONST gattAttrType_t batteryService = { ATT_UUID_SIZE, batteryServUUID };

// 电池电量数据的相关属性：可读可通知
static uint8 batteryDataProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8 batteryData[] = {0};
static gattCharCfg_t batteryDataConfig[GATT_MAX_NUM_CONN];

// 电池电量测量控制点的相关属性：可读可写
static uint8 batteryCtrlProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 batteryCtrl = 0;

// 电池电量测量周期的相关属性：可读可写
static uint8 batteryPeriodProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 batteryPeriod = 1;  // 单位为10秒钟，默认10秒测量一次



// 服务的属性表
static gattAttribute_t batteryServAttrTbl[] = 
{
  // battery 服务
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&batteryService                  /* pValue */
  },

    // 电池电量数据特征声明
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &batteryDataProps 
    },

      // 电池电量数据特征值
      { 
        { ATT_UUID_SIZE, batteryDataUUID },
        GATT_PERMIT_READ, 
        0, 
        batteryData 
      }, 
      
      // 电池电量数据特征的配置
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)batteryDataConfig
      },
      
    // 电池电量测量控制点特征声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &batteryCtrlProps
    },

      // 电池电量测量控制点特征值
      {
        { ATT_UUID_SIZE, batteryCtrlUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &batteryCtrl
      },

    // 电池电量测量周期特征声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &batteryPeriodProps
    },

      // 电池电量测量周期特征值
      {
        { ATT_UUID_SIZE, batteryPeriodUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &batteryPeriod
      },
};




/*
* 局部函数
*/
// 保存应用层给的回调函数实例
static batteryServiceCBs_t *batteryService_AppCBs = NULL;

// 服务给协议栈的回调函数
// 读属性回调
static uint8 battery_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
// 写属性回调
static bStatus_t battery_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
// 连接状态改变回调
static void battery_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// 服务给协议栈的回调结构体实例
CONST gattServiceCBs_t batteryServCBs =
{
  battery_ReadAttrCB,      // Read callback function pointer
  battery_WriteAttrCB,     // Write callback function pointer
  NULL                       // Authorization callback function pointer
};


static uint8 battery_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;
  uint16 uuid;

  // If attribute permissions require authorization to read, return error
  if ( gattPermitAuthorRead( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
  
  if (utilExtractUuid16(pAttr, &uuid) == FAILURE) {
    // Invalid handle
    *pLen = 0;
    return ATT_ERR_INVALID_HANDLE;
  }
  
  switch ( uuid )
  {
    // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
    // gattserverapp handles those reads
    case BATTERY_DATA_UUID:
      // 读电池电量值
      *pLen = 1;
      VOID osal_memcpy( pValue, pAttr->pValue, 1 );
      break;
      
    // 读控制点和周期值，都是单字节  
    case BATTERY_CTRL_UUID:
    case BATTERY_PERI_UUID:
      *pLen = 1;
      pValue[0] = *pAttr->pValue;
      break;
      
    default:
      *pLen = 0;
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }  
  
  return status;
}


static bStatus_t battery_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
  uint8 notifyApp = 0xFF;
  uint16 uuid;
  
  // If attribute permissions require authorization to write, return error
  if ( gattPermitAuthorWrite( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  if (utilExtractUuid16(pAttr,&uuid) == FAILURE) {
    // Invalid handle
    return ATT_ERR_INVALID_HANDLE;
  }
  
  switch ( uuid )
  {
    case BATTERY_DATA_UUID:
      // 电池电量值不能写
      break;

    // 写测量控制点
    case BATTERY_CTRL_UUID:
      // Validate the value
      // Make sure it's not a blob oper
      if ( offset == 0 )
      {
        if ( len != 1 )
        {
          status = ATT_ERR_INVALID_VALUE_SIZE;
        }
      }
      else
      {
        status = ATT_ERR_ATTR_NOT_LONG;
      }

      // Write the value
      if ( status == SUCCESS )
      {
        uint8 *pCurValue = (uint8 *)pAttr->pValue;

        *pCurValue = pValue[0];

        if( pAttr->pValue == &batteryCtrl )
        {
          notifyApp = BATTERY_CTRL;
        }
      }
      break;

    // 写测量周期
    case BATTERY_PERI_UUID:
      // Validate the value
      // Make sure it's not a blob oper
      if ( offset == 0 )
      {
        if ( len != 1 )
        {
          status = ATT_ERR_INVALID_VALUE_SIZE;
        }
      }
      else
      {
        status = ATT_ERR_ATTR_NOT_LONG;
      }
      // Write the value
      if ( status == SUCCESS )
      {
        uint8 *pCurValue = (uint8 *)pAttr->pValue;
        *pCurValue = pValue[0];

        if( pAttr->pValue == &batteryPeriod )
        {
          notifyApp = BATTERY_PERI;
        }
      }
      break;
      

    // 写电池电量数据的CCC  
    case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                              offset, GATT_CLIENT_CFG_NOTIFY );
      break;

    default:
      // Should never get here!
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  // If a charactersitic value changed then callback function to notify application of change
  if ( (notifyApp != 0xFF ) && batteryService_AppCBs && batteryService_AppCBs->pfnBatteryServiceCB )
  {
    batteryService_AppCBs->pfnBatteryServiceCB( notifyApp );
  }  
  
  return status;
}

static void battery_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      GATTServApp_InitCharCfg( connHandle, batteryDataConfig );
    }
  }
}




/*
 * 公共函数
*/

// 加载服务
extern bStatus_t Battery_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, batteryDataConfig );

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( battery_HandleConnStatusCB );  
  
  if ( services & BATTERY_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( batteryServAttrTbl, 
                                          GATT_NUM_ATTRS( batteryServAttrTbl ),
                                          &batteryServCBs );
  }

  return ( status );
}

// 登记应用层给的回调
extern bStatus_t Battery_RegisterAppCBs( batteryServiceCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    batteryService_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

// 设置本服务的特征参数
extern bStatus_t Battery_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    // 设置电池电量数据，触发Notification
    case BATTERY_DATA:
      if ( len == sizeof(uint8) )
      {
        VOID osal_memcpy( batteryData, value, 1 );
        
        // See if Notification has been enabled
        GATTServApp_ProcessCharCfg( batteryDataConfig, batteryData, FALSE,
                                   batteryServAttrTbl, GATT_NUM_ATTRS( batteryServAttrTbl ),
                                   INVALID_TASK_ID );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    // 设置测量控制点
    case BATTERY_CTRL:
      if ( len == sizeof ( uint8 ) )
      {
        batteryCtrl = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    // 设置测量周期  
    case BATTERY_PERI:
      if ( len == sizeof ( uint8 ) )
      {
        batteryPeriod = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }  
  
  return ( ret );
}

// 获取本服务特征参数
extern bStatus_t Battery_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // 获取电池电量数据
    case BATTERY_DATA:
      VOID osal_memcpy( value, batteryData, 1 );
      break;

    // 获取测量控制点  
    case BATTERY_CTRL:
      *((uint8*)value) = batteryCtrl;
      break;

    // 获取测量周期  
    case BATTERY_PERI:
      *((uint8*)value) = batteryPeriod;
      break;
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

