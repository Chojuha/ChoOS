#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"

#if 0

BOOL LockFromSystemData(void) {
	return SetInterruptFlag(FALSE);
}

BOOL UnLockFromSystemData(BOOL InterruptFlag) {
	return SetInterruptFlag(InterruptFlag);
}

#endif

void InitMutex(MUTEX *Mutex) {
	Mutex->LockFlag = FALSE;
	Mutex->LockCount = 0;
	Mutex->TaskID = TASK_INVALIDID;
}

void Lock(MUTEX *Mutex) {
	BYTE CurrentAPICID;
	BOOL InterruptFlag;
	InterruptFlag = SetInterruptFlag(FALSE);
	CurrentAPICID = GetAPICID();
	if(TestAndSet(&(Mutex->LockFlag) , 0 , 1) == FALSE) {
		if(Mutex->TaskID == GetRunningTask(CurrentAPICID)->Link.ID) {
			SetInterruptFlag(InterruptFlag);
			Mutex->LockCount++;
			return;
		}
		while(TestAndSet(&(Mutex->LockFlag) , 0 , 1) == FALSE) {
			Schedule();
		}
	}
	Mutex->LockCount = 1;
	Mutex->TaskID = GetRunningTask(CurrentAPICID)->Link.ID;
	SetInterruptFlag(InterruptFlag);
}

void UnLock(MUTEX *Mutex) {
	BOOL InterruptFlag;
	InterruptFlag = SetInterruptFlag(FALSE);
	if((Mutex->LockFlag == FALSE)||(Mutex->TaskID != GetRunningTask(GetAPICID())->Link.ID)) {
		SetInterruptFlag(InterruptFlag);
		return;
	}
	if(Mutex->LockCount > 1) {
		Mutex->LockCount--;
		return;
	}
	else {
		Mutex->TaskID = TASK_INVALIDID;
		Mutex->LockCount = 0;
		Mutex->LockFlag = FALSE;
	}
	SetInterruptFlag(InterruptFlag);
}

void InitSpinLock(SPINLOCK *SpinLock) {
	SpinLock->LockFlag = FALSE;
	SpinLock->LockCount = 0;
	SpinLock->APICID = 0xFF;
	SpinLock->InterruptFlag = FALSE;
}

void LockForSpinLock(SPINLOCK *SpinLock) {
	BOOL InterruptFlag;
	InterruptFlag = SetInterruptFlag(FALSE);
	if(TestAndSet(&(SpinLock->LockFlag) , 0 , 1) == FALSE) {
		if(SpinLock->APICID == GetAPICID()) {
			SpinLock->LockCount++;
			return;
		}
		while(TestAndSet(&(SpinLock->LockFlag) , 0 , 1) == FALSE) {
			while(SpinLock->LockFlag == TRUE) {
				Pause();
			}
		}
	}
	SpinLock->LockCount = 1;
	SpinLock->APICID = GetAPICID();
	SpinLock->InterruptFlag = InterruptFlag;
}

void UnLockForSpinLock(SPINLOCK *SpinLock) {
	BOOL InterruptFlag;
	InterruptFlag = SetInterruptFlag(FALSE);
	if((SpinLock->LockFlag == FALSE)||(SpinLock->APICID != GetAPICID())) {
		SetInterruptFlag(InterruptFlag);
		return;
	}
	if(SpinLock->LockCount > 1) {
		SpinLock->LockCount--;
		return;
	}
	InterruptFlag = SpinLock->InterruptFlag;
	SpinLock->APICID = 0xFF;
	SpinLock->LockCount = 0;
	SpinLock->InterruptFlag = FALSE;
	SpinLock->LockFlag = FALSE;
	SetInterruptFlag(InterruptFlag);
}
