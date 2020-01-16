
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
* ����
*/
// ���������������128λUUID
// ��ص�������UUID
CONST uint8 batteryServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(BATTERY_SERV_UUID)
};

// ��ص�������UUID
CONST uint8 batteryDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(BATTERY_DATA_UUID)
};

// ��ص����������Ƶ�UUID
CONST uint8 batteryCtrlUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(BATTERY_CTRL_UUID)
};

// ��ص�����������UUID
CONST uint8 batteryPeriodUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(BATTERY_PERI_UUID)
};



/*
* �ֲ�����
*/
// ��ص�����������ֵ
static CONST gattAttrType_t batteryService = { ATT_UUID_SIZE, batteryServUUID };

// ��ص������ݵ�������ԣ��ɶ���֪ͨ
static uint8 batteryDataProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8 batteryData[] = {0};
static gattCharCfg_t batteryDataConfig[GATT_MAX_NUM_CONN];

// ��ص����������Ƶ��������ԣ��ɶ���д
static uint8 batteryCtrlProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 batteryCtrl = 0;

// ��ص����������ڵ�������ԣ��ɶ���д
static uint8 batteryPeriodProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 batteryPeriod = 1;  // ��λΪ10���ӣ�Ĭ��10�����һ��



// ��������Ա�
static gattAttribute_t batteryServAttrTbl[] = 
{
  // battery ����
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&batteryService                  /* pValue */
  },

    // ��ص���������������
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &batteryDataProps 
    },

      // ��ص�����������ֵ
      { 
        { ATT_UUID_SIZE, batteryDataUUID },
        GATT_PERMIT_READ, 
        0, 
        batteryData 
      }, 
      
      // ��ص�����������������
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)batteryDataConfig
      },
      
    // ��ص����������Ƶ���������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &batteryCtrlProps
    },

      // ��ص����������Ƶ�����ֵ
      {
        { ATT_UUID_SIZE, batteryCtrlUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &batteryCtrl
      },

    // ��ص�������������������
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &batteryPeriodProps
    },

      // ��ص���������������ֵ
      {
        { ATT_UUID_SIZE, batteryPeriodUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &batteryPeriod
      },
};




/*
* �ֲ�����
*/
// ����Ӧ�ò���Ļص�����ʵ��
static batteryServiceCBs_t *batteryService_AppCBs = NULL;

// �����Э��ջ�Ļص�����
// �����Իص�
static uint8 battery_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
// д���Իص�
static bStatus_t battery_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
// ����״̬�ı�ص�
static void battery_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// �����Э��ջ�Ļص��ṹ��ʵ��
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
      // ����ص���ֵ
      *pLen = 1;
      VOID osal_memcpy( pValue, pAttr->pValue, 1 );
      break;
      
    // �����Ƶ������ֵ�����ǵ��ֽ�  
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
      // ��ص���ֵ����д
      break;

    // д�������Ƶ�
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

    // д��������
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
      

    // д��ص������ݵ�CCC  
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
 * ��������
*/

// ���ط���
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

// �Ǽ�Ӧ�ò���Ļص�
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

// ���ñ��������������
extern bStatus_t Battery_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    // ���õ�ص������ݣ�����Notification
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

    // ���ò������Ƶ�
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

    // ���ò�������  
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

// ��ȡ��������������
extern bStatus_t Battery_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // ��ȡ��ص�������
    case BATTERY_DATA:
      VOID osal_memcpy( value, batteryData, 1 );
      break;

    // ��ȡ�������Ƶ�  
    case BATTERY_CTRL:
      *((uint8*)value) = batteryCtrl;
      break;

    // ��ȡ��������  
    case BATTERY_PERI:
      *((uint8*)value) = batteryPeriod;
      break;
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

