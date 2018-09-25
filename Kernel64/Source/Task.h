#ifndef _TASK_H_
#define _TASK_H_

#include "Types.h"
#include "List.h"
#include "Synchronization.h"

#define TASK_REGISTERCOUNT     ( 5 + 19 )
#define TASK_REGISTERSIZE       8
#define TASK_GSOFFSET           0
#define TASK_FSOFFSET           1
#define TASK_ESOFFSET           2
#define TASK_DSOFFSET           3
#define TASK_R15OFFSET          4
#define TASK_R14OFFSET          5
#define TASK_R13OFFSET          6
#define TASK_R12OFFSET          7
#define TASK_R11OFFSET          8
#define TASK_R10OFFSET          9
#define TASK_R9OFFSET           10
#define TASK_R8OFFSET           11
#define TASK_RSIOFFSET          12
#define TASK_RDIOFFSET          13
#define TASK_RDXOFFSET          14
#define TASK_RCXOFFSET          15
#define TASK_RBXOFFSET          16
#define TASK_RAXOFFSET          17
#define TASK_RBPOFFSET          18
#define TASK_RIPOFFSET          19
#define TASK_CSOFFSET           20
#define TASK_RFLAGSOFFSET       21
#define TASK_RSPOFFSET          22
#define TASK_SSOFFSET           23
#define TASK_TCBPOOLADDRESS     0x800000
#define TASK_MAXCOUNT           1024
#define TASK_STACKPOOLADDRESS   ( TASK_TCBPOOLADDRESS + sizeof( TCB ) * TASK_MAXCOUNT )
#define TASK_STACKSIZE          8192
#define TASK_INVALIDID          0xFFFFFFFFFFFFFFFF
#define TASK_PROCESSORTIME      5
#define TASK_MAXREADYLISTCOUNT  5
#define TASK_FLAGS_HIGHEST            0
#define TASK_FLAGS_HIGH               1
#define TASK_FLAGS_MEDIUM             2
#define TASK_FLAGS_LOW                3
#define TASK_FLAGS_LOWEST             4
#define TASK_FLAGS_WAIT               0xFF
#define TASK_FLAGS_ENDTASK            0x8000000000000000
#define TASK_FLAGS_SYSTEM             0x4000000000000000
#define TASK_FLAGS_PROCESS            0x2000000000000000
#define TASK_FLAGS_THREAD             0x1000000000000000
#define TASK_FLAGS_IDLE               0x0800000000000000
#define GETPRIORITY( x )        ( ( x ) & 0xFF )
#define SETPRIORITY( x, priority )  ( ( x ) = ( ( x ) & 0xFFFFFFFFFFFFFF00 ) | \
        ( priority ) )
#define GETTCBOFFSET( x )       ( ( x ) & 0xFFFFFFFF )
#define GETTCBFROMTHREADLINK( x )   ( TCB* ) ( ( QWORD ) ( x ) - offsetof( TCB, \
                                      ThreadLink ) )
#define TASK_LOADBALANCINGID    0xFF

#pragma pack(push , 1)

typedef struct ContextStruct {
	QWORD Register[TASK_REGISTERCOUNT];
}CONTEXT;

typedef struct TaskControlBlockStruct {
	LISTLINK Link;
	QWORD Flags;
	void *MemoryAddress;
	QWORD MemorySize;
	LISTLINK ThreadLink;
	QWORD ParentProcessID;
	QWORD FPUContext[512/8];
	LIST ChildThreadList;
	CONTEXT Context;
	void *StackAddress;
	QWORD StackSize;
	BOOL FPUUsed;
	BYTE Affinity;
	BYTE APICID;
	char Padding[9];
}TCB;

typedef struct TCBPoolManagerStruct {
	SPINLOCK SpinLock;
	TCB *StartAddress;
	int MaxCount;
	int UseCount;
	int AllocatedCount;
}TCBPOOLMANAGER;

typedef struct SchedulerStruct {
	SPINLOCK SpinLock;
	TCB *RunningTask;
	int ProcessorTime;
	LIST ReadyList[TASK_MAXREADYLISTCOUNT];
	LIST WaitList;
	int ExecuteCount[TASK_MAXREADYLISTCOUNT];
	QWORD ProcessorLoad;
	QWORD SpendProcessorTimeInIdleTask;
	QWORD LastFPUUsedTaskID;
	BOOL UseLoadBalancing;
}SCHEDULER;

#pragma pack(pop)

static void InitTCBPool(void);
static TCB *AllocateTCB(void);
static void FreeTCB(QWORD ID);
TCB *CreateTask(QWORD Flags , void *MemoryAddress , QWORD MemorySize , QWORD EntryPointAddress , BYTE Affinity);
static void SetUpTask(TCB *Tcb , QWORD Flags , QWORD EntryPointAddress , void *StackAddress , QWORD StackSize);
void InitScheduler(void);
void SetRunningTask(BYTE APICID , TCB *Task);
TCB *GetRunningTask(BYTE APICID);
static TCB *GetNextTaskToRun(BYTE APICID);
static BOOL AddTaskToReadyList(BYTE APICID , TCB *Task);
static TCB *RemoveTaskFromReadyList(BYTE APICID , QWORD TaskID);
static BOOL FindSchedulerOfTaskAndLock(QWORD TaskID , BYTE *APICID);
BOOL ChangePriority(QWORD TaskID , BYTE Priority);
BOOL Schedule(void);
BOOL ScheduleInInterrupt(void);
void DecreaseProcessorTime(BYTE APICID);
BOOL IsProcessorTimeExpired(BYTE APICID);
BOOL EndTask(QWORD TaskID);
void ExitTask(void);
int GetReadyTaskCount(BYTE APICID);
int GetTaskCount(BYTE APICID);
TCB *GetTCBInTCBPool(int Offset);
BOOL IsTaskExist(QWORD ID);
QWORD GetProcessorLoad(BYTE APICID);
static TCB *GetProcessByThread(TCB *Thread);
void AddTaskToSchedulerWithLoadBalancing(TCB *Task);
static BYTE FindSchedulerOfMinumumTaskCount(const TCB *Task);
BYTE SetTaskLoadBalancing(BYTE APICID , BOOL UseBalancing);
BOOL ChangeProcessorAffinity(QWORD TaskID , BYTE Affinity);
void IdleTask(void);
void HaltProcessorByLoad(BYTE APICID);
QWORD GetLastFPUUsedTaskID(BYTE APICID);
void SetLastFPUUsedTaskID(BYTE APICID , QWORD TaskID);

#endif
