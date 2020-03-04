/**
* ecg service source file: providing the ecg-related info and sending the ecg data packet
*/

#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "CMUtil.h"
#include "Service_Ecg.h"

// Position of ECG data packet in attribute array
#define ECG_PACK_VALUE_POS            2

// Ecg service
CONST uint8 ECGServUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(ECG_SERV_UUID)
};

// Ecg Data Packet characteristic
CONST uint8 ECGPackUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(ECG_PACK_UUID)
};

// 1mV calibration characteristic
CONST uint8 ECG1mVCaliUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(ECG_1MV_CALI_UUID)
};

// Sample rate characteristic
CONST uint8 ECGSampleRateUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(ECG_SAMPLE_RATE_UUID)
};

// Lead Type characteristic
CONST uint8 ECGLeadTypeUUID[ATT_UUID_SIZE] =
{ 
  CM_UUID(ECG_LEAD_TYPE_UUID)
};

static ECGServiceCBs_t* ecgServiceCBs;

// Ecg Service attribute
static CONST gattAttrType_t ecgService = { ATT_UUID_SIZE, ECGServUUID };

// Ecg Data Packet Characteristic
// Note: the characteristic value is not stored here
static uint8 ecgPackProps = GATT_PROP_NOTIFY;
static uint8 ecgPack = 0;
static gattCharCfg_t ecgPackClientCharCfg[GATT_MAX_NUM_CONN];

// 1mV Calibration Characteristic
static uint8 ecg1mVCaliProps = GATT_PROP_READ;
static uint16 ecg1mVCali = 0;

// Sample Rate Characteristic
static uint8 ecgSampleRateProps = GATT_PROP_READ;
static uint16 ecgSampleRate = SAMPLERATE;

// Lead Type Characteristic
static uint8 ecgLeadTypeProps = GATT_PROP_READ;
static uint8 ecgLeadType = ECG_LEAD_TYPE_I;

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t ECGAttrTbl[] = 
{
  // Ecg Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&ecgService                      /* pValue */
  },

    // 1. Ecg Data Packet Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &ecgPackProps 
    },

      // Ecg Data Packet Value
      { 
        { ATT_UUID_SIZE, ECGPackUUID },
        0, 
        0, 
        &ecgPack 
      },

      // Ecg Data Packet Client Characteristic Configuration
      { 
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        (uint8 *) &ecgPackClientCharCfg 
      },      

    // 2. 1mV Calibration Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &ecg1mVCaliProps 
    },

      // 1mV Calibration Value
      { 
        { ATT_UUID_SIZE, ECG1mVCaliUUID },
        GATT_PERMIT_READ, 
        0, 
        (uint8*)&ecg1mVCali 
      },

    // 3. Sample Rate Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &ecgSampleRateProps 
    },

      // Sample Rate Value
      { 
        { ATT_UUID_SIZE, ECGSampleRateUUID },
        GATT_PERMIT_READ, 
        0, 
        (uint8*)&ecgSampleRate 
      },

    // 4. Lead Type Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &ecgLeadTypeProps 
    },

      // Lead Type Value
      { 
        { ATT_UUID_SIZE, ECGLeadTypeUUID },
        GATT_PERMIT_READ, 
        0, 
        &ecgLeadType 
      }
};

static uint8 readAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t writeAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
static void handleConnStatusCB( uint16 connHandle, uint8 changeType );

// Ecg Service Callbacks
CONST gattServiceCBs_t ecgCBs =
{
  readAttrCB,  // Read callback function pointer
  writeAttrCB, // Write callback function pointer
  NULL                   // Authorization callback function pointer
};

bStatus_t ECG_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, ecgPackClientCharCfg );
  
  VOID linkDB_Register(handleConnStatusCB);

  if ( services & ECG_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( ECGAttrTbl, 
                                          GATT_NUM_ATTRS( ECGAttrTbl ),
                                          &ecgCBs );
  }

  return ( status );
}

extern void ECG_Register( ECGServiceCBs_t* pfnServiceCBs )
{
  ecgServiceCBs = pfnServiceCBs;
    
  return;
}

extern bStatus_t ECG_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
     case ECG_PACK_CHAR_CFG:
      // Need connection handle
      //ECGMeasClientCharCfg.value = *((uint16*)value);
      break;      

    case ECG_1MV_CALI:
      osal_memcpy((uint8*)&ecg1mVCali, value, len);
      break;
      
    case ECG_LEAD_TYPE:  
      ecgLeadType = *((uint8*)value);
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

extern bStatus_t ECG_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case ECG_PACK_CHAR_CFG:
      // Need connection handle
      //*((uint16*)value) = ECGMeasClientCharCfg.value;
      break;      

    case ECG_1MV_CALI:
      osal_memcpy(value, (uint8*)&ecg1mVCali, 2);
      break;
      
    case ECG_LEAD_TYPE:  
      *((uint8*)value) = ecgLeadType;
      break; 

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

extern bStatus_t ECG_PacketNotify( uint16 connHandle, attHandleValueNoti_t *pNoti )
{
  uint16 value = GATTServApp_ReadCharCfg( connHandle, ecgPackClientCharCfg );

  // If notifications enabled
  if ( value & GATT_CLIENT_CFG_NOTIFY )
  {
    // Set the handle
    pNoti->handle = ECGAttrTbl[ECG_PACK_VALUE_POS].handle;
  
    // Send the notification
    return GATT_Notification( connHandle, pNoti, FALSE );
  }

  return bleIncorrectMode;
}
                               
static uint8 readAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  uint16 uuid = 0;
  if (utilExtractUuid16(pAttr, &uuid) == FAILURE) {
    // Invalid handle
    *pLen = 0;
    return ATT_ERR_INVALID_HANDLE;
  }

  switch(uuid)
  {
    case ECG_1MV_CALI_UUID:
    case ECG_SAMPLE_RATE_UUID:
      *pLen = 2;
       VOID osal_memcpy( pValue, pAttr->pValue, 2 );
       break;
       
    case ECG_LEAD_TYPE_UUID:
      *pLen = 1;
      pValue[0] = *pAttr->pValue;
      break;
      
    default:
      *pLen = 0;
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  return ( status );
}

static bStatus_t writeAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
 
  uint16 uuid = 0;
  if (utilExtractUuid16(pAttr,&uuid) == FAILURE) {
    // Invalid handle
    return ATT_ERR_INVALID_HANDLE;
  }
  
  switch ( uuid )
  {
    case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                               offset, GATT_CLIENT_CFG_NOTIFY );
      if ( status == SUCCESS )
      {
        uint16 charCfg = BUILD_UINT16( pValue[0], pValue[1] );

        (ecgServiceCBs->pfnEcgServiceCB)( (charCfg == GATT_CFG_NO_OPERATION) ?
                                ECG_PACK_NOTI_DISABLED :
                                ECG_PACK_NOTI_ENABLED );
      }
      break;       
 
    default:
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  return ( status );
}

static void handleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      GATTServApp_InitCharCfg( connHandle, ecgPackClientCharCfg );
    }
  }
}
