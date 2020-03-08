/**
* battery service head file
*/

#ifndef SERVICE_BATTERY_H
#define SERVICE_BATTERY_H


// Mark for battery level
#define BATTERY_LEVEL 0     

// position of battery level characteristic in the attribute table 
#define BATTERY_LEVEL_POS   2

// UUID
#define BATTERY_SERV_UUID                0x180F     // battery service
#define BATTERY_LEVEL_UUID               0x2A19     // battery level

// Callback events
#define BATTERY_LEVEL_NOTI_ENABLED         1
#define BATTERY_LEVEL_NOTI_DISABLED        2

// service bit field
#define BATTERY_SERVICE               0x00000001

// the typedef of battery service callback function in the application
typedef NULL_OK void (*BattServiceCB_t)( uint8 event );

// the struct of battery service callback
typedef struct
{
  BattServiceCB_t        pfnBattServiceCB;
} BattServiceCBs_t;


// add battery service
extern bStatus_t Battery_AddService( uint32 services );

// register battery service callback struct in the application
extern bStatus_t Battery_RegisterAppCBs( BattServiceCBs_t *appCallbacks );

// get the characteristic parameter in battery service
extern bStatus_t Battery_GetParameter( uint8 param, void *value );

// measure the battery level and send a notification on the connection handle.
extern bStatus_t Battery_MeasLevel( uint16 connHandle );

#endif












