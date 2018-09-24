#include "PIC.h"

void InitPIC(void) {
	OutPortByte(PIC_MASTER_PORT1 , 0x11);
	OutPortByte(PIC_MASTER_PORT2 , PIC_IRQSTARTVECTOR);
	OutPortByte(PIC_MASTER_PORT2 , 0x04);
	OutPortByte(PIC_MASTER_PORT2 , 0x01);
	OutPortByte(PIC_SLAVE_PORT1 , 0x11);
	OutPortByte(PIC_SLAVE_PORT2 , PIC_IRQSTARTVECTOR+8);
	OutPortByte(PIC_SLAVE_PORT2 , 0x02);
	OutPortByte(PIC_SLAVE_PORT2 , 0x01);
}

void MaskPICInterrupt(WORD IRQBitmask) {
	OutPortByte(PIC_MASTER_PORT2 , (BYTE)IRQBitmask);
	OutPortByte(PIC_SLAVE_PORT2 , (BYTE)(IRQBitmask >> 8));
}

void SendEOIToPIC(int IRQNumber) {
	OutPortByte(PIC_MASTER_PORT1 , 0x20);
	if(IRQNumber >= 8) {
		OutPortByte(PIC_SLAVE_PORT1 , 0x20);
	}
}
