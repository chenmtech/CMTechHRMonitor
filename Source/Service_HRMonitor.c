
/*********************************************************************
 * INCLUDES
 */
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include "Service_HRMonitor.h"

// Position of heart rate measurement value in attribute array
#define HRM_MEAS_VALUE_POS            2

// Heart rate service
CONST uint8 HRMServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HRM_SERV_UUID), HI_UINT16(HRM_SERV_UUID)
};

// Heart rate measurement characteristic
CONST uint8 HRMMeasUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HRM_MEAS_UUID), HI_UINT16(HRM_MEAS_UUID)
};

// Sensor location characteristic
CONST uint8 HRMSensLocUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HRM_SENS_LOC_UUID), HI_UINT16(HRM_SENS_LOC_UUID)
};

// Control point characteristic
CONST uint8 HRMCtrlPtUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HRM_CTRL_PT_UUID), HI_UINT16(HRM_CTRL_PT_UUID)
};

static HRMServiceCBs_t* HRMServiceCBs;

// Heart Rate Service attribute
static CONST gattAttrType_t HRMService = { ATT_BT_UUID_SIZE, HRMServUUID };

// Heart Rate Measurement Characteristic
// Note characteristic value is not stored here
static uint8 HRMMeasProps = GATT_PROP_NOTIFY;
static uint8 HRMMeas = 0;
static gattCharCfg_t HRMMeasClientCharCfg[GATT_MAX_NUM_CONN];

// Sensor Location Characteristic
static uint8 HRMSensLocProps = GATT_PROP_READ;
static uint8 HRMSensLoc = 0;

// Control point Characteristic
static uint8 HRMCtrlPtProps = GATT_PROP_WRITE;
static uint8 HRMCtrlPt = 0;

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t HRMAttrTbl[] = 
{
  // Heart Rate Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&HRMService                /* pValue */
  },

    // Heart Rate Measurement Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &HRMMeasProps 
    },

      // Heart Rate Measurement Value
      { 
        { ATT_BT_UUID_SIZE, HRMMeasUUID },
        0, 
        0, 
        &HRMMeas 
      },

      // Heart Rate Measurement Client Characteristic Configuration
      { 
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        (uint8 *) &HRMMeasClientCharCfg 
      },      

    // Sensor Location Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &HRMSensLocProps 
    },

      // Sensor Location Value
      { 
        { ATT_BT_UUID_SIZE, HRMSensLocUUID },
        GATT_PERMIT_READ, 
        0, 
        &HRMSensLoc 
      },

    // Control point Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &HRMCtrlPtProps 
    },

      // Control point Value
      { 
        { ATT_BT_UUID_SIZE, HRMCtrlPtUUID },
        GATT_PERMIT_WRITE, 
        0, 
        &HRMCtrlPt 
      }
};

static uint8 HRM_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t HRM_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

static void HRM_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// Heart Rate Service Callbacks
CONST gattServiceCBs_t HRMCBs =
{
  HRM_ReadAttrCB,  // Read callback function pointer
  HRM_WriteAttrCB, // Write callback function pointer
  NULL                   // Authorization callback function pointer
};

bStatus_t HRM_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, HRMMeasClientCharCfg );
  
  VOID linkDB_Register(HRM_HandleConnStatusCB);

  if ( services & HRM_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( HRMAttrTbl, 
                                          GATT_NUM_ATTRS( HRMAttrTbl ),
                                          &HRMCBs );
  }

  return ( status );
}

extern void HRM_Register( HRMServiceCBs_t* pfnServiceCBs )
{
  HRMServiceCBs = pfnServiceCBs;
    
  return;
}

extern bStatus_t HRM_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
     case HRM_MEAS_CHAR_CFG:
      // Need connection handle
      //HRMMeasClientCharCfg.value = *((uint16*)value);
      break;      

    case HRM_SENS_LOC:
      HRMSensLoc = *((uint8*)value);
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

extern bStatus_t HRM_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case HRM_MEAS_CHAR_CFG:
      // Need connection handle
      //*((uint16*)value) = HRMMeasClientCharCfg.value;
      break;      

    case HRM_SENS_LOC:
      *((uint8*)value) = HRMSensLoc;
      break;
      
    case HRM_CTRL_PT:
      *((uint8*)value) = HRMCtrlPt;
      break;  

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

extern bStatus_t HRM_MeasNotify( uint16 connHandle, attHandleValueNoti_t *pNoti )
{
  uint16 value = GATTServApp_ReadCharCfg( connHandle, HRMMeasClientCharCfg );

  // If notifications enabled
  if ( value & GATT_CLIENT_CFG_NOTIFY )
  {
    // Set the handle
    pNoti->handle = HRMAttrTbl[HRM_MEAS_VALUE_POS].handle;
  
    // Send the notification
    return GATT_Notification( connHandle, pNoti, FALSE );
  }

  return bleIncorrectMode;
}
                               
static uint8 HRM_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);

  if (uuid == HRM_SENS_LOC_UUID)
  {
    *pLen = 1;
    pValue[0] = *pAttr->pValue;
  }
  else
  {
    status = ATT_ERR_ATTR_NOT_FOUND;
  }

  return ( status );
}

static bStatus_t HRM_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
 
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
  switch ( uuid )
  {
    case HRM_CTRL_PT_UUID:
      if ( offset > 0 )
      {
        status = ATT_ERR_ATTR_NOT_LONG;
      }
      else if (len != 1)
      {
        status = ATT_ERR_INVALID_VALUE_SIZE;
      }
      else if (*pValue != HRM_CTRL_PT_ENERGY_EXP)
      {
        status = HRM_ERR_NOT_SUP;
      }
      else
      {
        *(pAttr->pValue) = pValue[0];
        
        (HRMServiceCBs->pfnHRMServiceCB)(HRM_CTRL_PT_SET);
        
      }
      break;

    case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                               offset, GATT_CLIENT_CFG_NOTIFY );
      if ( status == SUCCESS )
      {
        uint16 charCfg = BUILD_UINT16( pValue[0], pValue[1] );

        (HRMServiceCBs->pfnHRMServiceCB)( (charCfg == GATT_CFG_NO_OPERATION) ?
                                HRM_MEAS_NOTI_DISABLED :
                                HRM_MEAS_NOTI_ENABLED );
      }
      break;       
 
    default:
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  return ( status );
}

static void HRM_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      GATTServApp_InitCharCfg( connHandle, HRMMeasClientCharCfg );
    }
  }
}


/*********************************************************************
*********************************************************************/
