
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "hal_adc.h"
#include "Service_Battery.h"


// service and charecteristic UUID
// battery service UUID
CONST uint8 batteryServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATTERY_SERV_UUID), HI_UINT16(BATTERY_SERV_UUID)
};

// battery level UUID
CONST uint8 batteryLevelUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATTERY_LEVEL_UUID), HI_UINT16(BATTERY_LEVEL_UUID)
};


static CONST gattAttrType_t batteryService = { ATT_BT_UUID_SIZE, batteryServUUID };
static uint8 batteryLevelProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8 batteryLevel = 100;
static attHandleValueNoti_t battNoti;
static gattCharCfg_t batteryLevelConfig[GATT_MAX_NUM_CONN];


// attribute table
static gattAttribute_t batteryServAttrTbl[] = 
{
  // battery service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&batteryService                  /* pValue */
  },

    // battery level
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &batteryLevelProps 
    },

      // 
      { 
        { ATT_BT_UUID_SIZE, batteryLevelUUID },
        GATT_PERMIT_READ, 
        0, 
        (uint8*)&batteryLevel 
      }, 
      
      // 
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)batteryLevelConfig
      },
};

// save the instance of the callback struct in the application
static BattServiceCBs_t *battService_AppCBs = NULL;

static uint8 readAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t writeAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
static void handleConnStatusCB( uint16 connHandle, uint8 changeType );

static uint8 batteryMeasure(); // measure the percent of the battery level
static bStatus_t batteryNotify(uint16 connHandle); // send the notification of the battery level

CONST gattServiceCBs_t batteryCBs =
{
  readAttrCB,      // Read callback function pointer
  writeAttrCB,     // Write callback function pointer
  NULL             // Authorization callback function pointer
};

// add battery service
extern bStatus_t Battery_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, batteryLevelConfig );

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( handleConnStatusCB );  
  
  if ( services & BATTERY_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( batteryServAttrTbl, 
                                          GATT_NUM_ATTRS( batteryServAttrTbl ),
                                          &batteryCBs );
  }

  return ( status );
}

// register battery service callback struct in the application
extern bStatus_t Battery_RegisterAppCBs( BattServiceCBs_t *appCallbacks )
{
  battService_AppCBs = appCallbacks;
    
  return ( SUCCESS );
}

// get the characteristic parameter in battery service
extern bStatus_t Battery_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  
  switch ( param )
  {
    // battery level
    case BATTERY_LEVEL:
      *((uint8*)value) = batteryLevel;
      break;
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

// measure the battery level and send a notification on the connection handle.
extern bStatus_t Battery_MeasLevel( uint16 connHandle )
{
  uint8 level;

  level = batteryMeasure();

  // If level has gone down
  if (level < batteryLevel)
  {
    // Update level
    batteryLevel = level;

    // Send a notification
    batteryNotify(connHandle);
  }

  return SUCCESS;
}

static uint8 readAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
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
  
  uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1] );

  // Measure battery level if reading level
  if ( uuid == BATTERY_LEVEL_UUID )
  {
    uint8 level;

    level = batteryMeasure();

    // If level has gone down
    if (level < batteryLevel)
    {
      // Update level
      batteryLevel = level;
    }

    *pLen = 1;
    pValue[0] = batteryLevel;
  }
  else
  {
    status = ATT_ERR_ATTR_NOT_FOUND;
  }

  return ( status );
}


static bStatus_t writeAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
  uint16 uuid;
  
  // If attribute permissions require authorization to write, return error
  if ( gattPermitAuthorWrite( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
  
  switch ( uuid )
  {
    // battery level CCC  
    case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                              offset, GATT_CLIENT_CFG_NOTIFY );
      
      if ( status == SUCCESS )
      {
        uint16 charCfg = BUILD_UINT16( pValue[0], pValue[1] );

        battService_AppCBs->pfnBattServiceCB( (charCfg == GATT_CFG_NO_OPERATION) ?
                                BATTERY_LEVEL_NOTI_DISABLED :
                                BATTERY_LEVEL_NOTI_ENABLED );
      }
      break;

    default:
      // Should never get here!
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }
  
  return status;
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
      GATTServApp_InitCharCfg( connHandle, batteryLevelConfig );
    }
  }
}

// measure the percent of the battery level
static uint8 batteryMeasure()
{
  uint8 percent = 0;
  HalAdcSetReference( HAL_ADC_REF_125V );
  uint16 adc = HalAdcRead( HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_10 );
  
  /* how to convert adc to the percent of battery level
  // The internal reference voltage of CC2541 is 1.24V
  // if V is input voltage, then the output adc is:
  // adc = (V/3)/1.24*511
  // so: 
  // MAXadc = (3/3)/1.24*511 = 412
  // MINadc = (2.27/3)/1.24*511 = 312 , where 2.7V is the required lowest voltage of ADS1X91, but the experiment shows 2.27V still can work
  // then the percent value is equal to:
  // percent = (adc - MINadc)/(MAXadc - MINadc)*100 = (adc - 312)/100*100
  */
  
  if(adc >= 412)
  {
    percent = 100;
  } 
  else if(adc <= 312)
  {
    percent = 0;
  }
  else
  {
    percent = (uint8)( (adc - 312) );
  }  
  
  return percent;
}

// send the notification of the battery level
static bStatus_t batteryNotify(uint16 connHandle)
{
  if(linkDB_Up(connHandle))
  {
    uint16 value = GATTServApp_ReadCharCfg( connHandle, batteryLevelConfig );

    // If notifications enabled
    if ( value & GATT_CLIENT_CFG_NOTIFY )
    {
      battNoti.handle = batteryServAttrTbl[BATTERY_LEVEL_POS].handle;
      battNoti.len = 1;
      battNoti.value[0] = batteryLevel;
    
      // Send the notification
      return GATT_Notification( connHandle, &battNoti, FALSE );
    }
  }
     
  return bleIncorrectMode;
}