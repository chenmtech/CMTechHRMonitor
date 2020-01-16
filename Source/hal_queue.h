
#ifndef HAL_QUEUE_H
#define HAL_QUEUE_H

#include "bcomdef.h"

#define QUEUE_L         48    // 队列容量
#define QUEUE_UNIT_L    10    // 单个队列元素的字节数

// 队列初始化
extern void Queue_Init();

// 队列添加一个元素
extern void Queue_Push(uint8* pData);

// 队列弹出一个元素
extern bool Queue_Pop(uint8* pData);

// 
extern bool Queue_GetDataAtTime(uint8* pData, uint8* pTime);




#endif