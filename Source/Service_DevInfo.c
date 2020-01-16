
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

#include "Service_DevInfo.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Device information service
CONST uint8 devInfoServUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(DEVINFO_SERV_UUID), HI_UINT16(DEVINFO_SERV_UUID)
};

// System ID
CONST uint8 devInfoSystemIdUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(DEVINFO_SYSTEM_ID_UUID), HI_UINT16(DEVINFO_SYSTEM_ID_UUID)
};

// Model Number String
CONST uint8 devInfoModelNumberUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(DEVINFO_MODEL_NUMBER_UUID), HI_UINT16(DEVINFO_MODEL_NUMBER_UUID)
};

// Manufacturer Name String
CONST uint8 devInfoMfrNameUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(DEVINFO_MANUFACTURER_NAME_UUID), HI_UINT16(DEVINFO_MANUFACTURER_NAME_UUID)
};


/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * Profile Attributes - variables
 */

// Device Information Service attribute
static CONST gattAttrType_t devInfoService = { ATT_BT_UUID_SIZE, devInfoServUUID };

// System ID characteristic
static uint8 devInfoSystemIdProps = GATT_PROP_READ;
static uint8 devInfoSystemId[DEVINFO_SYSTEM_ID_LEN] = {0, 0, 0, 0, 0, 0, 0, 0};

// Model Number String characteristic
static uint8 devInfoModelNumberProps = GATT_PROP_READ;
static const uint8 devInfoModelNumber[] = "CMTempHumid1.0";

// Manufacturer Name String characteristic
static uint8 devInfoMfrNameProps = GATT_PROP_READ;
static const uint8 devInfoMfrName[] = "CMTech Inc";

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t devInfoAttrTbl[] =
{
  // Device Information Service
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&devInfoService                /* pValue */
  },

    // System ID Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &devInfoSystemIdProps
    },

      // System ID Value
      {
        { ATT_BT_UUID_SIZE, devInfoSystemIdUUID },
        GATT_PERMIT_READ,
        0,
        (uint8 *) devInfoSystemId
      },

    // Model Number String Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &devInfoModelNumberProps
    },

      // Model Number Value
      {
        { ATT_BT_UUID_SIZE, devInfoModelNumberUUID },
        GATT_PERMIT_READ,
        0,
        (uint8 *) devInfoModelNumber
      },

    // Manufacturer Name String Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &devInfoMfrNameProps
    },

      // Manufacturer Name Value
      {
        { ATT_BT_UUID_SIZE, devInfoMfrNameUUID },
        GATT_PERMIT_READ,
        0,
        (uint8 *) devInfoMfrName
      },
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 devInfo_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Device Info Service Callbacks
CONST gattServiceCBs_t devInfoCBs =
{
  devInfo_ReadAttrCB, // Read callback function pointer
  NULL,               // Write callback function pointer
  NULL                // Authorization callback function pointer
};

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      DevInfo_AddService
 *
 * @brief   Initializes the Device Information service by registering
 *          GATT attributes with the GATT server.
 *
 * @return  Success or Failure
 */
bStatus_t DevInfo_AddService( void )
{
  // Register GATT attribute list and CBs with GATT Server App
  return GATTServApp_RegisterService( devInfoAttrTbl,
                                      GATT_NUM_ATTRS( devInfoAttrTbl ),
                                      &devInfoCBs );
}

/*********************************************************************
 * @fn      DevInfo_SetParameter
 *
 * @brief   Set a Device Information parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to write
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t DevInfo_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
     case DEVINFO_SYSTEM_ID:
      osal_memcpy(devInfoSystemId, value, len);
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }

  return ( ret );
}

/*********************************************************************
 * @fn      DevInfo_GetParameter
 *
 * @brief   Get a Device Information parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t DevInfo_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    case DEVINFO_SYSTEM_ID:
      osal_memcpy(value, devInfoSystemId, sizeof(devInfoSystemId));
      break;

    case DEVINFO_MODEL_NUMBER:
      osal_memcpy(value, devInfoModelNumber, sizeof(devInfoModelNumber));
      break;

    case DEVINFO_MANUFACTURER_NAME:
      osal_memcpy(value, devInfoMfrName, sizeof(devInfoMfrName));
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }

  return ( ret );
}

/*********************************************************************
 * @fn          devInfo_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 *
 * @return      Success or Failure
 */
static uint8 devInfo_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);

  switch (uuid)
  {
    case DEVINFO_SYSTEM_ID_UUID:
      // verify offset
      if (offset >= sizeof(devInfoSystemId))
      {
        status = ATT_ERR_INVALID_OFFSET;
      }
      else
      {
        // determine read length
        *pLen = MIN(maxLen, (sizeof(devInfoSystemId) - offset));

        // copy data
        osal_memcpy(pValue, &devInfoSystemId[offset], *pLen);
      }
      break;

    case DEVINFO_MODEL_NUMBER_UUID:
      // verify offset
      if (offset >= sizeof(devInfoModelNumber))
      {
        status = ATT_ERR_INVALID_OFFSET;
      }
      else
      {
        // determine read length
        *pLen = MIN(maxLen, (sizeof(devInfoModelNumber) - offset));

        // copy data
        osal_memcpy(pValue, &devInfoModelNumber[offset], *pLen);
      }
      break;

    case DEVINFO_MANUFACTURER_NAME_UUID:
      // verify offset
      if (offset >= sizeof(devInfoMfrName))
      {
        status = ATT_ERR_INVALID_OFFSET;
      }
      else
      {
        // determine read length
        *pLen = MIN(maxLen, (sizeof(devInfoMfrName) - offset));

        // copy data
        osal_memcpy(pValue, &devInfoMfrName[offset], *pLen);
      }
      break;

    default:
      *pLen = 0;
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  return ( status );
}


/*********************************************************************
*********************************************************************/
