#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "Types.h"
#include "AssemblyUtility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "MPConfigurationTable.h"
#include "LocalAPIC.h"
#include "MultiProcessor.h"
#include "SerialPort.h"
#include "IOAPIC.h"
#include "InterruptHandler.h"

static CONSOLECOLOR ConsoleColor;
static QWORD CPUSpeed = NULL;
static BOOL IsPrtCommandMessage = TRUE;


SHELLCOMMANDENTRY CommandTable[] = {
	{"help" , "Show Help" , Help} , 
	{"clrscr" , "Clear Screen" , Clrscr} , 
	{"sysinfo" , "Show System Information" , ShowSystemInformation} , 
	{"wait" , "Wait ms(Ex:wait 1000(ms))" , Wait} , 
	{"date" , "Show Current Date And Current Time" , ShowDateAndTime} , 
	{"prt" , "Print String In Screen(Ex:prt Hello, World!!)" , Echo} , 
	{"createtask" , "Create Task" , CreateTestTask} , 
	{"killtask" , "Kill Task (Ex:killtask 0x400000003" , KillTask} , 
	{"tasklist" , "Show Task List" , TaskList} , 
	{"prtrand" , "Print Random Number" , PrintRandomNumber} , 
	{"cpuinfo" , "Show CPU Information" , ShowCPUInformation} , 
	{"procload" , "Show Process Load" , ShowProcessLoad} , 
	{"format" , "Format Hard Disk" , FormatHDDInConsole} , 
	{"mount" , "Mount Hard Disk" , MountHDDInConsole} , 
	{"remove" , "Delete File (Ex:deletefile hello.txt)" , DeleteFileInConsole} , 
	{"chkfs" , "Check File System" , CheckFileSystem} , 
	{"copyfile" , "Copy File or Combine File" , CopyFileInConsole} , 
	{"save" , "Flush File System Cache(Save)" , SaveF} , 
	{"download" , "Download File in Serial Port" , DownloadFileInSerialPort} , 
	{"dir" , "Show Directory" , ShowRootDirectory} , 
	{"disable" , "Disable Command Message" , DisableCommandMessage} , 
	{"enable" , "Enable Command Message" , EnableCommandMessage} , 
	{"iendi" , "Check Enable Command Message or Disable" , CheckCommandMessage} , 
	{"pause" , "Paused in Console" , Pause_Console} , 
	{"version" , "Show ChoOS Version" , Ver} , 
	{"calc" , "ChoOS Calculater" , Calculater} , 
	{"ascii" , "Show Ascii Code and Number" , ASCIIList} , 
	{"showmpconfig" , "Show MP Configuration Table Information" , ShowMPConfigurationTable} , 
	{"shutdown" , "Shutdown ChoOS(Ex:shutdown RST(Restart))" , ShutdownChoOS} , 
	{"cs" , "Execute *.CE or *.CS File" , ExecuteConsoleShellScript} , 
	{"~>" , "Comments. using CS Script" , Comments} , 
	{"editor" , "Start Editor" , Edit} , 
	{"readfile" , "Reading File" , ReadFileInConsole} , 
};

void TimeTask(void) {
	int i;
	for(i = 0; i < 80; i++) {
		KernelPrintfXY(0x22 , i , 24 , " ");
	}
	while(1) {
		BYTE Second;
		BYTE Minute;
		BYTE Hour;
		BYTE DayOfWeek;
		BYTE DayOfMonth;
		BYTE Month;
		WORD Year;
		ReadRTCTimer(&Hour , &Minute , &Second);
		ReadRTCDate(&Year , &Month , &DayOfMonth , &DayOfWeek);
		char PrtString[80];
		char TempBuffer[80];
		char TempBuffer2[80];
		char TempBuffer3[80];
		if(Hour >= 12) {
			if(Hour == 12) {
				SPrintf(TempBuffer , "PM 12:");
			}
			else {
				if(Hour-12 < 10) {
					SPrintf(TempBuffer , "PM 0%d:" , Hour-12);
				}
				else {
					SPrintf(TempBuffer , "PM %d:" , Hour-12);
				}
			}
		}
		else {
			if(Hour < 10) {
				SPrintf(TempBuffer , "AM 0%d:" , Hour);
			}
			else {
				SPrintf(TempBuffer , "AM %d:" , Hour);
			}
		}
		if(Minute < 10) {
			SPrintf(TempBuffer2 , "0%d:" , Minute);
		}
		else {
			SPrintf(TempBuffer2 , "%d:" , Minute);
		}
		if(Second < 10) {
			SPrintf(TempBuffer3 , "0%d" , Second);
		}
		else {
			SPrintf(TempBuffer3 , "%d" , Second);
		}
		SPrintf(PrtString , "%d/%d/%d %s %s%s%s          " , Year , Month , DayOfMonth , ConvertDayOfWeekToString(DayOfWeek) , TempBuffer , TempBuffer2 , TempBuffer3);
		KernelPrintfXY(0x21 , 0 , 24 , PrtString);
	}
}

