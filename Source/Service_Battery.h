/**
* ��ص��������ṩ��ص�������
*/

#ifndef SERVICE_BATTERY_H
#define SERVICE_BATTERY_H


// �������
#define BATTERY_LEVEL           0       //��ص�������

#define BATTERY_LEVEL_POS   2

// �����������16λUUID
#define BATTERY_SERV_UUID                0x180F     // ��ص�������UUID
#define BATTERY_LEVEL_UUID               0x2A19     // ��ص�������UUID

// Callback events
#define BATTERY_LEVEL_NOTI_ENABLED         1
#define BATTERY_LEVEL_NOTI_DISABLED        2

// �����bit field
#define BATTERY_SERVICE               0x00000001


// ��ҪӦ�ò��ṩ�Ļص���������
// �����������仯ʱ��֪ͨӦ�ò�
typedef NULL_OK void (*batteryServiceCB_t)( uint8 event );

// ��ҪӦ�ò��ṩ�Ļص��ṹ������
typedef struct
{
  batteryServiceCB_t        pfnBatteryServiceCB;
} batteryServiceCBs_t;


// ���ر�����
extern bStatus_t Battery_AddService( uint32 services );

// �Ǽ�Ӧ�ò��ṩ�Ļص��ṹ��ʵ��
extern bStatus_t Battery_RegisterAppCBs( batteryServiceCBs_t *appCallbacks );

// ��ȡ��������
extern bStatus_t Battery_GetParameter( uint8 param, void *value );

// measure the battery level and send a notification on the connection handle.
extern bStatus_t Battery_MeasLevel( uint16 connHandle );

#endif












