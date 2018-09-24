#include "InterruptHandler.h"
#include "PIC.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"
#include "Mouse.h"
#include "VBE.h"
#include "2DGraphics.h"

static INTERRUPTMANAGER InterruptManager;

char *ReturnError[21] = {
	"EXCEPTION_DIVIEDERROR" , 
	"EXCEPTION_DEBUG" , 
	"EXCEPTION_NMI" , 
	"EXCEPTION_BREAKPOINT" , 
	"EXCEPTION_OVERFLOW" , 
	"EXCEPTION_BOUNDRANGEEXCEEDED" , 
	"EXCEPTION_INVALIDOPCODE" , 
	"EXCEPTION_DEVICENOTAVAILABLE" , 
	"EXCEPTION_DOUBLEFAULT" , 
	"EXCEPTION_COPROCESSORSEGMENUTOVERRUN" , 
	"EXCEPTION_INVALIDTSS" , 
	"EXCEPTION_SEGMENTNOTPRESENT" , 
	"EXCEPTION_STACKSEGMENTFAULT" , 
	"EXCEPTION_GENERALPROTECTION" , 
	"EXCEPTION_PAGEFAULT" , 
	"EXCEPTION_15" , 
	"EXCEPTION_FPUERROR" , 
	"EXCEPTION_ALIGNMENTCHECK" , 
	"EXCEPTION_MACHINECHECK" , 
	"EXCEPTION_SIMDERRROR" , 
	"EXCEPTION_ETC" , 
};

void InitHandler(void) {
	MemSet(&InterruptManager , 0 , sizeof(InterruptManager));
}

void SetSymmetricIOMode(BOOL SymmetricIOMode) {
	InterruptManager.SymmetricIOMode = SymmetricIOMode;
}

void SetInterruptLoadBalancing(BOOL UseLoadBalancing) {
	InterruptManager.UseLoadBalancing = UseLoadBalancing;
}

void IncreaseInterruptCount(int IRQ) {
	InterruptManager.CoreInterruptCount[GetAPICID()][IRQ]++;
}

void SendEOI(int IRQ) {
	if(InterruptManager.SymmetricIOMode == FALSE) {
		SendEOIToPIC(IRQ);
	}
	else {
		SendEOIToLocalAPIC();
	}
}

INTERRUPTMANAGER *GetInterruptManager(void) {
	return &InterruptManager;
}

void ProcessLoadBalancing(int IRQ) {
	QWORD MinCount = 0xFFFFFFFFFFFFFFFF;
	int MinCountCoreIndex;
	int CoreCount;
	int i;
	BOOL ResetCount = FALSE;
	BYTE APICID;
	APICID = GetAPICID();
	if((InterruptManager.CoreInterruptCount[APICID][IRQ] == 0)||((InterruptManager.CoreInterruptCount[APICID][IRQ] % INTERRUPT_LOADBALANCINGDIVIDOR) != 0)||(InterruptManager.UseLoadBalancing == FALSE)) {
		return;
	}
	MinCountCoreIndex = 0;
	CoreCount = GetProcessorCount();
	for(i = 0; i < CoreCount; i++) {
		if((InterruptManager.CoreInterruptCount[i][IRQ] < MinCount)) {
			MinCount = InterruptManager.CoreInterruptCount[i][IRQ];
			MinCountCoreIndex = i;
		}
		else if(InterruptManager.CoreInterruptCount[i][IRQ] >= 0xFFFFFFFFFFFFFFFE) {
			ResetCount = TRUE;
		}
	}
	RoutingIRQToAPICID(IRQ , MinCountCoreIndex);
	if(ResetCount == TRUE) {
		for(i = 0; i < CoreCount; i++) {
			InterruptManager.CoreInterruptCount[i][IRQ] = 0;
		}
	}
}

void CommonExceptionHandler(int VectorNumber , QWORD ErrorCode) {
	if(*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0) {
		ClearScreen(0x00);
		char BufferVC[3] = {0 , };
		BufferVC[0] = '0' + VectorNumber/10;
		BufferVC[1] = '0' + VectorNumber%10;
		int i;
		KernelPrintfXY(0x07 , 0 , 0 , "Exception Occur. Your Computer must halted.");
		KernelPrintfXY(0x07 , 0 , 1 , "Error:");
		SPrintf(BufferVC , "%s | %d" , ReturnError[VectorNumber] , VectorNumber); 
		KernelPrintfXY(0x07 , 7 , 1 , BufferVC);
		KernelPrintfXY(0x07 , 0 , 2 , "Core ID:");
		SPrintf(BufferVC , "0x%X" , GetAPICID());
		KernelPrintfXY(0x07 , 9 , 2 , BufferVC);
		KernelPrintfXY(0x07 , 0 , 6 , "System Halt. you must Press Power Button.");
		while(1);
	}
	else {
		RECT ScreenArea;
		COLOR *VideoMemory;
		VBEMODEINFOBLOCK *VBEMode;
		VBEMode = GetVBEInfoBlock();
		ScreenArea.X1 = 0;
		ScreenArea.Y1 = 0;
		ScreenArea.X2 = VBEMode->XResolution-1;
		ScreenArea.Y2 = VBEMode->YResolution-1;
		VideoMemory = (COLOR*)((QWORD)VBEMode->PhysicalBasePointer & 0xFFFFFFFF);
		InternalDrawRect(&ScreenArea , VideoMemory , 0 , 0 , 1024 , 1024 , RGB(0xDD , 00 , 00) , TRUE);
		char TempBuffer[1024];
		FlushFileSystemCache();
		SPrintf(TempBuffer , "Exception Occur. You Computer must halted.");
		InternalDrawText(&ScreenArea , VideoMemory , 10 , 100 , RGB(0xFF , 0xFF , 0xFF) , RGB(0xDD , 00 , 00) , TempBuffer , StrLen(TempBuffer));
		SPrintf(TempBuffer , "Error:%s | %d" , ReturnError[VectorNumber] , VectorNumber);
		InternalDrawText(&ScreenArea , VideoMemory , 10 , 120 , RGB(0xFF , 0xFF , 0xFF) , RGB(0xDD , 00 , 00) , TempBuffer , StrLen(TempBuffer));
		SPrintf(TempBuffer , "Core ID:0x%X" , GetAPICID());
		InternalDrawText(&ScreenArea , VideoMemory , 10 , 140 , RGB(0xFF , 0xFF , 0xFF) , RGB(0xDD , 00 , 00) , TempBuffer , StrLen(TempBuffer));
		SPrintf(TempBuffer , "System Cache Flushed : SUCCESS");
		InternalDrawText(&ScreenArea , VideoMemory , 10 , 160 , RGB(0xFF , 0xFF , 0xFF) , RGB(0xDD , 00 , 00) , TempBuffer , StrLen(TempBuffer));
		SPrintf(TempBuffer , "System Halt.");
		InternalDrawText(&ScreenArea , VideoMemory , 10 , 200 , RGB(0xFF , 0xFF , 0xFF) , RGB(0xDD , 00 , 00) , TempBuffer , StrLen(TempBuffer));
		while(1);
	}
}

