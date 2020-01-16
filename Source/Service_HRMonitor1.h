/**
* �Զ��廷����ʪ�ȷ����ṩʵʱ��ʪ�Ȳ�������
*/

#ifndef SERVICE_HRM_H
#define SERVICE_HRM_H

 
// ��ʪ�ȷ������
#define HRM_DATA                   0       //ʵʱ��ʪ������
#define HRM_DATA_CHAR_CFG          1       //ʵʱ����CCC
#define HRM_INTERVAL               2       //ʵʱ����ʱ����
#define HRM_IRANGE                 3       //ʵʱ���������Χ

// ��ʪ������CCC�����Ա��е�λ��
#define HRM_DATA_CHAR_CFG_POS      3

// ���������ֵ��16λUUID
#define HRM_SERV_UUID               0xAA60     // ��ʪ�ȷ���UUID
#define HRM_DATA_UUID               0xAA61     // ʵʱ��ʪ������UUID
#define HRM_INTERVAL_UUID           0x2A21     // ʵʱ����ʱ����UUID
#define HRM_IRANGE_UUID             0x2906     // ʵʱ���������ΧUUID

// �����bit field
#define HRM_SERVICE               0x00000001

// ��ʪ�����ݵ��ֽڳ���
#define HRM_DATA_LEN             4       

// �ص��¼�
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


// ��ĳ���¼�����ʱ���õĻص���������
typedef void (*HRMServiceCB_t)( uint8 event );


// ��Ӧ�ò��ṩ�Ļص��ṹ������
typedef struct
{
  HRMServiceCB_t pfnHRMServiceCB;
} HRMServiceCBs_t;


// ���ر�����
extern bStatus_t HRM_AddService( uint32 services );

// �Ǽ�Ӧ�ò��ṩ�Ļص��ṹ��ʵ��
extern bStatus_t HRM_RegisterAppCBs( HRMServiceCBs_t *appCallbacks );

extern void HRM_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

// ������������
extern bStatus_t HRM_SetParameter( uint8 param, uint8 len, void *value );

// ��ȡ��������
extern bStatus_t HRM_GetParameter( uint8 param, void *value );

// indicateʵʱ��ʪ������
extern bStatus_t HRM_HRIndicate( uint16 connHandle, int16 temp, uint16 humid, uint8 taskId );

#endif












