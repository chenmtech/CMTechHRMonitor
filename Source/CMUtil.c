

#include "CMUtil.h"

// 从属性中提取16位UUID
extern bStatus_t utilExtractUuid16(gattAttribute_t *pAttr, uint16 *pUuid)
{
  bStatus_t status = SUCCESS;

  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID direct
    *pUuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
  }
  else if (pAttr->type.len == ATT_UUID_SIZE)
  {
    // 16-bit UUID extracted bytes 12 and 13
    *pUuid = BUILD_UINT16( pAttr->type.uuid[12], pAttr->type.uuid[13]);
  } 
  else {
    *pUuid = 0xFFFF;
    status = FAILURE;
  }

  return status;
}

//延时us
extern void delayus(uint16 us)
{
  while(us--)
  {
    /* 32 NOPs == 1 usecs */
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop");
  }
}









