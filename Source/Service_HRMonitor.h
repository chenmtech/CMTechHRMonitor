
#ifndef SERVICE_HRMONITOR_H
#define SERVICE_HRMONITOR_H


/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */


// Heart Rate Service Parameters
#define HRM_MEAS                      0
#define HRM_MEAS_CHAR_CFG             1
#define HRM_SENS_LOC                  2
#define HRM_COMMAND                   3

// Heart Rate Service UUIDs
#define HRM_SERV_UUID                 0x180D
#define HRM_MEAS_UUID                 0x2A37
#define HRM_SENS_LOC_UUID             0x2A38
#define HRM_COMMAND_UUID              0x2A39

// Maximum length of heart rate measurement characteristic
#define HRM_MEAS_MAX                  (ATT_MTU_SIZE -5)

// Values for flags
#define HRM_FLAGS_FORMAT_UINT16       0x01
#define HRM_FLAGS_CONTACT_NOT_SUP     0x00
#define HRM_FLAGS_CONTACT_NOT_DET     0x04
#define HRM_FLAGS_CONTACT_DET         0x06
#define HRM_FLAGS_ENERGY_EXP          0x08
#define HRM_FLAGS_RR                  0x10

// Values for sensor location
#define HRM_SENS_LOC_OTHER            0x00
#define HRM_SENS_LOC_CHEST            0x01
#define HRM_SENS_LOC_WRIST            0x02
#define HRM_SENS_LOC_FINGER           0x03
#define HRM_SENS_LOC_HAND             0x04
#define HRM_SENS_LOC_EARLOBE          0x05
#define HRM_SENS_LOC_FOOT             0x06

// Value for command characteristic
#define HRM_COMMAND_ENERGY_EXP        0x01

// ATT Error code
// Control point value not supported
#define HRM_ERR_NOT_SUP               0x80

// Heart Rate Service bit fields
#define HRM_SERVICE                   0x00000001

// Callback events
#define HRM_MEAS_NOTI_ENABLED         1
#define HRM_MEAS_NOTI_DISABLED        2
#define HRM_COMMAND_SET               3

/*********************************************************************
 * TYPEDEFS
 */

// Heart Rate Service callback function
typedef void (*HRMServiceCB_t)(uint8 event);

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */


/*********************************************************************
 * API FUNCTIONS 
 */

/*
 * HRM_AddService- Initializes the Heart Rate service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */

extern bStatus_t HRM_AddService( uint32 services );

/*
 * HRM_Register - Register a callback function with the
 *          Heart Rate Service
 *
 * @param   pfnServiceCB - Callback function.
 */

extern void HRM_Register( HRMServiceCB_t pfnServiceCB );

/*
 * HRM_SetParameter - Set a Heart Rate parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
extern bStatus_t HRM_SetParameter( uint8 param, uint8 len, void *value );
  
/*
 * HRM_GetParameter - Get a Heart Rate parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
extern bStatus_t HRM_GetParameter( uint8 param, void *value );

/*********************************************************************
 * @fn          HRM_MeasNotify
 *
 * @brief       Send a notification containing a heart rate
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
extern bStatus_t HRM_MeasNotify( uint16 connHandle, attHandleValueNoti_t *pNoti );

/*********************************************************************
 * @fn          HRM_HandleConnStatusCB
 *
 * @brief       Heart Rate Service link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
extern void HRM_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

/*********************************************************************
*********************************************************************/


#endif /* HRMSERVICE_H */
