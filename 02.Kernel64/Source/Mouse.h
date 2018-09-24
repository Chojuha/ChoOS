#ifndef _MOUSE_H_
#define _MOUSE_H_

#include "Types.h"
#include "Synchronization.h"

#define MOUSE_MAXQUEUECOUNT 100
#define MOUSE_LBUTTONDOWN 0x01
#define MOUSE_RBUTTONDOWN 0x02
#define MOUSE_MBUTTONDOWN 0x04

#pragma pack(push , 1)

typedef struct MousePacketStruct {
	BYTE ButtonStatusAndFlag;
	BYTE XMovement;
	BYTE YMovement;
}MOUSEDATA;

#pragma pack(pop)

typedef struct MouseManagerStruct {
	SPINLOCK SpinLock;
	int ByteCount;
	MOUSEDATA CurrentData;
}MOUSEMANAGER;

BOOL InitMouse(void);
BOOL ActivateMouse(void);
void EnableMouseInterrupt(void);
BOOL AccumulateMouseDataAndPutQueue(BYTE MouseData);
BOOL GetMouseDataFromMouseQueue(BYTE *ButtonStatus , int *RelativeX , int *RelativeY);
BOOL IsMouseDataInOutputBuffer(void);

#endif