void StartConsoleShell(CONSOLECOLOR Color) {
	CreateTask(TASK_FLAGS_LOW|TASK_FLAGS_THREAD , 0 , 0 , (QWORD)TimeTask , TASK_LOADBALANCINGID);
	PrtStartConsoleShellMessage(Color);
	ConsoleColor = Color-COLOR_TEXT_WHITE;
	char CommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
	int CommandBufferIndex = 0;
	BYTE Key;
	int CursorX;
	int CursorY;
	if(IsPrtCommandMessage == TRUE) {
		Printf(Color , CONSOLESHELL_PROMPTMESSAGE);
	}
	else {
		;
	}
	while(1) {
		Key = GetCh();
		if(Key == KEY_BACKSPACE) {
			if(CommandBufferIndex > 0) {
				GetCursor(&CursorX , &CursorY);
				KernelPrintfXY(Color , CursorX-1 , CursorY , " ");
				SetCursor(CursorX-1 , CursorY);
				CommandBufferIndex--;
			}
		}
		else if(Key == KEY_ENTER) {
			Printf(Color , "\n");
			if(CommandBufferIndex > 0) {
				CommandBuffer[CommandBufferIndex] = '\0';
				ExecuteCommand(Color , CommandBuffer);
			}
			if(IsPrtCommandMessage == TRUE) {
				Printf(Color , CONSOLESHELL_PROMPTMESSAGE);
			}
			else {
				;
			}
			MemSet(CommandBuffer , '\0' , CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
			CommandBufferIndex = 0;
		}
		else if(Key == KEY_LSHIFT) {
			;
		}
		else if(Key == KEY_RSHIFT) {
			;
		}
		else if(Key == KEY_RSHIFT) {
			;
		}
		else if(Key == KEY_CAPSLOCK) {
			;
		}
		else if(Key == KEY_NUMLOCK) {
			;
		}
		else if(Key == KEY_SCROLLLOCK) {
			;
		}
		else if(Key == KEY_UP) {
			;
		}
		else if(Key == KEY_DOWN) {
			;
		}
		else if(Key == KEY_RIGHT) {
			;
		}
		else if(Key == KEY_LEFT) {
			;
		}
		else if(Key == KEY_ESC) {
			;
		}
		else if(Key == KEY_CTRL) {
			;
		}
		else if(Key == KEY_LALT) {
			;
		}
		else {
			if(Key == KEY_TAB) {
				Key = ' ';
				Printf(Color , "    ");
				CommandBuffer[CommandBufferIndex++] = Key;
				CommandBuffer[CommandBufferIndex++] = Key;
				CommandBuffer[CommandBufferIndex++] = Key;
				CommandBuffer[CommandBufferIndex++] = Key;
			}
			if(CommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT) {
				CommandBuffer[CommandBufferIndex++] = Key;
				Printf(Color , "%c" , Key);
			}
		}
	}
}

static void PrtStartConsoleShellMessage(CONSOLECOLOR Color) {
	Printf(Color , "<Welcome to ChoOS>\n");
	Printf(Color , "Copyright 2017-2018 ChoOS Co. All rights reserved.\n\n");
}

void ExecuteCommand(CONSOLECOLOR Color , const char *CommandBuffer) {
	int i;
	int SpaceIndex;
	int CommandBufferLength;
	int CommandLength;
	int Count;
	CommandBufferLength = StrLen(CommandBuffer);
	for(SpaceIndex = 0; SpaceIndex < CommandBufferLength; SpaceIndex++) {
		if(CommandBuffer[SpaceIndex] == ' ') {
			break;
		}
	}
	Count = sizeof(CommandTable)/sizeof(SHELLCOMMANDENTRY);
	if(IsEqual((char*)CommandBuffer , "do you know about chojuha?") == TRUE) {
		Printf(Color , "Yes! I know!!\n");
		return;
	}
	for(i = 0; i < Count; i++) {
		CommandLength = StrLen(CommandTable[i].Command);
		if((CommandLength == SpaceIndex) && (MemCmp(CommandTable[i].Command , CommandBuffer , SpaceIndex) == 0)) {
			CommandTable[i].Function(Color , CommandBuffer+SpaceIndex+1);
			break;
		}
	}
	if(i >= Count) {
		Printf(Color , "Sorry, I Don't know this Order.\n");
	}
}

void ExecuteCommandInCS(CONSOLECOLOR Color , const char *CommandBuffer) {
	int i;
	int SpaceIndex;
	int CommandBufferLength;
	int CommandLength;
	int Count;
	CommandBufferLength = StrLen(CommandBuffer);
	for(SpaceIndex = 0; SpaceIndex < CommandBufferLength; SpaceIndex++) {
		if(CommandBuffer[SpaceIndex] == ' ') {
			break;
		}
	}
	Count = sizeof(CommandTable)/sizeof(SHELLCOMMANDENTRY);
	for(i = 0; i < Count; i++) {
		CommandLength = StrLen(CommandTable[i].Command);
		if((CommandLength == SpaceIndex) && (MemCmp(CommandTable[i].Command , CommandBuffer , SpaceIndex) == 0)) {
			CommandTable[i].Function(Color , CommandBuffer+SpaceIndex+1);
			break;
		}
	}
	if(i >= Count) {
		Printf(Color , "Sorry, I Don't know this Order\n");
	}
}

void InitParameter(PARAMETERLIST *List , const char *Parameter) {
	List->Buffer = Parameter;
	List->Length = StrLen(Parameter);
	List->CurrentPosition = 0;
}

int GetNextParameter(PARAMETERLIST *List , char *Parameter) {
	int i;
	int Length;
	if(List->Length <= List->CurrentPosition) {
		return 0;
	}
	for(i = List->CurrentPosition; i < List->Length; i++) {
		if(List->Buffer[i] == ' ') {
			break;
		}
	}
	MemCpy(Parameter , List->Buffer+List->CurrentPosition , i);
	Length = i-List->CurrentPosition;
	Parameter[Length] = '\0';
	List->CurrentPosition += Length+1;
	return Length;
}

static void Help(CONSOLECOLOR Color , const char *CommandBuffer) {
	int i;
	int Count;
	int CursorX;
	int CursorY;
	int Length = 0;
	int MaxCommandLength = 0;
	Printf(Color , "<ChoOS Shell Help>\n\n");
	Count = sizeof(CommandTable)/sizeof(SHELLCOMMANDENTRY);
	for(i = 0; i < Count; i++) {
		Length = StrLen(CommandTable[i].Command);
		if(Length > MaxCommandLength) {
			MaxCommandLength = Length;
		}
	}
	for(i = 0; i < Count; i++) {
		Printf(Color , "%s" , CommandTable[i].Command);
		GetCursor(&CursorX , &CursorY);
		SetCursor(MaxCommandLength , CursorY);
		Printf(Color , " | %s\n" , CommandTable[i].Help);
		if((i != 0) && ((i % 10) == 0)) {
			if(Paused(Color) == TRUE) {
				return;
			}
		}
	}
}

static void Clrscr(CONSOLECOLOR Color , const char *ParameterBuffer) {
	ClearScreen(Color);
	SetCursor(0 , 1);
}

static void ShowSystemInformation(CONSOLECOLOR Color , const char *ParameterBuffer) {
	Printf(Color , "<ChoOS System Information>\n\n");
	Printf(Color , "x32 Supported            : TRUE->FALSE\n");
	Printf(Color , "x64 Supported            : TRUE\n");
	Printf(Color , "Current System architect : x64\n");
	Printf(Color , "Total RAM Size(MB)       : %dMB\n" , GetTotalRamSize());
	if(Paused(Color) == TRUE) {
		return;
	}
	Printf(Color , "<DynamicMemory Information>\n\n");
	QWORD DynamicMemoryStartAddress;
	QWORD DynamicMemoryTotalSize;
	QWORD DynamicMemoryMetaSize;
	QWORD DynamicMemoryUsedSize;
	GetDynamicMemoryInformation(&DynamicMemoryStartAddress , DynamicMemoryTotalSize , &DynamicMemoryMetaSize , &DynamicMemoryUsedSize);
	Printf(Color , "Start Address            : 0x%Q\n" , DynamicMemoryStartAddress);
	Printf(Color , "Total Size               : %dMB\n" , DynamicMemoryTotalSize / 1024 / 1024);
	Printf(Color , "Meta Size                : %dKB\n" , DynamicMemoryMetaSize / 1024);
	Printf(Color , "Used Size                : %dKB\n" , DynamicMemoryUsedSize / 1024);
	if(Paused(Color) == TRUE) {
		return;
	}
	Printf(Color , "<Hard Disk Information>\n\n");
	HDDINFORMATION HDD;
	char Buffer[100];
	if(GetHDDInformation(&HDD) == FALSE) {
		Printf(Color , "I Can't Read HDD Information.\n");
		return;
	}
	MemCpy(Buffer , HDD.ModelNumber , sizeof(HDD.ModelNumber));
	Buffer[sizeof(HDD.ModelNumber)-1] = '\0';
	Printf(Color , "Model Number             : %s\n" , Buffer);
	MemCpy(Buffer , HDD.SerialNumber , sizeof(HDD.SerialNumber));
	Buffer[sizeof(HDD.SerialNumber)-1] = '\0';
	Printf(Color , "Serial Number            : %s\n" , Buffer);
	Printf(Color , "Cylinder Count           : %d\n" , HDD.NumberOfCylinder);
	Printf(Color , "Sector Count             : %d\n" , HDD.NumberOfSectorPerCylinder);
	Printf(Color , "Total Sector             : %d Sector\n" , HDD.TotalSectors);
	Printf(Color , "Total Sector(MB)         : %dMB\n" , HDD.TotalSectors / 2 / 1024);
	if(Paused(Color) == TRUE) {
		return;
	}
	FILESYSTEMMANAGER FSManager;
	GetFileSystemInformation(&FSManager);
	Printf(Color , "<File System Information>\n\n");
	Printf(Color , "File System Mounted              : ");
	if(FSManager.Mounted == TRUE) {
		Printf(Color , "TRUE");
	}
	else {
		Printf(Color , "FALSE");
	}
	Printf(Color , "\n");
	Printf(Color , "Reserved Sector Count            : %d Sector\n" , FSManager.ReservedSectorCount);
	Printf(Color , "Cluster Link Table Start Address : %d Sector\n" , FSManager.ClusterLinkAreaStartAddress);
	Printf(Color , "Data Area Start Address          : %d Sector\n" , FSManager.DataAreaStartAddress);
	Printf(Color , "Total Cluster Count              : %d Cluster\n" , FSManager.TotalClusterCount);
}

void ShutdownChoOS(CONSOLECOLOR Color , const char *ParameterBuffer) {
	CONSOLECOLOR Color_A = Color;
	ClearScreen(Color);
	PARAMETERLIST List;
	char Parameter[100];
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , Parameter) == 0) {
		goto SHUTDOWNSTART;
	}
	else if(IsEqual(Parameter , "RST") == TRUE) {
		Reboot();
	}
	else {
		goto SHUTDOWNSTART;
	}
