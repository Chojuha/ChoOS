#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MultiProcessor.h"
#include "VBE.h"
#include "2DGraphics.h"
#include "MPConfigurationTable.h"
#include "Mouse.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "GUITask.h"

void MainForApplicationProcessor(void);
BOOL ChangeToMultiCoreMode(void);

void Main(void) {
	RECT ScreenArea;
	COLOR *VideoMemory;
	VBEMODEINFOBLOCK *VBEMode;
	VBEMode = GetVBEInfoBlock();
	ScreenArea.X1 = 0;
	ScreenArea.Y1 = 0;
	ScreenArea.X2 = VBEMode->XResolution-1;
	ScreenArea.Y2 = VBEMode->YResolution-1;
	VideoMemory = (COLOR*)((QWORD)VBEMode->PhysicalBasePointer & 0xFFFFFFFF);
	if(*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0) {
		MainForApplicationProcessor();
	}
	*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) = 0;
	InitConsole(0 , 3);
	InitGDTTableAndTSS();
	LoadGDTR(GDTR_STARTADDRESS);
	LoadTR(GDT_TSSSEGMENT);
	InitIDTTable();
	LoadIDTR(IDTR_STARTADDRESS);
	CheckTotalRamSize();
	InitScheduler();
	InitDynamicMemory();
	InitPIT(MSTOCOUNT(1) , TRUE);
	if(InitKeyboard() == TRUE) {
		ChangeKeyboardLED(FALSE , FALSE , FALSE);
	}
	else {
		
		while(1);
	}
	if(InitMouse() == TRUE) {
		EnableMouseInterrupt();
	}
	else {
		while(1);
	}
	InitPIC();
	MaskPICInterrupt(0);
	EnableInterrupt();
	if(InitFileSystem() == FALSE) {
		;
	}
	InitSerialPort();
	if(ChangeToMultiCoreMode() == TRUE) {
		;
	}
	else {
		;
	}
	ClearScreen(COLOR_BLUE_WHITE);
	CreateTask(TASK_FLAGS_LOWEST|TASK_FLAGS_THREAD|TASK_FLAGS_SYSTEM|TASK_FLAGS_IDLE , 0 , 0 , (QWORD)IdleTask , GetAPICID());
	if(*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0) {
		StartConsoleShell(COLOR_BLUE_WHITE);
	}
	else {
		StartWindowManager();
	}
}

void MainForApplicationProcessor(void) {
	QWORD TickCount;
	LoadGDTR(GDTR_STARTADDRESS);
	LoadTR(GDT_TSSSEGMENT+(GetAPICID()*sizeof(GDTENTRY16)));
	LoadIDTR(IDTR_STARTADDRESS);
	InitScheduler();
	EnableSoftwareLocalAPIC();
	SetTaskPriority(0);
	InitLocalVectorTable();
	EnableInterrupt();
	IdleTask();
}

BOOL ChangeToMultiCoreMode(void) {
	MPCONFIGURATIONMANAGER *MPManager;
	BOOL InterruptFlag;
	int i;
	if(StartUpApplicationProcessor(NULL) == FALSE) {
		return FALSE;
	}
	MPManager = GetMPConfigurationManager();
	if(MPManager->UsePICMode == TRUE) {
		OutPortByte(0x22 , 0x70);
		OutPortByte(0x23 , 0x01);
	}
	MaskPICInterrupt(0xFFFF);
	EnableGlobalLocalAPIC();
	EnableSoftwareLocalAPIC();
	InterruptFlag = SetInterruptFlag(FALSE);
	SetTaskPriority(0);
	InitLocalVectorTable();
	SetSymmetricIOMode(TRUE);
	InitIORedirectionTable();
	SetInterruptFlag(InterruptFlag);
	SetInterruptLoadBalancing(TRUE);
	for(i = 0; i < MAXPROCESSORCOUNT; i++) {
		SetTaskLoadBalancing(i , TRUE);
	}
	return TRUE;
}

