#include "Task.h"
#include "Descriptor.h"
#include "MultiProcessor.h"

static SCHEDULER Scheduler[MAXPROCESSORCOUNT];
static TCBPOOLMANAGER TCBPoolManager;

static void InitTCBPool(void) {
	int i;
	MemSet(&(TCBPoolManager) , 0 , sizeof(TCBPoolManager));
	TCBPoolManager.StartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	MemSet(TASK_TCBPOOLADDRESS , 0 , sizeof(TCB)*TASK_MAXCOUNT);
	for(i = 0; i < TASK_MAXCOUNT; i++) {
		TCBPoolManager.StartAddress[i].Link.ID = i;
	}
	TCBPoolManager.MaxCount = TASK_MAXCOUNT;
	TCBPoolManager.AllocatedCount = 1;
	InitSpinLock(&TCBPoolManager.SpinLock);
}

static TCB *AllocateTCB(void) {
	TCB *EmptyTCB;
	int i;
	LockForSpinLock(&TCBPoolManager.SpinLock);
	if(TCBPoolManager.UseCount == TCBPoolManager.MaxCount) {
		UnLockForSpinLock(&TCBPoolManager.SpinLock);
		return NULL;
	}
	for(i = 0; i < TCBPoolManager.MaxCount; i++) {
		if((TCBPoolManager.StartAddress[i].Link.ID >> 32) == 0) {
			EmptyTCB = &(TCBPoolManager.StartAddress[i]);
			break;
		}
	}
	EmptyTCB->Link.ID = ((QWORD)TCBPoolManager.AllocatedCount << 32)|i;
	TCBPoolManager.UseCount++;
	TCBPoolManager.AllocatedCount++;
	if(TCBPoolManager.AllocatedCount == 0) {
		TCBPoolManager.AllocatedCount = 1;
	}
	UnLockForSpinLock(&TCBPoolManager.SpinLock);
	return EmptyTCB;
}

static void FreeTCB(QWORD ID) {
	int i;
	i = GETTCBOFFSET(ID);
	MemSet(&(TCBPoolManager.StartAddress[i].Context) , 0 , sizeof(CONTEXT));
	LockForSpinLock(&TCBPoolManager.SpinLock);
	TCBPoolManager.StartAddress[i].Link.ID = i;
	TCBPoolManager.UseCount--;
	UnLockForSpinLock(&TCBPoolManager.SpinLock);
}

TCB *CreateTask(QWORD Flags , void *MemoryAddress , QWORD MemorySize , QWORD EntryPointAddress , BYTE Affinity) {
	TCB *Task;
	TCB *Process;
	void *StackAddress;
	BYTE CurrentAPICID;
	CurrentAPICID = GetAPICID();
	Task = AllocateTCB();
	if(Task == NULL) {
		return NULL;
	}
	LockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
	Process = GetProcessByThread(GetRunningTask(CurrentAPICID));
	if(Process == NULL) {
		FreeTCB(Task->Link.ID);
		UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
		return NULL;
	}
	if(Flags & TASK_FLAGS_THREAD) {
		Task->ParentProcessID = Process->Link.ID;
		Task->MemoryAddress = Process->MemoryAddress;
		Task->MemorySize = Process->MemorySize;
		AddListToTail(&(Process->ChildThreadList) , &(Task->ThreadLink));
	}
	else {
		Task->ParentProcessID = Process->Link.ID;
		Task->MemoryAddress = MemoryAddress;
		Task->MemorySize = MemorySize;
	}
	Task->ThreadLink.ID = Task->Link.ID;
	UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
	StackAddress = (void*)(TASK_STACKPOOLADDRESS+(TASK_STACKSIZE*GETTCBOFFSET(Task->Link.ID)));
	SetUpTask(Task , Flags , EntryPointAddress , StackAddress , TASK_STACKSIZE);
	InitList(&(Task->ChildThreadList));
	Task->FPUUsed = FALSE;
	Task->APICID = CurrentAPICID;
	Task->Affinity = Affinity;
	AddTaskToSchedulerWithLoadBalancing(Task);
	return Task;
}