SHUTDOWNSTART:
	Printf(Color , "<Shutdown ChoOS>\n\n");
	Printf(Color , "1. Restart ChoOS\n");
	Printf(Color , "2. Return Console\n\n");
	Printf(Color , "Please Input Number [ ]");
	SetCursor(21 , 5);
	char a;
	a = GetCh();
	if(a == '1') {
		if(FlushFileSystemCache() == FALSE) {
			return;
		}
		Reboot();
	}
	else {
		ClearScreen(Color_A);
		SetCursor(0 , 1);
		return;
	}
}

static void Wait(CONSOLECOLOR Color , const char *ParameterBuffer) {
	char Parameter[100];
	int Length;
	PARAMETERLIST List;
	long MilliSecond;
	int i;
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , Parameter) == 0) {
		Printf(Color , "wait:Syntax Error\n");
		return;
	}
	MilliSecond = atoi(ParameterBuffer , 10);
	if(MilliSecond > 36000000) {
		return;
	}
	DisableInterrupt();
	for(i = 0; i < MilliSecond/30; i++) {
		WaitUsingDirectPIT(MSTOCOUNT(30));
	}
	WaitUsingDirectPIT(MSTOCOUNT(MilliSecond % 30));
	EnableInterrupt();
	InitPIT(MSTOCOUNT(1) , TRUE);
}

static void ShowDateAndTime(CONSOLECOLOR Color , const char *ParameterBuffer) {
	BYTE Second;
	BYTE Minute;
	BYTE Hour;
	BYTE DayOfWeek;
	BYTE DayOfMonth;
	BYTE Month;
	WORD Year;
	ReadRTCTimer(&Hour , &Minute , &Second);
	ReadRTCDate(&Year , &Month , &DayOfMonth , &DayOfWeek);
	Printf(Color , "%d/%d/%d %s\n" , Year , Month , DayOfMonth , ConvertDayOfWeekToString(DayOfWeek));
	if(Hour >= 12) {
		if(Hour == 12) {
			Printf(Color , "PM 12:");
		}
		else {
			Printf(Color , "PM %d:" , Hour-12);
		}
	}
	else {
		Printf(Color , "AM %d:" , Hour);
	}
	if(Minute < 10) {
		Printf(Color , "0");
	}
	Printf(Color , "%d:" , Minute);
	if(Second < 10) {
		Printf(Color , "0");
	}
	Printf(Color , "%d\n" , Second);
}

static void TestTask(void) {
	int i = 0;
	int Offset;
	CHARACTER *Screen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB *RunningTask;
	RunningTask = GetRunningTask(GetAPICID());
	Offset = (((RunningTask->Link.ID & 0xFFFFFFFF)*2));
	Offset = CONSOLE_WIDTH*CONSOLE_HEIGHT-(Offset % (CONSOLE_WIDTH*CONSOLE_HEIGHT));
	while(1) {
		Screen[Offset].Charactor = (Random()%255)+1;
		QWORD Color = (Random()%6)+1;
		Screen[Offset].Attribute = (0x10)+Color+1;
		i++;
	}
	ExitTask();
}

static void Echo(CONSOLECOLOR Color , const char *ParameterBuffer) {
	char Parameter[299];
	PARAMETERLIST List;
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , Parameter) == 0) {
		return;
	}
	Printf(Color , Parameter);
	int i;
	for(i = 0; GetNextParameter(&List , Parameter) != 0; i++) {
		Printf(Color , " ");
		Printf(Color , Parameter);
	}
	Printf(Color , "\n");
	return;
}

static void CreateTestTask(CONSOLECOLOR Color , const char *ParameterBuffer) {
	int i;
	char Parameter[299];
	PARAMETERLIST List;
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , Parameter) == 0) {
		return;
	}
	for(i = 0; i < atoi(Parameter , 10); i++) {
		CreateTask(TASK_FLAGS_LOW|TASK_FLAGS_THREAD , 0 , 0 , (QWORD)TestTask , TASK_LOADBALANCINGID);
	}
	Printf(Color , "\n");
}

static void KillTask(CONSOLECOLOR Color , const char *ParameterBuffer) {
	PARAMETERLIST List;
	char CharOfID[30];
	QWORD ID;
	TCB *Tcb;
	int i;
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , CharOfID) == 0) {
		Printf(Color , "killtask:Syntax Error\n");
		return;
	}
	if(MemCmp(CharOfID , "0x" , 2) == 0) {
		ID = atoi(CharOfID+2 , 16);
	}
	else {
		ID = atoi(CharOfID , 10);
	}
	if(ID != 0xFFFFFFFF) {
		Tcb = GetTCBInTCBPool(GETTCBOFFSET(ID));
		ID = Tcb->Link.ID;
		if(((ID >> 32) != 0) && ((Tcb->Flags & TASK_FLAGS_SYSTEM) == 0x00)) {
			Printf(Color , "killtask:Kill Task ID [0x%q] " , ID);
			if(EndTask(ID) == TRUE) {
				Printf(Color , "Success\n");
			}
			else {
				Printf(Color , "Fail\n");
			}
		}
		else {
			Printf(Color , "killtask:Task dose Not Exist or Task is System Task\n");
		}
	}
	else {
		int Success;
		int Fail;
		for(i = 0; i < TASK_MAXCOUNT; i++) {
			Tcb = GetTCBInTCBPool(i);
			ID = Tcb->Link.ID;
			if(((ID >> 32) != 0) && ((Tcb->Flags & TASK_FLAGS_SYSTEM) == 0x00)) {
				if(EndTask(ID) == TRUE) {
					Success++;
				}
				else {
					Fail++;
				}
			}
		}
		if((Success >= 1) && (Fail == 0)) {
			Printf(Color , "killtask:Kill All Task Successed\n");
		}
		else if(Fail >= 1) {
			Printf(Color , "killtask:Kill All Task Failed\n");
		}
		else {
			Printf(Color , "killtask:Task List is Empty(!= TASK_SYSTEM)\n");
		}
	}
}

