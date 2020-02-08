/**
* 电池电量服务：提供电池电量测量
*/

#ifndef SERVICE_BATTERY_H
#define SERVICE_BATTERY_H


// 特征标记
#define BATTERY_LEVEL           0       //电池电量数据

#define BATTERY_LEVEL_POS   2

// 服务和特征的16位UUID
#define BATTERY_SERV_UUID                0x180F     // 电池电量服务UUID
#define BATTERY_LEVEL_UUID               0x2A19     // 电池电量数据UUID

// Callback events
#define BATTERY_LEVEL_NOTI_ENABLED         1
#define BATTERY_LEVEL_NOTI_DISABLED        2

// 服务的bit field
#define BATTERY_SERVICE               0x00000001


// 需要应用层提供的回调函数声明
// 当特征发生变化时，通知应用层
typedef NULL_OK void (*batteryServiceCB_t)( uint8 event );

// 需要应用层提供的回调结构体声明
typedef struct
{
  batteryServiceCB_t        pfnBatteryServiceCB;
} batteryServiceCBs_t;


// 加载本服务
extern bStatus_t Battery_AddService( uint32 services );

// 登记应用层提供的回调结构体实例
extern bStatus_t Battery_RegisterAppCBs( batteryServiceCBs_t *appCallbacks );

// 读取特征参数
extern bStatus_t Battery_GetParameter( uint8 param, void *value );

// measure the battery level and send a notification on the connection handle.
extern bStatus_t Battery_MeasLevel( uint16 connHandle );

#endif