static void SetUpTask(TCB *Tcb , QWORD Flags , QWORD EntryPointAddress , void *StackAddress , QWORD StackSize) {
	MemSet(Tcb->Context.Register , 0 , sizeof(Tcb->Context.Register));
	Tcb->Context.Register[TASK_RSPOFFSET] = (QWORD)StackAddress+StackSize-8;
	Tcb->Context.Register[TASK_RBPOFFSET] = (QWORD)StackAddress+StackSize-8;
	*(QWORD*)((QWORD)StackAddress+StackSize-8) = (QWORD)ExitTask;
	Tcb->Context.Register[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
	Tcb->Context.Register[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
	Tcb->Context.Register[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
	Tcb->Context.Register[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
	Tcb->Context.Register[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
	Tcb->Context.Register[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;
	Tcb->Context.Register[TASK_RIPOFFSET] = EntryPointAddress;
	Tcb->Context.Register[TASK_RFLAGSOFFSET] |= 0x0200;
	Tcb->StackAddress = StackAddress;
	Tcb->StackSize = StackSize;
	Tcb->Flags = Flags;
}

void InitScheduler(void) {
	int i;
	int j;
	BYTE CurrentAPICID;
	TCB *Task;
	CurrentAPICID = GetAPICID();
	if(CurrentAPICID == 0) {
		InitTCBPool();
		for(j = 0; j < TASK_MAXREADYLISTCOUNT; j++) {
			for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
				InitList(&(Scheduler[j].ReadyList[i]));
				Scheduler[j].ExecuteCount[i] = 0;
			}
			InitList(&(Scheduler[j].WaitList));
			InitSpinLock(&(Scheduler[j].SpinLock));
		}
	}
	Task = AllocateTCB();
	Scheduler[CurrentAPICID].RunningTask = Task;
	Task->APICID = CurrentAPICID;
	Task->Affinity = CurrentAPICID;
	if(CurrentAPICID == 0) {
		Task->Flags = TASK_FLAGS_HIGHEST|TASK_FLAGS_PROCESS|TASK_FLAGS_SYSTEM;
	}
	else {
		Task->Flags = TASK_FLAGS_LOWEST|TASK_FLAGS_PROCESS|TASK_FLAGS_SYSTEM|TASK_FLAGS_IDLE;
	}
	Task->ParentProcessID = Task->Link.ID;
	Task->MemoryAddress = (void*)0x100000;
	Task->MemorySize = 0x500000;
	Task->StackAddress = (void*)0x600000;
	Task->StackSize = 0x100000;
	Scheduler[CurrentAPICID].SpendProcessorTimeInIdleTask = 0;
	Scheduler[CurrentAPICID].ProcessorLoad = 0;
	Scheduler[CurrentAPICID].LastFPUUsedTaskID = TASK_INVALIDID;
}

void SetRunningTask(BYTE APICID , TCB *Task) {
	LockForSpinLock(&(Scheduler[APICID].SpinLock));
	Scheduler[APICID].RunningTask = Task;
	UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
}

TCB *GetRunningTask(BYTE APICID) {
	TCB *RunningTask;
	LockForSpinLock(&(Scheduler[APICID].SpinLock));
	RunningTask = Scheduler[APICID].RunningTask;
	UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
	return RunningTask;
}

static TCB *GetNextTaskToRun(BYTE APICID) {
	TCB *Target = NULL;
	int TaskCount;
	int i;
	int j;
	for(j = 0; j < 2; j++) {
		for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
			TaskCount = GetListCount(&(Scheduler[APICID].ReadyList[i]));
			if(Scheduler[APICID].ExecuteCount[i] < TaskCount) {
				Target = (TCB*)RemoveListFromHeader(&(Scheduler[APICID].ReadyList[i]));
				Scheduler[APICID].ExecuteCount[i]++;
				break;
			}
			else {
				Scheduler[APICID].ExecuteCount[i] = 0;
			}
		}
		if(Target != NULL) {
			break;
		}
	}
	return Target;
}

static BOOL AddTaskToReadyList(BYTE APICID , TCB *Task) {
	BOOL Priority;
	Priority = GETPRIORITY(Task->Flags);
	if(Priority == TASK_FLAGS_WAIT) {
		AddListToTail(&(Scheduler[APICID].WaitList) , Task);
		return TRUE;
	}
	if(Priority >= TASK_MAXREADYLISTCOUNT) {
		return FALSE;
	}
	AddListToTail(&(Scheduler[APICID].ReadyList[Priority]) , Task);
	return TRUE;
}

static TCB *RemoveTaskFromReadyList(BYTE APICID , QWORD TaskID) {
	TCB *Target;
	BYTE Priority;
	if(GETTCBOFFSET(TaskID) >= TASK_MAXCOUNT) {
		return NULL;
	}
	Target = &(TCBPoolManager.StartAddress[GETTCBOFFSET(TaskID)]);
	if(Target->Link.ID != TaskID) {
		return NULL;
	}
	Priority = GETPRIORITY(Target->Flags);
	if(Priority >= TASK_MAXREADYLISTCOUNT) {
		return NULL;
	}
	Target = RemoveList(&(Scheduler[APICID].ReadyList[Priority]) , TaskID);
	return Target;
}

static BOOL FindSchedulerOfTaskAndLock(QWORD TaskID , BYTE *APICID) {
	TCB *Target;
	BYTE bAPICID;
	while(1) {
		Target = &(TCBPoolManager.StartAddress[GETTCBOFFSET(TaskID)]);
		if((Target == NULL)||(Target->Link.ID != TaskID)) {
			return FALSE;
		}
		bAPICID = Target->APICID;
		LockForSpinLock(&(Scheduler[bAPICID].SpinLock));
		Target = &(TCBPoolManager.StartAddress[GETTCBOFFSET(TaskID)]);
		if(Target->APICID == bAPICID) {
			break;
		}
		UnLockForSpinLock(&(Scheduler[bAPICID].SpinLock));
	}
	*APICID = bAPICID;
	return TRUE;
}

BOOL ChangePriority(QWORD TaskID , BYTE Priority) {
	TCB *Target;
	BYTE APICID;
	if(Priority > TASK_MAXREADYLISTCOUNT) {
		return FALSE;
	}
	if(FindSchedulerOfTaskAndLock(TaskID , &APICID) == FALSE) {
		return FALSE;
	}
	Target = Scheduler[APICID].RunningTask;
	if(Target->Link.ID == TaskID) {
		SETPRIORITY(Target->Flags , Priority);
	}
	else {
		Target = RemoveTaskFromReadyList(APICID , TaskID);
		if(Target == NULL) {
			Target = GetTCBInTCBPool(GETTCBOFFSET(TaskID));
			if(Target != NULL) {
				SETPRIORITY(Target->Flags , Priority);
			}
		}
		else {
			SETPRIORITY(Target->Flags , Priority);
			AddTaskToReadyList(APICID , Target);
		}
	}
	UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
	return TRUE;
}

BOOL Schedule(void) {
	TCB *RunningTask;
	TCB *NextTask;
	BOOL PrevInterrupt;
	BYTE CurrentAPICID;
	PrevInterrupt = SetInterruptFlag(FALSE);
	CurrentAPICID = GetAPICID();
	if(GetReadyTaskCount(CurrentAPICID) < 1) {
		SetInterruptFlag(PrevInterrupt);
		return FALSE;
	}
	LockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
	NextTask = GetNextTaskToRun(CurrentAPICID);
	if(NextTask == NULL) {
		UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
		SetInterruptFlag(PrevInterrupt);
		return FALSE;
	}
	RunningTask = Scheduler[CurrentAPICID].RunningTask;
	Scheduler[CurrentAPICID].RunningTask = NextTask;
	if((RunningTask->Flags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
		Scheduler[CurrentAPICID].SpendProcessorTimeInIdleTask += TASK_PROCESSORTIME-Scheduler[CurrentAPICID].ProcessorTime;
	}
	if(Scheduler[CurrentAPICID].LastFPUUsedTaskID != NextTask->Link.ID) {
		SetTS();
	}
	else {
		ClearTS();
	}
	if(RunningTask->Flags & TASK_FLAGS_ENDTASK) {
		AddListToTail(&(Scheduler[CurrentAPICID].WaitList) , RunningTask);
		UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
		SwitchContext(NULL , &(NextTask->Context));
	}
	else {
		AddTaskToReadyList(CurrentAPICID , RunningTask);
		UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
		SwitchContext(&(RunningTask->Context) , &(NextTask->Context));
	}
	Scheduler[CurrentAPICID].ProcessorTime = TASK_PROCESSORTIME;
	SetInterruptFlag(PrevInterrupt);
	return FALSE;
}

BOOL ScheduleInInterrupt(void) {
	TCB *RunningTask;
	TCB *NextTask;
	char *ContextAddress;
	BYTE CurrentAPICID;
	QWORD ISTStartAddress;
	CurrentAPICID = GetAPICID();
	LockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
	NextTask = GetNextTaskToRun(CurrentAPICID);
	if(NextTask == NULL) {
		UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
		return FALSE;
	}
	ISTStartAddress = IST_STARTADDRESS+IST_SIZE-(IST_SIZE/MAXPROCESSORCOUNT*CurrentAPICID);
	ContextAddress = (char*)ISTStartAddress-sizeof(CONTEXT);
	RunningTask = Scheduler[CurrentAPICID].RunningTask;
	Scheduler[CurrentAPICID].RunningTask = NextTask;
	if((RunningTask->Flags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
		Scheduler[CurrentAPICID].SpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}
	if(RunningTask->Flags & TASK_FLAGS_ENDTASK) {
		AddListToTail(&(Scheduler[CurrentAPICID].WaitList) , RunningTask);
	}
	else {
		MemCpy(&(RunningTask->Context) , ContextAddress , sizeof(CONTEXT));
	}
	if(Scheduler[CurrentAPICID].LastFPUUsedTaskID != NextTask->Link.ID) {
		SetTS();
	}
	else {
		ClearTS();
	}
	UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
	MemCpy(ContextAddress , &(NextTask->Context) , sizeof(CONTEXT));
	if((RunningTask->Flags & TASK_FLAGS_ENDTASK) != TASK_FLAGS_ENDTASK) {
		AddTaskToSchedulerWithLoadBalancing(RunningTask);
	}
	Scheduler[CurrentAPICID].ProcessorTime = TASK_PROCESSORTIME;
	return TRUE;
}

void DecreaseProcessorTime(BYTE APICID) {
	Scheduler[APICID].ProcessorTime--;
}

BOOL IsProcessorTimeExpired(BYTE APICID) {
	if(Scheduler[APICID].ProcessorTime <= 0) {
		return TRUE;
	}
	return FALSE;
}

BOOL EndTask(QWORD TaskID) {
	TCB *Target;
	BYTE Priority;
	BYTE APICID;
	if(FindSchedulerOfTaskAndLock(TaskID , &APICID) == FALSE) {
		return FALSE;
	}
	Target = Scheduler[APICID].RunningTask;
	if(Target->Link.ID == TaskID) {
		Target->Flags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(Target->Flags , TASK_FLAGS_WAIT);
		UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
		if(GetAPICID() == APICID) {
			Schedule();
			while(1) {
				;
			}
		}
		return TRUE;
	}
	Target = RemoveTaskFromReadyList(APICID , TaskID);
	if(Target == NULL) {
		Target = GetTCBInTCBPool(GETTCBOFFSET(TaskID));
		if(Target != NULL) {
			Target->Flags |= TASK_FLAGS_ENDTASK;
			SETPRIORITY(Target->Flags , TASK_FLAGS_WAIT);
		}
		UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
		return TRUE;
	}
	Target->Flags |= TASK_FLAGS_ENDTASK;
	SETPRIORITY(Target->Flags , TASK_FLAGS_WAIT);
	AddListToTail(&(Scheduler[APICID].WaitList) , Target);
	UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
	return TRUE;
}

void ExitTask(void) {
	EndTask(Scheduler[GetAPICID()].RunningTask->Link.ID);
}

int GetReadyTaskCount(BYTE APICID) {
	int TotalCount = 0;
	int i;
	LockForSpinLock(&(Scheduler[APICID].SpinLock)); 
	for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
		TotalCount += GetListCount(&(Scheduler[APICID].ReadyList[i]));
	}
	UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
	return TotalCount;
}

int GetTaskCount(BYTE APICID) {
	int TotalCount;
	TotalCount = GetReadyTaskCount(APICID);
	LockForSpinLock(&(Scheduler[APICID].SpinLock));
	TotalCount += GetListCount(&(Scheduler[APICID].WaitList))+1;
	UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
	return TotalCount;
}

TCB *GetTCBInTCBPool(int Offset) {
	if((Offset < -1) && (Offset > TASK_MAXCOUNT)) {
		return NULL;
	}
	return &(TCBPoolManager.StartAddress[Offset]);
}

BOOL IsTaskExist(QWORD ID) {
	TCB *Tcb;
	Tcb = GetTCBInTCBPool(GETTCBOFFSET(ID));
	if((Tcb == NULL)||(Tcb->Link.ID != ID)) {
		return FALSE;
	}
	return TRUE;
}

QWORD GetProcessorLoad(BYTE APICID) {
	return Scheduler[APICID].ProcessorLoad;
}

static TCB *GetProcessByThread(TCB *Thread) {
	TCB *Process;
	if(Thread->Flags & TASK_FLAGS_PROCESS) {
		return Thread;
	}
	Process = GetTCBInTCBPool(GETTCBOFFSET(Thread->ParentProcessID));
	if((Process == NULL)||(Process->Link.ID != Thread->ParentProcessID)) {
		return NULL;
	}
	return Process;
}

void AddTaskToSchedulerWithLoadBalancing(TCB *Task) {
	BYTE CurrentAPICID;
	BYTE TargetAPICID;
	CurrentAPICID = Task->APICID;
	if((Scheduler[CurrentAPICID].UseLoadBalancing == TRUE) && (Task->Affinity == TASK_LOADBALANCINGID)) {
		TargetAPICID = FindSchedulerOfMinumumTaskCount(Task);
	}
	else if((Task->Affinity != CurrentAPICID) && (Task->Affinity != TASK_LOADBALANCINGID)) {
		TargetAPICID = Task->Affinity;
	}
	else {
		TargetAPICID = CurrentAPICID;
	}
	LockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
	if((CurrentAPICID != TargetAPICID) && (Task->Link.ID == Scheduler[CurrentAPICID].LastFPUUsedTaskID)) {
		ClearTS();
		SaveFPUContext(Task->FPUContext);
		Scheduler[CurrentAPICID].LastFPUUsedTaskID = TASK_INVALIDID;
	}
	UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
	LockForSpinLock(&(Scheduler[TargetAPICID].SpinLock));
	Task->APICID = TargetAPICID;
	AddTaskToReadyList(TargetAPICID , Task);
	UnLockForSpinLock(&(Scheduler[TargetAPICID].SpinLock));
}

static BYTE FindSchedulerOfMinumumTaskCount(const TCB *Task) {
	BYTE Priority;
	BYTE i;
	int CurrentTaskCount;
	int MinTaskCount;
	BYTE MinCoreIndex;
	int TempTaskCount;
	int ProcessorCount;
	ProcessorCount = GetProcessorCount();
	if(ProcessorCount == 1) {
		return Task->APICID;
	}
	Priority = GETPRIORITY(Task->Flags);
	CurrentTaskCount = GetListCount(&(Scheduler[Task->APICID].ReadyList[Priority]));
	MinTaskCount = TASK_MAXCOUNT;
	MinCoreIndex = Task->APICID;
	for(i = 0; i < ProcessorCount; i++) {
		if(i == Task->APICID) {
			continue;
		}
		TempTaskCount = GetListCount(&(Scheduler[i].ReadyList[Priority]));
		if((TempTaskCount+2 <= CurrentTaskCount) && (TempTaskCount < MinTaskCount)) {
			MinCoreIndex = i;
			MinTaskCount = TempTaskCount;
		}
	}
	return MinCoreIndex;
}

BYTE SetTaskLoadBalancing(BYTE APICID , BOOL UseBalancing) {
	Scheduler[APICID].UseLoadBalancing = UseBalancing;
}

BOOL ChangeProcessorAffinity(QWORD TaskID , BYTE Affinity) {
	TCB *Target;
	BYTE APICID;
	if(FindSchedulerOfTaskAndLock(TaskID , &APICID) == FALSE) {
		return FALSE;
	}
	Target = Scheduler[APICID].RunningTask;
	if(Target->Link.ID == TaskID) {
		Target->Affinity = Affinity;
		UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
	}
	else {
		Target = RemoveTaskFromReadyList(APICID , TaskID);
		if(Target == NULL) {
			Target = GetTCBInTCBPool(GETTCBOFFSET(TaskID));
			if(Target != NULL) {
				Target->Affinity = Affinity;
			}
		}
		else {
			Target->Affinity = Affinity;
		}
		UnLockForSpinLock(&(Scheduler[APICID].SpinLock));
		AddTaskToSchedulerWithLoadBalancing(Target);
	}
	return TRUE;
}

void IdleTask(void) {
	TCB *Task;
	TCB *ChildThread;
	TCB *Process;
	QWORD LastTickCount;
	QWORD LastMeasureTickCount;
	QWORD LastSpendTickInIdleTask;
	QWORD CurrentMeasureTickCount;
	QWORD CurrentSpendTickInIdleTask;
	int i;
	int Count;
	QWORD TaskID;
	QWORD ChildThreadID;
	void *ThreadLink;
	BYTE CurrentAPICID;
	BYTE ProcessAPICID;
	CurrentAPICID = GetAPICID();
	LastSpendTickInIdleTask = Scheduler[CurrentAPICID].SpendProcessorTimeInIdleTask;
	LastMeasureTickCount = GetTickCount();
	while(1) {
		CurrentMeasureTickCount  = GetTickCount();
		CurrentSpendTickInIdleTask = Scheduler[CurrentAPICID].SpendProcessorTimeInIdleTask;
		if(CurrentMeasureTickCount-LastMeasureTickCount == 0) {
			Scheduler[CurrentAPICID].ProcessorLoad = 0;
		}
		else {
			Scheduler[CurrentAPICID].ProcessorLoad = 100-(CurrentSpendTickInIdleTask-LastSpendTickInIdleTask)*100/(CurrentMeasureTickCount-LastMeasureTickCount);
		}
		LastMeasureTickCount = CurrentMeasureTickCount;
		LastSpendTickInIdleTask = CurrentSpendTickInIdleTask;
		HaltProcessorByLoad(CurrentAPICID);
		if(GetListCount(&(Scheduler[CurrentAPICID].WaitList)) > 0) {
			while(1) {
				LockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
				Task = RemoveListFromHeader(&(Scheduler[CurrentAPICID].WaitList));
				UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
				if(Task == NULL) {
					break;
				}
				if(Task->Flags & TASK_FLAGS_PROCESS) {
					Count = GetListCount(&(Task->ChildThreadList));
					for(i = 0; i < Count; i++) {
						LockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
						ThreadLink = (TCB*)RemoveListFromHeader(&(Task->ChildThreadList));
						if(ThreadLink == NULL) {
							UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
							break;
						}
						ChildThread = GETTCBFROMTHREADLINK(ThreadLink);
						AddListToTail(&(Task->ChildThreadList) , &(ChildThread->ThreadLink));
						ChildThreadID = ChildThread->Link.ID;
						UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
						EndTask(ChildThreadID);
					}
					if(GetListCount(&(Task->ChildThreadList)) > 0) {
						LockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
						AddListToTail(&(Scheduler[CurrentAPICID].WaitList) , Task);
						UnLockForSpinLock(&(Scheduler[CurrentAPICID].SpinLock));
						continue;
					}
					else {
						;
					}
				}
				else if(Task->Flags & TASK_FLAGS_THREAD) {
					Process = GetProcessByThread(Task);
					if(Process != NULL) {
						if(FindSchedulerOfTaskAndLock(Process->Link.ID , &ProcessAPICID) == TRUE) {
							RemoveList(&(Process->ChildThreadList) , Task->Link.ID);
							UnLockForSpinLock(&(Scheduler[ProcessAPICID].SpinLock));
						}
					}
				}
				TaskID = Task->Link.ID;
				FreeTCB(Task->Link.ID);
			}
		}
		Schedule();
	}
}

void HaltProcessorByLoad(BYTE APICID) {
	if(Scheduler[APICID].ProcessorLoad < 40) {
		Hlt();
		Hlt();
		Hlt();
	}
	else if(Scheduler[APICID].ProcessorLoad < 80) {
		Hlt();
		Hlt();
	}
	else if(Scheduler[APICID].ProcessorLoad < 95) {
		Hlt();
	}
}

QWORD GetLastFPUUsedTaskID(BYTE APICID) {
	return Scheduler[APICID].LastFPUUsedTaskID;
}

void SetLastFPUUsedTaskID(BYTE APICID , QWORD TaskID) {
	Scheduler[APICID].LastFPUUsedTaskID = TaskID;
}