static void TaskList(CONSOLECOLOR Color , const char *ParameterBuffer) {
	int i;
	TCB *Tcb;
	int Count = 0;
	int TotalTaskCount = 0;
	char Buffer[20];
	int RemainLength;
	int ProcessorCount;
	ProcessorCount = GetProcessorCount();
	for(i = 0; i < ProcessorCount; i++) {
		TotalTaskCount += GetTaskCount(i);
	}
	Printf(Color , "Total Task Count : %d\n" , TotalTaskCount);
	if(ProcessorCount > 1) {
		for(i = 0; i < ProcessorCount; i++) {
			if((i != 0) && ((i%4) == 0)) {
				Printf(Color , "\n");
			}
			SPrintf(Buffer , "Core %d : %d" , i , GetTaskCount(i));
			Printf(Color , Buffer);
			RemainLength = 19-StrLen(Buffer);
			MemSet(Buffer , ' ' , RemainLength);
			Buffer[RemainLength] = '\0';
			Printf(Color , Buffer);
		}
		if(Paused(Color) == TRUE) {
			return;
		}
	}
	for(i = 0; i < TASK_MAXCOUNT; i++) {
		Tcb = GetTCBInTCBPool(i);
		if((Tcb->Link.ID >> 32) != 0) {
			if((Count != 0) && ((Count%6) == 0)) {
				if(Paused(Color) == TRUE) {
					return;
				}
			}
			Printf(Color , "%d. Task ID:0x%Q | Priority:%d | Flags:0x%Q | Thread:%d\n" , 1+Count++ , Tcb->Link.ID , GETPRIORITY(Tcb->Flags) , Tcb->Flags , GetListCount(&(Tcb->ChildThreadList)));
			Printf(Color , "    Core ID:0x%X | CPU Affinity:0x%X\n" , Tcb->APICID , Tcb->Affinity);
			Printf(Color , "    Parent PID:0x%Q | Memoy Address:0x%Q | Size:0x%Q\n" , Tcb->ParentProcessID , Tcb->MemoryAddress , Tcb->MemorySize);
		}
	}
}

static void PrintRandomNumber(CONSOLECOLOR Color , const char *ParameterBuffer) {
	Printf(Color , "Random Number = %d\n" , Random());
}

static void ShowCPUInformation(CONSOLECOLOR Color , const char *ParameterBuffer) {
	int X;
	int Y;
	if(CPUSpeed == NULL) {
		Printf(Color , "Loading\n");
		DisableInterrupt();
		int i;
		QWORD LastTSC;
		QWORD TotalTSC;
		for(i = 0; i < 200; i++) {
			LastTSC = ReadTSC();
			WaitUsingDirectPIT(MSTOCOUNT(50));
			TotalTSC += ReadTSC()-LastTSC;
			Printf(Color , ".");
		}
		InitPIT(MSTOCOUNT(1) , TRUE);
		EnableInterrupt();
		CPUSpeed = TotalTSC / 10 / 1000 / 1000;
		GetCursor(&X , &Y);
		SetCursor(0 , Y+1);
	}
	Printf(Color , "<CPU Information>\n\n");
	Printf(Color , "CPU Speed                : %dMHz\n" , CPUSpeed);
}

static void ShowProcessLoad(CONSOLECOLOR Color , const char *ParameterBuffer) {
	int i;
	char Buffer[50];
	int RemainLength;
	for(i = 0; i < GetProcessorCount(); i++) {
		if((i != 0) && ((i%5) == 0)) {
			Printf(Color , "\n");
		}
		SPrintf(Buffer , "Core %d : %d%%" , i , GetProcessorLoad(i));
		Printf(Color , "%s" , Buffer);
		RemainLength = 19-StrLen(Buffer);
		MemSet(Buffer , ' ' , RemainLength);
		Buffer[RemainLength] = '\0';
		Printf(Color , Buffer);
	}
	Printf(Color , "\n");
}

static void FormatHDDInConsole(CONSOLECOLOR Color , const char *ParameterBuffer) {
	Printf(0x14 , "Warning:are you format your Hard Disk(Y/N)?");
	switch(GetCh()) {
		case 'y':
			Printf(Color , "\n");
			break;
		case 'n':
			Printf(Color , "\n");
			return;
		default:
			Printf(Color , "\n");
			return;
	}
	if(FormatHDD() == FALSE) {
		Printf(Color , "Hard Disk Format Fail\n");
	}
	else {
		Printf(Color , "Hard Disk Format Success\n");
	}
}

static void MountHDDInConsole(CONSOLECOLOR Color , const char *ParameterBuffer) {
	if(MountHDD() == FALSE) {
		Printf(Color , "Hard Disk Mount Fail\n");
	}
	else {
		Printf(Color , "Hard Disk Mount Success\n");
	}
}

static void DeleteFileInConsole(CONSOLECOLOR Color , const char *ParameterBuffer) {
	PARAMETERLIST List;
	char FileName[30];
	int Length;
	DWORD Cluster;
	DIRECTORYENTRY Entry;
	int i;
	InitParameter(&List , ParameterBuffer);
	Length = GetNextParameter(&List , FileName);
	FileName[Length] = '\0';
	if((Length > (FILESYSTEM_MAXFILENAMELENGTH-1))||(Length == 0)) {
		Printf(Color , "remove:Too Long or Short File Name\n");
		return;
	}
	if(remove(FileName) == -1) {
		Printf(Color , "remove:File Remove Fail\n");
		return;
	}
	Printf(Color , "\n");
}

