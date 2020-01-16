
#ifndef HAL_QUEUE_H
#define HAL_QUEUE_H

#include "bcomdef.h"

#define QUEUE_L         48    // ��������
#define QUEUE_UNIT_L    10    // ��������Ԫ�ص��ֽ���

// ���г�ʼ��
extern void Queue_Init();

// �������һ��Ԫ��
extern void Queue_Push(uint8* pData);

// ���е���һ��Ԫ��
extern bool Queue_Pop(uint8* pData);

// 
extern bool Queue_GetDataAtTime(uint8* pData, uint8* pTime);




#endif