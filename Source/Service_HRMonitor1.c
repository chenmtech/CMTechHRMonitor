
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"
#include "cmutil.h"

#include "Service_HRMonitor.h"

/*
* 常量
*/
// 温湿度服务UUID
CONST uint8 HRMServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(HRM_SERV_UUID)
};

// 温湿度数据UUID
CONST uint8 HRMDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(HRM_DATA_UUID)
};

// 测量时间间隔UUID
CONST uint8 HRMIntervalUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HRM_INTERVAL_UUID), HI_UINT16(HRM_INTERVAL_UUID)
};

// 测量间隔范围UUID
CONST uint8 HRMIRangeUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HRM_IRANGE_UUID), HI_UINT16(HRM_IRANGE_UUID)
};


/*
* 局部变量
*/
// 温湿度服务属性类型值
static CONST gattAttrType_t HRMService = { ATT_UUID_SIZE, HRMServUUID };

// 温湿度数据
static uint8 HRMDataProps = GATT_PROP_INDICATE;
static attHandleValueInd_t dataInd;
static int16* pTemp = (int16*)dataInd.value; 
static uint16* pHumid = (uint16*)(dataInd.value+2);
static gattCharCfg_t HRMDataConfig[GATT_MAX_NUM_CONN];

// 测量时间间隔，units of second
static uint8 HRMIntervalProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint16 HRMInterval = 5;  

// 测量间隔范围
static HRMIRange_t  HRMIRange = {1,3600};

// 服务的属性表
static gattAttribute_t HRMAttrTbl[] = 
{
  // HRM 服务
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&HRMService                /* pValue */
  },

    // 实时温湿度数据特征值声明
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &HRMDataProps 
    },

      // 温湿度数据特征值
      { 
        { ATT_UUID_SIZE, HRMDataUUID },
        GATT_PERMIT_READ, 
        0, 
        dataInd.value 
      }, 
      
      // 温湿度数据CCC
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)HRMDataConfig
      },

    // 测量时间间隔特征值声明
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &HRMIntervalProps
    },

      // 测量时间间隔特征值
      {
        { ATT_BT_UUID_SIZE, HRMIntervalUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8*)&HRMInterval
      },
      
      { 
        { ATT_BT_UUID_SIZE, HRMIRangeUUID },
        GATT_PERMIT_READ,
        0, 
        (uint8 *)&HRMIRange 
      },
};


/*
* 局部函数
*/
// 保存应用层给的回调函数结构体
static HRMServiceCBs_t * HRMService_AppCBs = NULL;

// 服务给协议栈的回调函数
// 读属性回调
static uint8 HRM_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );

// 写属性回调
static bStatus_t HRM_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );


// 服务给协议栈的回调结构体
CONST gattServiceCBs_t HRMServCBs =
{
  HRM_ReadAttrCB,      // Read callback function pointer
  HRM_WriteAttrCB,     // Write callback function pointer
  NULL                       // Authorization callback function pointer
};


static uint8 HRM_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
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
    return ATT_ERR_ATTR_NOT_FOUND;
  }
  
  switch ( uuid )
  {
    // 读测量时间间隔值  
    case HRM_INTERVAL_UUID:
      *pLen = 2;
      VOID osal_memcpy(pValue, &HRMInterval, 2);
      break;
      
    // 读测量间隔范围值   
    case HRM_IRANGE_UUID:
      *pLen = 4;
       VOID osal_memcpy( pValue, &HRMIRange, 4 ) ;
      break;   
      
    default:
      *pLen = 0;
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }  
  
  return status;
}