static void ShowRootDirectory(CONSOLECOLOR Color , const char *ParameterBuffer) {
	DIR *Directory;
	BYTE *ClusterBuffer;
	int i;
	int Count;
	struct dirent *Entry;
	int TotalCount;
	char Buffer[400];
	char TempVal[50];
	DWORD TotalByte;
	DWORD UsedClusterCount;
	HDDINFORMATION Manager;
	GetHDDInformation(&Manager);
	Directory = opendir("/");
	if(Directory == NULL) {
		Printf(Color , "dir:Directory Read Fail\n");
		return;
	}
	TotalCount = 0;
	TotalByte = 0;
	UsedClusterCount = 0;
	rewinddir(Directory);
	Count = 0;
	while(1) {
		Entry = readdir(Directory);
		if(Entry == NULL) {
			break;
		}
		MemSet(Buffer , ' ' , sizeof(Buffer)-1);
		Buffer[sizeof(Buffer)-1] = '\0';
		MemCpy(Buffer , Entry->d_name , StrLen(Entry->d_name));
		SPrintf(TempVal , "%dB" , Entry->FileSize);
		TotalByte += Entry->FileSize;
		TotalCount++;
		MemCpy(Buffer+30 , TempVal , StrLen(TempVal));
		
		char FileType[30] = {0 , };
		int i;
		for(i = 0; Entry->d_name[i] != '.'; i++) {
			;
		}
		int r = i;
		for(; Entry->d_name[i] != '\0'; i++) {
			FileType[i-r] = Entry->d_name[i];
		}
		FileType[i] = '\0';
		if(IsEqual(FileType , ".TXT")||IsEqual(FileType , ".TEXT")||IsEqual(FileType , ".txt")||IsEqual(FileType , ".text")) {
			SPrintf(TempVal , "Text File");
		}
		else if(IsEqual(FileType , ".ukn")||IsEqual(FileType , ".UKN")) {
			SPrintf(TempVal , "Text/Binary File");
		}
		else if(IsEqual(FileType , ".cde")||IsEqual(FileType , ".CDE")||IsEqual(FileType , ".code")||IsEqual(FileType , ".CODE")) {
			SPrintf(TempVal , "ChoOS Code File");
		}
		else if(IsEqual(FileType , ".sys")||IsEqual(FileType , ".SYS")||IsEqual(FileType , ".log")||IsEqual(FileType , ".LOG")) {
			SPrintf(TempVal , "System File(log File)");
		}
		else if(IsEqual(FileType , ".EDT")||IsEqual(FileType , ".edt")||IsEqual(FileType , ".edit")||IsEqual(FileType , ".EDIT")) {
			SPrintf(TempVal , "Edit File");
		}
		else if(IsEqual(FileType , ".bny")||IsEqual(FileType , ".BNY")||IsEqual(FileType , ".binary")||IsEqual(FileType , ".BINARY")) {
			SPrintf(TempVal , "Binary File");
		}
		else if(IsEqual(FileType , ".ce")||IsEqual(FileType , ".CE")||IsEqual(FileType , ".CONEXECUTE")||IsEqual(FileType , ".conexeucte")||IsEqual(FileType , ".conexe")||IsEqual(FileType , ".CONEXE")||IsEqual(FileType , ".cs")||IsEqual(FileType , ".CS")) {
			SPrintf(TempVal , "Console Execute File");
		}
		else if(IsEqual(FileType , "")||IsEqual(FileType , "")||IsEqual(FileType , "")||IsEqual(FileType , "")||IsEqual(FileType , "")||IsEqual(FileType , "")||IsEqual(FileType , "")||IsEqual(FileType , "")) {
			SPrintf(TempVal , "Just File");
		}
		else if(IsEqual(FileType , ".c")||IsEqual(FileType , ".C")) {
			SPrintf(TempVal , "C Source File");
		}
		else if(IsEqual(FileType , ".h")||IsEqual(FileType , ".hpp")||IsEqual(FileType , ".rh")||IsEqual(FileType , ".hh")) {
			SPrintf(TempVal , "C/C++ Header File");
		}
		else if(IsEqual(FileType , ".cpp")||IsEqual(FileType , ".cc")||IsEqual(FileType , ".cxx")||IsEqual(FileType , ".c++")||IsEqual(FileType , ".cp")||IsEqual(FileType , ".CPP")||IsEqual(FileType , ".CC")||IsEqual(FileType , ".CXX")||IsEqual(FileType , ".C++")||IsEqual(FileType , ".CP")) {
			SPrintf(TempVal , "C++ Source File");
		}
		else {
			SPrintf(TempVal , "Unknow File Type");
		}
		MemCpy(Buffer+55 , TempVal , StrLen(TempVal)+1);
		Printf(Color , "%s\n" , Buffer);
		if((Count != 0) && ((Count % 10) == 0)) {
			if(Paused(Color) == TRUE) {
				break;
			}
		}
		Count++;
	}
	Printf(Color , "\n");
	Printf(Color , "Total File Count : %d\n" , TotalCount);
	Printf(Color , "Total File Size  : %fMB(%fKB , %dB)\n" , (double)TotalByte / 1000 / 1000 , (double)TotalByte / 1000 , TotalByte);
	closedir(Directory);
}

void DisableCommandMessage(CONSOLECOLOR Color , const char *ParameterBuffer) {
	IsPrtCommandMessage = FALSE;
}

void EnableCommandMessage(CONSOLECOLOR Color , const char *ParameterBuffer) {
	IsPrtCommandMessage = TRUE;
}

static void FallingMarbleTask(void) {
	int X = 0;
	int Y = 0;
	TCB *MarbleTask;
	MarbleTask = GetRunningTask(GetAPICID());
	X = (MarbleTask->Link.ID & 0xFFFFFFFF)*2;
	X = (X % (CONSOLE_WIDTH*CONSOLE_HEIGHT));
	while(1) {
		int RandomColor = Random() % 10;
		char prt[1];
		if(Random() % 6 == 0) {
			prt[0] = '0';
		}
		else if(Random() % 6 == 1) {
			prt[0] = 'o';
		}
		else if(Random() % 6 == 2) {
			prt[0] = 'O';
		}
		else if(Random() % 6 == 3) {
			prt[0] = '@';
		}
		else if(Random() % 6 == 4) {
			prt[0] = '*';
		}
		else if(Random() % 6 == 5) {
			prt[0] = '&';
		}
		prt[1] = '\0';
		KernelPrintfXY(ReturnColor(COLOR_BACKGROUND_BLUE , COLOR_TEXT_WHITE) , X , Y , prt);
		Sleep(50);
		KernelPrintfXY(COLOR_BACKGROUND_BLUE , X , Y , " ");
		Sleep(50);
		if(Y >= 25) {
			Y = 0;
		}
		int Py = (Random() % 4);
		if(Py == 0) {
			Py = (Random() % 4);
			if(Py == 0) {
				Py++;
			}
		}
		Y += Py;
		Schedule();
	}
}

static void FallingMarbleCreateTask() {
	int i;
	ClearScreen(COLOR_BACKGROUND_BLUE);
	for(i = 0; i < 300; i++) {
		if(CreateTask(TASK_FLAGS_LOW|TASK_FLAGS_THREAD , 0 , 0 , (QWORD)FallingMarbleTask , TASK_LOADBALANCINGID) == NULL) {
			break;
		}
		Sleep(Random() % 10 + 5);
	}
	GetCh();
}

static void FallingMarble(CONSOLECOLOR Color , const char *ParameterBuffer) {
	TCB *Process;
	Process = CreateTask(TASK_FLAGS_PROCESS|TASK_FLAGS_LOW , 0 , 0 , (QWORD)FallingMarbleCreateTask , TASK_LOADBALANCINGID);
	if(Process != NULL) {
		while((Process->Link.ID >> 32) != 0) {
			Sleep(100);
		}
	}
	else {
		Printf(Color , ".......??\n");
	}
}

static void CheckCommandMessage(CONSOLECOLOR Color , const char *ParameterBuffer) {
	if(IsPrtCommandMessage == TRUE) {
		Printf(Color , "Enable.\n");
		return;
	}
	else {
		Printf(Color , "Disable.\n");
		return;
	}
}

static void Pause_Console(CONSOLECOLOR Color , const char *ParameterBuffer) {
	PARAMETERLIST List;
	char Parameter[295];
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , Parameter) == 0) {
		int X;
		int Y;
		GetCursor(&X , &Y);
		Printf(Color , "<Paused>\n");
		GetCh();
		SetCursor(X , Y);
		Printf(Color , "           \n");
		SetCursor(X , Y);
		return;
	}
	if(IsEqual(Parameter , "-no") == TRUE) {
		GetCh();
		Printf(Color , "\n");
		return;
	}
	else {
		int i;
		InitParameter(&List , ParameterBuffer);
		int X;
		int Y;
		GetCursor(&X , &Y);
		int TotalCount;
		for(i = 0; GetNextParameter(&List , Parameter) != 0; i++) {
			Printf(Color , "%s " , Parameter);
			TotalCount += StrLen(Parameter)+1;
		}
		GetCh();
		SetCursor(X , Y);
		for(i = 0; i < TotalCount; i++) {
			Printf(Color , " ");
		}
		SetCursor(X , Y);
		return;
	}
}

static void Ver(CONSOLECOLOR Color , const char *ParameterBuffer) {
	Printf(Color , "ChoOS Version 1.0 CUI Mode\n\n");
	Printf(Color , "Copyright 2017-2018 ChoOS Co. All rights reserved.\n");
}

#define M_PIE 3.1415926535897932384626433832795028841971693993751

