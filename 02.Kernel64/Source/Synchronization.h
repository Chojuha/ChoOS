#ifndef _SYNCHRONIZATION_H_
#define _SYNCHRONIZATION_H_

#include "Types.h"

#pragma pack(push , 1)

typedef struct MutexStruct {
	volatile QWORD TaskID;
	volatile DWORD LockCount;
	volatile BOOL LockFlag;
	BYTE Padding[3];
}MUTEX;

typedef struct SpinLockStruct {
	volatile DWORD LockCount;
	volatile BYTE APICID;
	volatile BOOL LockFlag;
	volatile BOOL InterruptFlag;
	BYTE Padding[1];
}SPINLOCK;

#pragma pack(pop)

#if 0
BOOL LockFromSystemData(void);
BOOL UnLockFromSystemData(BOOL InterruptFlag);
#endif
void InitMutex(MUTEX *Mutex);
void Lock(MUTEX *Mutex);
void UnLock(MUTEX *Mutex);
void InitSpinLock(SPINLOCK *SpinLock);
void LockForSpinLock(SPINLOCK *SpinLock);
void UnLockForSpinLock(SPINLOCK *SpinLock);

#endif
