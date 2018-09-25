#ifndef _PIC_H_
#define _PIC_H_

#include "Types.h"

#define PIC_MASTER_PORT1 0x20
#define PIC_MASTER_PORT2 0x21
#define PIC_SLAVE_PORT1 0xA0
#define PIC_SLAVE_PORT2 0xA1
#define PIC_IRQSTARTVECTOR 0x20

void InitPIC(void);
void MaskPICInterrupt(WORD IRQBitmask);
void SendEOIToPIC(int IRQNumber);

#endif