static void ASCIIList(CONSOLECOLOR Color , const char *ParameterBuffer) {
	char Parameter[300];
	PARAMETERLIST List;
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , Parameter) == 0) {
		Printf(Color , "ascii:Syntax Error\n");
		return;
	}
	int Count;
	char ASC;
	Count = atoi(Parameter , 10);
	if(GetNextParameter(&List , Parameter) == 0) {
		int i;
		Printf(Color , "<ASCII Code List>\n\n");
		for(i = 0; i <= Count; i++) {
			if(((i % 5) == 0) && (i != 0)) {
				if(Paused(Color) == TRUE) {
					return;
				}
			}
			Printf(Color , "ASCIICode:%d | Charactor:'%c'\n" , i , i);
		}
		return;
	}
	InitParameter(&List , ParameterBuffer);
	GetNextParameter(&List , Parameter);
	if(IsEqual(Parameter , "-p") == TRUE) {
		if(GetNextParameter(&List , Parameter) == 0) {
			Printf(Color , "ascii:Syntax Error\n");
			return;
		}
		ASC = atoi(Parameter , 10);
		Printf(Color , "ASCIICode:%d | Charactor:'%c'\n" , ASC , ASC);
		return;
	}
	else {
		Printf(Color , "ascii:Syntax Error\n");
		return;
	}
}

static void Calculater(CONSOLECOLOR Color , const char *ParameterBuffer) {
	int i = 0;
	int j = 0;
	char Option[1024];
	char Arg1[20];
	double Result = 0.0000;
	PARAMETERLIST List;
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , Option) == 0) {
		Printf(Color , "calc:Syntax Error\n");
		return;
	}
	if(GetNextParameter(&List , Arg1) == 0) {
		Printf(Color , "calc:Syntax Error\n");
		return;
	}
	int k;
	for(k = 0; k < 300; k++) {
		if(IsEqual(Option , "+") == TRUE) {
			i = atoi(Arg1 , 10);
			Result += (double)i;
			if(GetNextParameter(&List , Arg1) == 0) {
				break;
			}
		}
		else if(IsEqual(Option , "-") == TRUE) {
			i = atoi(Arg1 , 10);
			Result -= (double)i;
			if(GetNextParameter(&List , Arg1) == 0) {
				if(k == 1) {
					i = atoi(Arg1 , 10);
					Result = -(i);
				}
					
				break;
			}
		}
		else if(IsEqual(Option , "*") == TRUE) {
			i = atoi(Arg1 , 10);
			if(Result == 0) {
				Result = (double)i;
			}
			else {
				Result *= (double)i;
			}
			if(GetNextParameter(&List , Arg1) == 0) {
				break;
			}
		}
		else if(IsEqual(Option , "/") == TRUE) {
			i = atoi(Arg1 , 10);
			if(Result == 0) {
				Result = (double)i;
			}
			else {
				Result /= (double)i;
			}
			if(i == 0) {
				Printf(Color , "[!] Division Error.!\n");
				break;
			}
			if(GetNextParameter(&List , Arg1) == 0) {
				break;
			}
		}
		else {
			Printf(Color , "calc:Syntax Error\n");
			return;
		}
	}
	Printf(Color , "result:");
	if(Result < 0) {
		Printf(Color , "MINUS");
	}
	else {
		Printf(Color , "%f" , Result);
	}
	Printf(Color , "\n");
}

static void CreateTaskInConsole(CONSOLECOLOR Color , const char *ParameterBuffer) {
	Printf(Color , "\n");
	char Buffer[20];
	PARAMETERLIST List;
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , Buffer) == 0) {
		return;
	}
	int Count = 0;
	Count = atoi(Buffer , 10);
	int i;
	for(i = 0; i < Count; i++) {
		if(CreateTask(TASK_FLAGS_LOW|TASK_FLAGS_THREAD , 0 , 0 , (QWORD)TestTask , TASK_LOADBALANCINGID) == 0) {
			break;
		}
	}
}

static void CheckFileSystem(CONSOLECOLOR Color , const char *ParameterBuffer) {
	int i;
	FILE *LogFileSave;
	LogFileSave = fopen("FSCHECK.LOG" , "w");
	fprintf(LogFileSave , "ChoOS File System Checking Program Version 1.0\n");
	fprintf(LogFileSave , "Copyright 2017-2018 ChoOS Co. All rights reserved.\n\n");
	BOOL Error = FALSE;
	Printf(Color , "<File System Checker Version 1.0>\n\n");
	Printf(Color , "1. Create Empty File\n");
	fprintf(LogFileSave , "1. Create Empty File\n");
	FILE *Check;
	Check = fopen("CHOOSFSCHECK1.EMPTY" , "w");
	if(Check == NULL) {
		Printf(Color , "FAIL(Create)\n");
		fprintf(LogFileSave , "FAIL(Create)\n");
		Error = TRUE;
	}
	else {
		Printf(Color , "SUCCESS(Create)\n");
		fprintf(LogFileSave , "SUCCESS(Create)\n");
	}
	fclose(Check);
	Printf(Color , "2. Create Text File\n");
	fprintf(LogFileSave , "2. Create Text File\n");
	Check = fopen("CHOOFSCHECK2.TEXT" , "w");
	if(Check == NULL) {
		Printf(Color , "FAIL(Create) , ");
		fprintf(LogFileSave , "FAIL(Create) , ");
		Error = TRUE;
	}
	else {
		Printf(Color , "SUCCESS(Create) , ");
		fprintf(LogFileSave , "SUCCESS(Create) , ");
	}
	if(fprintf(Check , "Hello, World ChoOS!!") == 0) {
		Printf(Color , "FAIL(Write)\n");
		fprintf(LogFileSave , "FAIL(Write)\nString:\"Hello, World ChoOS!!\"\n");
	}
	else {
		Printf(Color , "SUCCESS(Write)\n");
		fprintf(LogFileSave , "SUCCESS(Write)\nString:\"Hello, World ChoOS!!\"\n");
	}
	fclose(Check);
	Printf(Color , "3. Create Large Size File\n");
	fprintf(LogFileSave , "3. Create Large Size File\n");
	Check = fopen("CHOOSFSCHECK3.LARGE" , "w");
	if(Check == NULL) {
		Printf(Color , "FAIL(Create) , ");
		fprintf(LogFileSave , "FAIL(Create) , ");
		Error = TRUE;
	}
	else {
		Printf(Color , "SUCCESS(Create) , ");
		fprintf(LogFileSave , "SUCCESS(Create) , ");
	}
	if(fprintf(Check , "Hello, World ChoOS!!\n") == 0) {
		Printf(Color , "FAIL(Write)\n");
		fprintf(LogFileSave , "FAIL(Write)\n");
		Error = TRUE;
	}
	else {
		Printf(Color , "SUCCESS(Write)\n");
		fprintf(LogFileSave , "SUCCESS(Write)\n");
	}
	int TX;
	int TY;
	GetCursor(&TX , &TY);
	for(i = 0 ; i <= 90; i++) {
		int f;
		SetCursor(TX , TY);
		Printf(Color , "WRITE OFFSET:%d\n" , i);
		for(f = 0; f < 10; f++) {
			if((fprintf(Check , "%c" , Random())) == 0) {
				Printf(Color , "FAILED.\n\n");
				goto ERR;
			}
			SetCursor(TX , TY+1);
			Printf(Color , "SUCCESS.\n");
		}
	}
	fclose(Check);
	Printf(Color , "4. Remove Empty File\n");
	fprintf(LogFileSave , "4. Remove Empty File\n");
	if(remove("CHOOSFSCHECK1.EMPTY") == -1) {
		Printf(Color , "FAIL\n");
		fprintf(LogFileSave , "FAIL\n");
		Error = TRUE;
	}
	else {
		Printf(Color , "SUCCESS\n");
		fprintf(LogFileSave , "SUCCESS\n");
	}
	Printf(Color , "5. Remove Text File\n");
	fprintf(LogFileSave , "5. Remove Text File\n");
	if(remove("CHOOFSCHECK2.TEXT") == -1) {
		Printf(Color , "FAIL\n");
		fprintf(LogFileSave , "FAIL\n");
		Error = TRUE;
	}
	else {
		Printf(Color , "SUCCESS\n");
		fprintf(LogFileSave , "SUCCESS\n");
	}
	Printf(Color , "6. Remove Large Size File\n");
	fprintf(LogFileSave , "6. Remove Large Size File\n");
	if(remove("CHOOSFSCHECK3.LARGE") == -1) {
		Printf(Color , "FAIL\n");
		fprintf(LogFileSave , "FAIL\n");
		Error = TRUE;
	}
	else {
		Printf(Color , "SUCCESS\n");
		fprintf(LogFileSave , "SUCCESS\n");
	}
	Printf(Color , "6. Save File System Cache in Hard Disk\n");
	fprintf(LogFileSave , "6. Save File System Cache in Hard Disk\n");
	if(FlushFileSystemCache() == FALSE) {
		Printf(Color , "FAIL\n");
		fprintf(LogFileSave , "FILE\n");
		Error = TRUE;
	}
	else {
		Printf(Color , "SUCCESS\n");
		fprintf(LogFileSave , "SUCCESS\n");
	}
	if(Error == TRUE) {
ERR:
		Printf(Color , "File System Test is Failed.\n");
		Printf(Color , "Shutdown or Restart your PC and try again.\n");
		Printf(Color , "File System Check log file name:FSCheck.log\n");
		return;
	}
	else {
		Printf(Color , "File System Test is Successed.\n");
		Printf(Color , "File System Check log file name:FSCHECK.LOG\n");
	}
}

