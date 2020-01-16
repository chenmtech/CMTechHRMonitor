
#ifndef CM_UTIL_H
#define CM_UTIL_H

#include "bcomdef.h"
#include "gatt.h"

// ��16λUUID����128λ��UUID
// Base UUID������0A20XXXX-CCE5-4025-A156-38EA833F6EF8
#define CM_UUID( uuid )  0xF8, 0x6E, 0x3F, 0x83, 0xEA, 0x38, 0x56, 0xA1, 0x25, 0x40, 0xE5, 0xCC, LO_UINT16(uuid), HI_UINT16(uuid), 0x20, 0x0A

// ����������ȡ16λUUID
extern bStatus_t utilExtractUuid16(gattAttribute_t *pAttr, uint16 *pValue);

extern void delayus(uint16 us); // ��ʱus







#endif

