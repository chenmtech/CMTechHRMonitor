/**
* 电池电量服务：提供电池电量测量
*/

#ifndef SERVICE_BATTERY_H
#define SERVICE_BATTERY_H


// 特征标记位
#define BATTERY_DATA            0       //电池电量数据
#define BATTERY_CTRL            1       //电池电量测量控制
#define BATTERY_PERI            2       //电池电量测量周期

// 服务和特征的16位UUID
#define BATTERY_SERV_UUID               0xAA90     // 电池电量服务UUID
#define BATTERY_DATA_UUID               0xAA91     // 电池电量数据UUID
#define BATTERY_CTRL_UUID               0xAA92     // 电池电量测量控制UUID
#define BATTERY_PERI_UUID               0xAA93     // 电池电量测量周期UUID

// 服务的bit field
#define BATTERY_SERVICE               0x00000001


// 需要应用层提供的回调函数声明
// 当特征发生变化时，通知应用层
typedef NULL_OK void (*batteryServiceCB_t)( uint8 paramID );

// 需要应用层提供的回调结构体声明
typedef struct
{
  batteryServiceCB_t        pfnBatteryServiceCB;
} batteryServiceCBs_t;


// 加载本服务
extern bStatus_t Battery_AddService( uint32 services );

// 登记应用层提供的回调结构体实例
extern bStatus_t Battery_RegisterAppCBs( batteryServiceCBs_t *appCallbacks );

// 设置特征参数
extern bStatus_t Battery_SetParameter( uint8 param, uint8 len, void *value );

// 读取特征参数
extern bStatus_t Battery_GetParameter( uint8 param, void *value );

#endif