static void ReadFileInConsole(CONSOLECOLOR Color , const char *ParameterBuffer) {
	PARAMETERLIST List;
	char FileKey;
	char FileName[300];
	char Option[300];
	int Length;
	InitParameter(&List , ParameterBuffer);
	Length = GetNextParameter(&List , FileName);
	if(Length == 0) {
		Printf(Color , "readfile:Syntax Error\n");
		return;
	}
	FileName[Length] = '\0';
	FILE *FileOpen;
	FileOpen = fopen(FileName , "r");
	if(FileOpen == 0) {
		Printf(Color , "readfile:File not Found\n");
		return;
	}
	int ChangeLine = 0;
	while(1) {
		if(((ChangeLine%5) == 0) && (ChangeLine != 0)) {
			if(Paused(Color) == TRUE) {
				return;
			}
			FileKey = NULL;
			ChangeLine = 0;
		}
		if(fread(&FileKey , 1 , 1 , FileOpen) != 1) {
			break;
		}
		if(FileKey == '\n') {
			ChangeLine++;
		}
		Printf(Color , "%c" , FileKey);
	}
	Printf(Color , "\n");
	fclose(FileOpen);
}

static void CreateFileInConsole(CONSOLECOLOR Color , const char *ParameterBuffer) {
	PARAMETERLIST List;
	int i;
	char FileKey;
	char FileName[300];
	int Length;
	InitParameter(&List , ParameterBuffer);
	Length = GetNextParameter(&List , FileName);
	if(Length == 0) {
		Printf(Color , "createfile:Syntax Error\n");
		return;
	}
	FileName[Length] = '\0';
	FILE *FileOpen;
	FileOpen = fopen(FileName , "w");
	if(FileOpen == NULL) {
		Printf(Color , "createfile:File Create Failed.\n");
		return;
	}
	Printf(Color , "if you Exit then, press <ESC> Key to Exit.\n\n");
	char Buffer[1024] = {0 , };
	Gets(Color , KEY_ESC , Buffer , DEFAULT);
	fprintf(FileOpen , "%s\n" , Buffer);
ESCAPE:
	fclose(FileOpen);
	FlushFileSystemCache();
	Printf(Color , "\n");
	for(i = 0; i < 80; i++) {
		Printf(Color , "=");
	}
	Printf(Color , "\n");
	Printf(Color , "File Create/Write Success.\n");
}

static void ShowMPConfigurationTable(CONSOLECOLOR Color , const char *ParameterBuffer) {
	PrintMPConfigurationTable(Color);
}

static void SaveF(CONSOLECOLOR Color , const char *ParameterBuffer) {
	if(FlushFileSystemCache() == FALSE) {
		Printf(Color , "File System Cache Flushing Failed.\n");
	}
	else {
		Printf(Color , "File System Cache Flushing Successed.\n");
	}
	return;
}

static void DownloadFileInSerialPort(CONSOLECOLOR Color , const char *ParameterBuffer) {
	PARAMETERLIST List;
	char FileName[50];
	int FileNameLength;
	DWORD DataLength;
	FILE *File;
	DWORD ReceivedSize;
	DWORD TempSize;
	BYTE DataBuffer[SERIAL_FIFOMAXSIZE];
	QWORD LastReceivedTickCount;
	InitParameter(&List , ParameterBuffer);
	FileNameLength = GetNextParameter(&List , FileName);
	FileName[FileNameLength] = '\0';
	if((FileNameLength > (FILESYSTEM_MAXFILENAMELENGTH-1))||(FileNameLength == 0)) {
		Printf(Color , "download:Syntax Error\n");
		return;
	}
	ClearSerialFIFO();
	Printf(Color , "<ChoOS Serial Port Downloader>\n\n");
	Printf(Color , "Waiting . . . . .");
	ReceivedSize = 0;
	LastReceivedTickCount = GetTickCount();
	while(ReceivedSize < 4) {
		TempSize = ReceiveSerialData(((BYTE*)&DataLength)+ReceivedSize , 4-ReceivedSize);
		ReceivedSize += TempSize;
		if(TempSize == 0) {
			Sleep(0);
			if((GetTickCount()-LastReceivedTickCount) > 30000) {
				Printf(Color , "\n<Time Out>\nExit.");
				return;
			}
			else {
				LastReceivedTickCount = GetTickCount();
			}
		}
	}
	Printf(Color , "\nConnecting Success.\n<Data Recevived Started>\n\nStatus:" , DataLength);
	SendSerialData("A" , 1);
	File = fopen(FileName , "w");
	if(File == NULL) {
		Printf(Color , "<File Create Failed>\nExit.");
		return;
	}
	ReceivedSize = 0;
	LastReceivedTickCount = GetTickCount();
	int X;
	int Y;
	GetCursor(&X , &Y);
	while(ReceivedSize < DataLength) {
		TempSize = ReceiveSerialData(DataBuffer , SERIAL_FIFOMAXSIZE);
		ReceivedSize += TempSize;
		if(TempSize != 0) {
			if(((ReceivedSize % SERIAL_FIFOMAXSIZE) == 0)||((ReceivedSize == DataLength))) {
				SendSerialData("A" , 1);
				SetCursor(X , Y);
				Printf(Color , "%dB/%dB" , ReceivedSize , DataLength);
			}
			if(fwrite(DataBuffer , 1 , TempSize , File) != TempSize) {
				Printf(Color , "\n<File Write Error>\nExit.");
				break;
			}
			LastReceivedTickCount = GetTickCount();
		}
		else {
			Sleep(0);
			if((GetTickCount()-LastReceivedTickCount) > 10000) {
				Printf(Color , "\n<Time Out>\nExit.");
				break;
			}
		}
	}
	if(ReceivedSize != DataLength) {
		Printf(Color , "\n");
		Printf(Color , "Error Occur. Total Size : %d\n" , DataLength);
		Printf(Color , "Received Size : %d\n" , ReceivedSize);
	}
	else {
		Printf(Color , "\n");
		Printf(Color , "Receive Complete. Total Size : %d\n" , DataLength);
	}
	fclose(File);
	FlushFileSystemCache();
}