/*

BYTE MouseBuffer[MOUSE_CURSOR_WIDTH*MOUSE_CURSOR_HEIGHT] = {
	0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 
	0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 
	0 , 0 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 
	0 , 0 , 1 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 1 , 0 , 0 , 0 , 
	0 , 0 , 1 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 1 , 0 , 0 , 0 , 0 , 
	0 , 0 , 1 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 1 , 0 , 0 , 0 , 0 , 0 , 
	0 , 0 , 1 , 2 , 2 , 2 , 2 , 2 , 2 , 1 , 1 , 0 , 0 , 0 , 0 , 0 , 
	0 , 0 , 1 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 1 , 1 , 0 , 0 , 0 , 0 , 
	0 , 0 , 1 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 1 , 1 , 0 , 0 , 0 , 
	0 , 0 , 1 , 2 , 2 , 2 , 1 , 2 , 2 , 2 , 2 , 2 , 1 , 1 , 0 , 0 , 
	0 , 0 , 1 , 2 , 2 , 1 , 1 , 1 , 2 , 2 , 2 , 2 , 2 , 1 , 1 , 0 , 
	0 , 0 , 1 , 2 , 1 , 0 , 0 , 1 , 1 , 2 , 2 , 2 , 2 , 2 , 1 , 1 , 
	0 , 0 , 1 , 1 , 0 , 0 , 0 , 0 , 1 , 1 , 2 , 2 , 2 , 1 , 1 , 0 , 
	0 , 0 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 2 , 1 , 1 , 0 , 0 , 
	0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 1 , 0 , 0 , 0 , 
	0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 0 , 0 , 0 , 0 , 
};

void DrawCursor(RECT *Area , COLOR *VideoMemory , int X , int Y) {
	int i;
	int j;
	BYTE *CurrentPos;
	CurrentPos = MouseBuffer;
	for(j = 0; j < MOUSE_CURSOR_HEIGHT; j++) {
		for(i = 0; i < MOUSE_CURSOR_WIDTH; i++) {
			switch(*CurrentPos) {
				case 0:
					break;
				case 1:
					InternalDrawPixel(Area , VideoMemory , i+X , j+Y , RGB(00 , 00 , 0xFF));
					break;
				case 2:
					InternalDrawPixel(Area , VideoMemory , i+X , j+Y , RGB(00 , 0xFF , 00));
					break;
			}
			CurrentPos++;
		}
	}
}	

void GraphicsTest(void) {
	RECT ScreenArea;
	COLOR *VideoMemory;
	int X;
	int Y;
	BYTE Button;
	int RelativeX;
	int RelativeY;
	VBEMODEINFOBLOCK *VBEMode;
	VBEMode = GetVBEInfoBlock();
	ScreenArea.X1 = 0;
	ScreenArea.Y1 = 0;
	ScreenArea.X2 = VBEMode->XResolution-1;
	ScreenArea.Y2 = VBEMode->YResolution-1;
	VideoMemory = (COLOR*)((QWORD)VBEMode->PhysicalBasePointer & 0xFFFFFFFF);
	X = VBEMode->XResolution/2;
	Y = VBEMode->YResolution/2;
	DrawCursor(&ScreenArea , VideoMemory , X , Y);
	int T = 0;
	while(1) {
		if(GetMouseDataFromMouseQueue(&Button , &RelativeX , &RelativeY) == FALSE) {
			Sleep(0);
			continue;
		}
		InternalDrawRect(&ScreenArea , VideoMemory , X , Y , X+MOUSE_CURSOR_WIDTH , Y+MOUSE_CURSOR_HEIGHT , RGB(00 , 0xFF , 0xFF) , TRUE);
		X += RelativeX;
		Y += RelativeY;
		if(X < ScreenArea.X1) {
			X = ScreenArea.X1;
		}
		else if(X > ScreenArea.X2) {
			X = ScreenArea.X2;
		}
		if(Y < ScreenArea.Y1) {
			Y = ScreenArea.Y1;
		}
		else if(Y > ScreenArea.Y2) {
			Y = ScreenArea.Y2;
		}
		if(Button & MOUSE_LBUTTONDOWN) {
			CreateTestWindow(ScreenArea , VideoMemory , X , Y);
		}
		if(Button & MOUSE_RBUTTONDOWN) {
			InternalDrawRect(&ScreenArea , VideoMemory , ScreenArea.X1 , ScreenArea.Y1 , ScreenArea.X2 , ScreenArea.Y2 , RGB(00 , 0xFF , 0xFF) , TRUE);
		}
		DrawCursor(&ScreenArea , VideoMemory , X , Y);
	}
}

void TT(void) {
	RECT ScreenArea;
	COLOR *VideoMemory;
	BYTE Button;
	int RelativeX;
	int RelativeY;
	VBEMODEINFOBLOCK *VBEMode;
	VBEMode = GetVBEInfoBlock();
	ScreenArea.X1 = 0;
	ScreenArea.Y1 = 0;
	ScreenArea.X2 = VBEMode->XResolution-1;
	ScreenArea.Y2 = VBEMode->YResolution-1;
	VideoMemory = (COLOR*)((QWORD)VBEMode->PhysicalBasePointer & 0xFFFFFFFF);
	while(1) {
		InternalDrawText(&ScreenArea , VideoMemory , 0 , 20 , RGB(00 , 00 , 00) , RGB(00 , 0xFF , 0xFF) , "System is Working -" , 20);
		Sleep(100);
		InternalDrawText(&ScreenArea , VideoMemory , 0 , 20 , RGB(00 , 00 , 00) , RGB(00 , 0xFF , 0xFF) , "System is Working \\" , 20);
		Sleep(100);
		InternalDrawText(&ScreenArea , VideoMemory , 0 , 20 , RGB(00 , 00 , 00) , RGB(00 , 0xFF , 0xFF) , "System is Working |" , 20);
		Sleep(100);
		InternalDrawText(&ScreenArea , VideoMemory , 0 , 20 , RGB(00 , 00 , 00) , RGB(00 , 0xFF , 0xFF) , "System is Working /" , 20);
		Sleep(100);
	}
}

*/
