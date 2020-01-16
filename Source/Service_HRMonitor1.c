
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
* ����
*/
// ��ʪ�ȷ���UUID
CONST uint8 HRMServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(HRM_SERV_UUID)
};

// ��ʪ������UUID
CONST uint8 HRMDataUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(HRM_DATA_UUID)
};

// ����ʱ����UUID
CONST uint8 HRMIntervalUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HRM_INTERVAL_UUID), HI_UINT16(HRM_INTERVAL_UUID)
};

// ���������ΧUUID
CONST uint8 HRMIRangeUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HRM_IRANGE_UUID), HI_UINT16(HRM_IRANGE_UUID)
};


/*
* �ֲ�����
*/
// ��ʪ�ȷ�����������ֵ
static CONST gattAttrType_t HRMService = { ATT_UUID_SIZE, HRMServUUID };

// ��ʪ������
static uint8 HRMDataProps = GATT_PROP_INDICATE;
static attHandleValueInd_t dataInd;
static int16* pTemp = (int16*)dataInd.value; 
static uint16* pHumid = (uint16*)(dataInd.value+2);
static gattCharCfg_t HRMDataConfig[GATT_MAX_NUM_CONN];

// ����ʱ������units of second
static uint8 HRMIntervalProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint16 HRMInterval = 5;  

// ���������Χ
static HRMIRange_t  HRMIRange = {1,3600};

// ��������Ա�
static gattAttribute_t HRMAttrTbl[] = 
{
  // HRM ����
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&HRMService                /* pValue */
  },

    // ʵʱ��ʪ����������ֵ����
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &HRMDataProps 
    },

      // ��ʪ����������ֵ
      { 
        { ATT_UUID_SIZE, HRMDataUUID },
        GATT_PERMIT_READ, 
        0, 
        dataInd.value 
      }, 
      
      // ��ʪ������CCC
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)HRMDataConfig
      },

    // ����ʱ��������ֵ����
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &HRMIntervalProps
    },

      // ����ʱ��������ֵ
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
* �ֲ�����
*/
// ����Ӧ�ò���Ļص������ṹ��
static HRMServiceCBs_t * HRMService_AppCBs = NULL;

// �����Э��ջ�Ļص�����
// �����Իص�
static uint8 HRM_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );

// д���Իص�
static bStatus_t HRM_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );


// �����Э��ջ�Ļص��ṹ��
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
    // ������ʱ����ֵ  
    case HRM_INTERVAL_UUID:
      *pLen = 2;
      VOID osal_memcpy(pValue, &HRMInterval, 2);
      break;
      
    // �����������Χֵ   
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
    // д��ʪ�����ݵ�CCC  
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

    // д�������
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
 * ��������
*/

// ���ط���
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

// �Ǽ�Ӧ�ò���Ļص�
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

// ���ñ��������������
extern bStatus_t HRM_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    // ���ò���ʱ����
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
    
    // ���ò��������Χ  
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

// ��ȡ��������������
extern bStatus_t HRM_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // ��ȡ����ʱ����
    case HRM_INTERVAL:
      *((uint16*)value) = HRMInterval;
      break;
   
    // ��ȡ���������Χ  
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