static void CopyFileInConsole(CONSOLECOLOR Color , const char *ParameterBuffer) {
	PARAMETERLIST List;
	char FileName1[30];
	char FileName2[30];
	char FileName3[30];
	char Option[30];
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , FileName2) == 0) {
		Printf(Color , "copyfile:Syntax Error\n");
		return;
	}
	if(GetNextParameter(&List , FileName1) == 0) {
		Printf(Color , "copyfile:Syntax Error\n");
	}
	if(GetNextParameter(&List , Option) == 0) {
		FILE *TF;
		FILE *SF;
		TF = fopen(FileName2 , "r");
		SF = fopen(FileName1 , "w");
		if(TF == 0) {
			Printf(Color , "copyfile:File Copying Failed\n");
			return;
		}
		Printf(Color , "Source:%s\nTarget:%s\n\nCopying File . . . . . " , FileName2 , FileName1);
		while(1) {
			char Buffer;
			if(fread(&Buffer , 1 , 1 , TF) != 1) {
				break;
			}
			if(fwrite(&Buffer , 1 , 1 , SF) != 1) {
				break;
			}
		}
		fclose(TF);
		fclose(SF);
		if(GetFileSize(FileName1) == GetFileSize(FileName2)) {
			Printf(Color , "\nFile Copying Successed\n");
		}
		else {
			Printf(Color , "File Copying Failed\n");
		}
		fclose(SF);
		fclose(TF);
		return;
	}
	else if(IsEqual(Option , "/b") == TRUE) {
		if(GetNextParameter(&List , FileName3) == 0) {
			Printf(Color , "copyfile:Syntax Error\n");
			return;
		}
		FILE *TF;
		FILE *SF;
		FILE *MF;
		SF = fopen(FileName1 , "r");
		TF = fopen(FileName2 , "r");
		if(TF == 0) {
			Printf(Color , "File Copying Failed(File Not Found:%s)\n" , FileName2);
			fclose(SF);
			fclose(TF);
			return;
		}
		if(SF == 0) {
			Printf(Color , "File Copying Failed(File Not Found:%s)\n" , FileName1);
			fclose(SF);
			fclose(TF);
			return;
		}
		MF = fopen(FileName3 , "w");
		Printf(Color , "File %s + File %s => %s\n\nCopying File . . . . . \n" , FileName1 , FileName2 , FileName3);
		while(1) {
			char Buffer;
			if(fread(&Buffer , 1 , 1 , TF) != 1) {
				break;
			}
			if(fwrite(&Buffer , 1 , 1 , MF) != 1) {
				break;
			}
		}
		while(1) {
			char Buffer;
			if(fread(&Buffer , 1 , 1 , SF) != 1) {
				break;
			}
			if(fwrite(&Buffer , 1 , 1 , MF) != 1) {
				break;
			}
		}
		fclose(SF);
		fclose(TF);
		fclose(MF);
		if((GetFileSize(FileName1)+GetFileSize(FileName2)) == GetFileSize(FileName3)) {
			Printf(Color , "\nFile Copying Successed\n");
		}
		else {
			Printf(Color , "File Copying Failed\n");
		}
		fclose(SF);
		fclose(TF);
		return;
	}
}

static void ExecuteConsoleShellScript(CONSOLECOLOR Color , const char *ParameterBuffer) {
	PARAMETERLIST List;
	char FileName[30];
	InitParameter(&List , ParameterBuffer);
	if(GetNextParameter(&List , FileName) == 0) {
		Printf(Color , "cs:Syntax Error\n");
		return;
	}
	FILE *F;
	F = fopen(FileName , "r");
	if(F == NULL) {
		Printf(Color , "cs:File Not Found\n");
		return;
	}
	while(1) {
		char Buffer[300] = {0 , };
		int BufferIndex = 0;
		while(1) {
			char cBuffer;
			if(fread(&cBuffer , 1 , 1 , F) != 1) {
				break;
			}
			if(cBuffer == '\n') {
				break;
			}
			Buffer[BufferIndex++] = cBuffer;
		}
		if((BufferIndex == 0)||(IsEqual(Buffer , "\n") == TRUE)) {
			break;
		}
		ExecuteCommandInCS(Color , Buffer);
	}
	fclose(F);
	return;
}

static void Comments(CONSOLECOLOR Color , const char *ParameterBuffer) {
	;
}

static void Edit(CONSOLECOLOR Color , const char *ParameterBuffer) {
	int i;
	int j;
	int EditCharWidth;
	int EditCharHeight;
	char Checker[1024][1024];
	int LineLengthChecker[24];
	int INDEX = 0;
	CHARACTER *Screen = (CHARACTER*)0xB8000;
	ClearScreen(Color);
	SetCursor(0 , 0);
	for(i = 0; i < 80; i++) {
		Printf(0x20 , " ");
	}
	SetCursor(0 , 0);
	Printf(0x21 , "ChoOS Editor Version 1.0");
	SetCursor(0 , 1);
	Printf(0x17 , "New File(F1)   Open File(F2)   Save(F3)   Save As(F4)");
	SetCursor(0 , 2);
	Printf(0x17 , "File Information(F5)   Help(F6)   Exit(ESC)");
	SetCursor(0 , 3);
	int X;
	int Y;
	GetCursor(&X , &Y);
	while(1) {
		SetCursor(X , Y);
		BYTE TempBuffer = GetCh();
		switch(TempBuffer) {
			case '\n':
				break;
			case '\b':
				break;
			case KEY_CTRL:
			case KEY_LSHIFT:
			case KEY_RSHIFT:
			case KEY_LALT:
			case KEY_CAPSLOCK:
			case KEY_NUMLOCK:
			case KEY_SCROLLLOCK:
			case KEY_TAB:
			case KEY_F7:
			case KEY_F8:
			case KEY_F9:
			case KEY_F10:
			case KEY_F11:
			case KEY_F12:
				break;
			case KEY_F1:
				break;
			case KEY_F2:
				break;
			case KEY_F3:
				break;
			case KEY_F4:
				break;
			case KEY_F5:
				break;
			case KEY_F6:
				break;
			case KEY_ESC:
				goto EXIT;
			case KEY_LEFT:
				X--;
				break;
			case KEY_RIGHT:
				X++;
				break;
			default:
				if(X+1 > 80) {
					X = 0;
					Y++;
				}
				else {
					X++;
				}
				SetCursor(X , Y);
				Screen[CONVERTXY(X , Y)].Charactor = TempBuffer;
				break;
		}
		if((X <= -1 && Y <= 3)) {
			X++;
			Y++;
		}
	}
EXIT:
	Printf(Color , "\"");
	for(i = 0; i <= 10; i++) {
		if(Screen[CONVERTXY(i , 3)].Charactor == 0) {
			Printf(0x07 , " ");
		}
		else {
			Printf(0x07 , "%c" , Screen[CONVERTXY(i , 3)].Charactor);
		}
	}
}
