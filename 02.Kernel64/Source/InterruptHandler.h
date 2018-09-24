#ifndef _INTERRUPTHANDLER_H_
#define _INTERRUPTHANDLER_H_

#include "Types.h"
#include "MultiProcessor.h"

#define INTERRUPT_MAXVECTORCOUNT 16
#define INTERRUPT_LOADBALANCINGDIVIDOR 10

typedef struct InterruptManagerStruct {
	QWORD CoreInterruptCount[MAXPROCESSORCOUNT][INTERRUPT_MAXVECTORCOUNT];
	BOOL UseLoadBalancing;
	BOOL SymmetricIOMode;
}INTERRUPTMANAGER;

void SetSymmetricIOMode(BOOL SymmetricIOMode);
void SetInterruptLoadBalancing(BOOL UseLoadBalancing);
void IncreaseInterruptCount(int IRQ);
void SendEOI(int IRQ);
INTERRUPTMANAGER *GetInterruptManager(void);
void ProcessLoadBalancing(int IRQ);

void CommonExceptionHandler(int VectorNumber , QWORD ErrorCode);
void CommonInterruptHandler(int VectorNumber);
void KeyboardHandler(int VectorNumber);
void TimerHandler(int VectorNumber);
void DeviceNotAvailableHandler(int VectorNumber);
void HDDHandler(int VectorNumber);
void MouseHandler(int VectorNumber);

#endif
