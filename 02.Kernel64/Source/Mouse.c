#include "Mouse.h"
#include "Keyboard.h"
#include "Queue.h"
#include "AssemblyUtility.h"

static MOUSEMANAGER MouseManager = {0 , };

static QUEUE MouseQueue;
static MOUSEDATA MouseQueueBuffer[MOUSE_MAXQUEUECOUNT];

BOOL InitMouse(void) {
	InitQueue(&MouseQueue , MouseQueueBuffer , MOUSE_MAXQUEUECOUNT , sizeof(MOUSEDATA));
	InitSpinLock(&(MouseManager.SpinLock));
	if(ActivateMouse() == TRUE) {
		EnableMouseInterrupt();
		return TRUE;
	}
	return FALSE;
}

BOOL ActivateMouse(void) {
	int i;
	int j;
	BOOL PrevInterrupt;
	BOOL Result;
	PrevInterrupt = SetInterruptFlag(FALSE);
	OutPortByte(0x64 , 0xA8);
	OutPortByte(0x64 , 0xD4);
	for(i = 0; i < 0xFFFF; i++) {
		if(IsInputBufferFull() == FALSE) {
			break;
		}
	}
	OutPortByte(0x60 , 0xF4);
	Result = WaitForACKAndPutOtherScanCode();
	SetInterruptFlag(PrevInterrupt);
	return Result;
}

void EnableMouseInterrupt(void) {
	BYTE OutputPortData;
	int i;
	OutPortByte(0x64 , 0x20);
	for(i = 0; i < 0xFFFF; i++) {
		if(IsOutputBufferFull() == TRUE) {
			break;
		}
	}
	OutputPortData = InPortByte(0x60);
	OutputPortData |= 0x02;
	OutPortByte(0x64 , 0x60);
	for(i = 0; i < 0xFFFF; i++) {
		if(IsInputBufferFull() == FALSE) {
			break;
		}
	}
	OutPortByte(0x60 , OutputPortData);
}

BOOL AccumulateMouseDataAndPutQueue(BYTE MouseData) {
	BOOL Result;
	switch(MouseManager.ByteCount) {
		case 0:
			MouseManager.CurrentData.ButtonStatusAndFlag = MouseData;
			MouseManager.ByteCount++;
			break;
		case 1:
			MouseManager.CurrentData.XMovement = MouseData;
			MouseManager.ByteCount++;
			break;
		case 2:
			MouseManager.CurrentData.YMovement = MouseData;
			MouseManager.ByteCount++;
			break;
		default:
			MouseManager.ByteCount = 0;
			break;
	}
	if(MouseManager.ByteCount >= 3) {
		LockForSpinLock(&(MouseManager.SpinLock));
		Result = PutQueue(&MouseQueue , &MouseManager.CurrentData);
		UnLockForSpinLock(&(MouseManager.SpinLock));
		MouseManager.ByteCount = 0;
	}
	return Result;
}

BOOL GetMouseDataFromMouseQueue(BYTE *ButtonStatus , int *RelativeX , int *RelativeY) {
	MOUSEDATA Data;
	BOOL Result;
	if(IsQueueEmpty(&(MouseQueue)) == TRUE) {
		return FALSE;
	}
	LockForSpinLock(&(MouseManager.SpinLock));
	Result = GetQueue(&(MouseQueue) , &Data);
	UnLockForSpinLock(&(MouseManager.SpinLock));
	if(Result == FALSE) {
		return FALSE;
	}
	*ButtonStatus = Data.ButtonStatusAndFlag & 0x7;
	*RelativeX = Data.XMovement & 0xFF;
	if(Data.ButtonStatusAndFlag & 0x10) {
		*RelativeX |= (0xFFFFFF00);
	}
	*RelativeY = Data.YMovement & 0xFF;
	if(Data.ButtonStatusAndFlag & 0x20) {
		*RelativeY |= (0xFFFFFF00);
	}
	*RelativeY = -*RelativeY;
	return TRUE;
}

BOOL IsMouseDataInOutputBuffer(void) {
	if(InPortByte(0x64) & 0x20) {
		return TRUE;
	}
	return FALSE;
}