static bStatus_t HRM_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
  uint16 uuid;
  uint16 interval;
  
  // If attribute permissions require authorization to write, return error
  if ( gattPermitAuthorWrite( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  if (utilExtractUuid16(pAttr, &uuid) == FAILURE) {
    return ATT_ERR_ATTR_NOT_FOUND;
  }
  
  switch ( uuid )
  {
    // 写温湿度数据的CCC  
    case GATT_CLIENT_CHAR_CFG_UUID:
      if(pAttr->handle == HRMAttrTbl[HRM_DATA_CHAR_CFG_POS].handle)
      {
        status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                              offset, GATT_CLIENT_CFG_INDICATE );
        if(status == SUCCESS)
        {
          uint16 value = BUILD_UINT16( pValue[0], pValue[1] );
          uint8 event = (value == GATT_CFG_NO_OPERATION) ? HRM_DATA_IND_DISABLED : HRM_DATA_IND_ENABLED;
          HRMService_AppCBs->pfnHRMServiceCB(event);
        }
      }
      else 
      {
        status = ATT_ERR_INVALID_HANDLE;
      }
      break;

    // 写测量间隔
    case HRM_INTERVAL_UUID:
      // Validate the value
      // Make sure it's not a blob oper
      if ( offset == 0 )
      {
        if ( len != 2 )
        {
          status = ATT_ERR_INVALID_VALUE_SIZE;
        }
      }
      else
      {
        status = ATT_ERR_ATTR_NOT_LONG;
      }
      
      interval = (uint16)*pValue;
      
      //validate range
      if ((interval >= HRMIRange.high) | ((interval <= HRMIRange.low) & (interval != 0)))
      {
        status = ATT_ERR_INVALID_VALUE;
      }
      
      // Write the value
      if ( status == SUCCESS )
      {
        uint8 *pCurValue = (uint8 *)pAttr->pValue;        
        // *pCurValue = *pValue; 
        VOID osal_memcpy( pCurValue, pValue, 2 ) ;
        
        //notify application of write
        HRMService_AppCBs->pfnHRMServiceCB(HRM_INTERVAL_SET);
      }
      break;
    
    default:
      // Should never get here!
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  return status;
}

extern void HRM_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      GATTServApp_InitCharCfg( connHandle, HRMDataConfig );
    }
  }
}










/*
 * 公共函数
*/

// 加载服务
extern bStatus_t HRM_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, HRMDataConfig );

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( HRM_HandleConnStatusCB );  
  
  if ( services & HRM_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( HRMAttrTbl, 
                                          GATT_NUM_ATTRS( HRMAttrTbl ),
                                          &HRMServCBs );
  }

  return ( status );
}

// 登记应用层给的回调
extern bStatus_t HRM_RegisterAppCBs( HRMServiceCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    HRMService_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

// 设置本服务的特征参数
extern bStatus_t HRM_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    // 设置测量时间间隔
    case HRM_INTERVAL:
      if ( len == 2 )
      {
        osal_memcpy(&HRMInterval, (uint8*)value, 2);
      }
      else
      {
        ret = INVALIDPARAMETER;
      }
      break;
    
    // 设置测量间隔范围  
    case HRM_IRANGE:    
      if(len == 4)
      {
        HRMIRange = *((HRMIRange_t*)value);
      }
      else
      {
        ret = INVALIDPARAMETER;
      }
      break;  

    default:
      ret = INVALIDPARAMETER;
      break;
  }  
  
  return ( ret );
}

// 获取本服务特征参数
extern bStatus_t HRM_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // 获取测量时间间隔
    case HRM_INTERVAL:
      *((uint16*)value) = HRMInterval;
      break;
   
    // 获取测量间隔范围  
    case HRM_IRANGE:
      *((HRMIRange_t*)value) = HRMIRange;
      break;      

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

extern bStatus_t HRM_HRIndicate( uint16 connHandle, int16 temp, uint16 humid, uint8 taskId )
{
  uint16 value = GATTServApp_ReadCharCfg( connHandle, HRMDataConfig );

  // If indications enabled
  if ( value & GATT_CLIENT_CFG_INDICATE )
  {
    *pTemp = temp;
    *pHumid = humid;
    // Set the handle (uses stored relative handle to lookup actual handle)
    dataInd.handle = HRMAttrTbl[HRM_DATA_CHAR_CFG_POS].handle;
    dataInd.len = 4;
  
    // Send the Indication
    return GATT_Indication( connHandle, &dataInd, FALSE, taskId );
  }

  return bleIncorrectMode;
}