void CommonInterruptHandler(int VectorNumber) {
	int IRQ;
	IRQ = VectorNumber-PIC_IRQSTARTVECTOR;
	SendEOI(IRQ);
	IncreaseInterruptCount(IRQ);
	ProcessLoadBalancing(IRQ);
}

void KeyboardHandler(int VectorNumber) {
	int IRQ;
	BYTE Temp;
	if(IsOutputBufferFull() == TRUE) {
		if(IsMouseDataInOutputBuffer() == FALSE) {
			Temp = GetKeyboardScanCode();
			ConvertScanCodeAndPutQueue(Temp);
		}
		else {
			Temp = GetKeyboardScanCode();
			AccumulateMouseDataAndPutQueue(Temp);
		}
	}
	IRQ = VectorNumber-PIC_IRQSTARTVECTOR;
	SendEOI(IRQ);
	IncreaseInterruptCount(IRQ);
	ProcessLoadBalancing(IRQ);
}

void TimerHandler(int VectorNumber) {
	static int TimerInterruptCount = 0;
	int IRQ;
	BYTE CurrentAPICID;
	TimerInterruptCount = (TimerInterruptCount+1)%10;
	IRQ = VectorNumber-PIC_IRQSTARTVECTOR;
	SendEOI(IRQ);
	IncreaseInterruptCount(IRQ);
	CurrentAPICID = GetAPICID();
	if(CurrentAPICID == 0) {
		TickCount++;
	}
	DecreaseProcessorTime(CurrentAPICID);
	if(IsProcessorTimeExpired(CurrentAPICID) == TRUE) {
		ScheduleInInterrupt();
	}
}

void DeviceNotAvailableHandler(int VectorNumber) {
	TCB *FPUTask;
	TCB *CurrentTask;
	BYTE CurrentAPICID;
	QWORD LastFPUTaskID;
	CurrentAPICID = GetAPICID();
	ClearTS();
	LastFPUTaskID = GetLastFPUUsedTaskID(CurrentAPICID);
	CurrentTask = GetRunningTask(CurrentAPICID);
	if(LastFPUTaskID == CurrentTask->Link.ID) {
		return;
	}
	else if(LastFPUTaskID != TASK_INVALIDID) {
		FPUTask = GetTCBInTCBPool(GETTCBOFFSET(LastFPUTaskID));
		if((FPUTask != NULL) && (FPUTask->Link.ID == LastFPUTaskID)) {
			SaveFPUContext(FPUTask->FPUContext);
		}
	}
	if(CurrentTask->FPUUsed == FALSE) {
		InitFPU();
		CurrentTask->FPUUsed = TRUE;
	}
	else {
		LoadFPUContext(CurrentTask->FPUContext);
	}
	SetLastFPUUsedTaskID(CurrentAPICID , CurrentTask->Link.ID);
}

void HDDHandler(int VectorNumber) {
	SetHDDInterruptFlag(TRUE , TRUE);
	int IRQ;
	IRQ = VectorNumber-PIC_IRQSTARTVECTOR;
	SendEOI(IRQ);
	IncreaseInterruptCount(IRQ);
	ProcessLoadBalancing(IRQ);
}

void MouseHandler(int VectorNumber) {
	BYTE Temp;
	int IRQ;
	if(IsOutputBufferFull() == TRUE) {
		if(IsMouseDataInOutputBuffer() == FALSE) {
			Temp = GetKeyboardScanCode();
			ConvertScanCodeAndPutQueue(Temp);
		}
		else {
			Temp = GetKeyboardScanCode();
			AccumulateMouseDataAndPutQueue(Temp);
		}
	}
	IRQ = VectorNumber-PIC_IRQSTARTVECTOR;
	SendEOI(IRQ);
	IncreaseInterruptCount(IRQ);
	ProcessLoadBalancing(IRQ);
}
