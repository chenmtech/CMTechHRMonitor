/**
* 自定义环境温湿度服务：提供实时温湿度测量数据
*/

#ifndef SERVICE_HRM_H
#define SERVICE_HRM_H

 
// 温湿度服务参数
#define HRM_DATA                   0       //实时温湿度数据
#define HRM_DATA_CHAR_CFG          1       //实时测量CCC
#define HRM_INTERVAL               2       //实时测量时间间隔
#define HRM_IRANGE                 3       //实时测量间隔范围

// 温湿度数据CCC在属性表中的位置
#define HRM_DATA_CHAR_CFG_POS      3

// 服务和特征值的16位UUID
#define HRM_SERV_UUID               0xAA60     // 温湿度服务UUID
#define HRM_DATA_UUID               0xAA61     // 实时温湿度数据UUID
#define HRM_INTERVAL_UUID           0x2A21     // 实时测量时间间隔UUID
#define HRM_IRANGE_UUID             0x2906     // 实时测量间隔范围UUID

// 服务的bit field
#define HRM_SERVICE               0x00000001

// 温湿度数据的字节长度
#define HRM_DATA_LEN             4       

// 回调事件
#define HRM_DATA_IND_ENABLED          1
#define HRM_DATA_IND_DISABLED         2
#define HRM_INTERVAL_SET              3

/**
 * Thermometer Interval Range 
 */
typedef struct
{
  uint16 low;         
  uint16 high; 
} HRMIRange_t;


// 当某个事件发生时调用的回调函数声明
typedef void (*HRMServiceCB_t)( uint8 event );


// 由应用层提供的回调结构体声明
typedef struct
{
  HRMServiceCB_t pfnHRMServiceCB;
} HRMServiceCBs_t;


// 加载本服务
extern bStatus_t HRM_AddService( uint32 services );

// 登记应用层提供的回调结构体实例
extern bStatus_t HRM_RegisterAppCBs( HRMServiceCBs_t *appCallbacks );

extern void HRM_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// 设置特征参数
extern bStatus_t HRM_SetParameter( uint8 param, uint8 len, void *value );

// 读取特征参数
extern bStatus_t HRM_GetParameter( uint8 param, void *value );

// indicate实时温湿度数据
extern bStatus_t HRM_HRIndicate( uint16 connHandle, int16 temp, uint16 humid, uint8 taskId );

#endif












