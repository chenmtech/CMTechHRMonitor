/**
* ��ص��������ṩ��ص�������
*/

#ifndef SERVICE_BATTERY_H
#define SERVICE_BATTERY_H


// �������λ
#define BATTERY_DATA            0       //��ص�������
#define BATTERY_CTRL            1       //��ص�����������
#define BATTERY_PERI            2       //��ص�����������

// �����������16λUUID
#define BATTERY_SERV_UUID               0xAA90     // ��ص�������UUID
#define BATTERY_DATA_UUID               0xAA91     // ��ص�������UUID
#define BATTERY_CTRL_UUID               0xAA92     // ��ص�����������UUID
#define BATTERY_PERI_UUID               0xAA93     // ��ص�����������UUID

// �����bit field
#define BATTERY_SERVICE               0x00000001


// ��ҪӦ�ò��ṩ�Ļص���������
// �����������仯ʱ��֪ͨӦ�ò�
typedef NULL_OK void (*batteryServiceCB_t)( uint8 paramID );

// ��ҪӦ�ò��ṩ�Ļص��ṹ������
typedef struct
{
  batteryServiceCB_t        pfnBatteryServiceCB;
} batteryServiceCBs_t;


// ���ر�����
extern bStatus_t Battery_AddService( uint32 services );

// �Ǽ�Ӧ�ò��ṩ�Ļص��ṹ��ʵ��
extern bStatus_t Battery_RegisterAppCBs( batteryServiceCBs_t *appCallbacks );

// ������������
extern bStatus_t Battery_SetParameter( uint8 param, uint8 len, void *value );

// ��ȡ��������
extern bStatus_t Battery_GetParameter( uint8 param, void *value );

#endif












