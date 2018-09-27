#include "PIT.h"

void InitPIT(WORD Count , BOOL Periodic) {
	OutPortByte(PIT_PORT_CONTROL , PIT_COUNTER0_ONCE);
	if(Periodic == TRUE) {
		OutPortByte(PIT_PORT_CONTROL , PIT_COUNTER0_PERIODIC);
	}
	OutPortByte(PIT_PORT_COUNTER0 , Count);
	OutPortByte(PIT_PORT_COUNTER0 , Count >> 8);	
}

WORD ReadCounter0(void) {
	BYTE HByte;
	BYTE LByte;
	WORD Tmp = 0;
	OutPortByte(PIT_PORT_CONTROL , PIT_COUNTER0_LATCH);
	LByte = InPortByte(PIT_PORT_COUNTER0);
	HByte = InPortByte(PIT_PORT_COUNTER0);
	Tmp = HByte;
	Tmp = (Tmp << 8)|LByte;
	return Tmp;
}

void WaitUsingDirectPIT(WORD Count) {
	WORD LCounter0;
	WORD CCounter0;
	InitPIT(0 , TRUE);
	while(1) {
		CCounter0 = ReadCounter0();
		if(((LCounter0-CCounter0) & 0xFFFF) >= Count) {
			break;
		}
	}
}
