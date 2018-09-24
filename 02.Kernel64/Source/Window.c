#include "Window.h"
#include "VBE.h"
#include "Task.h"
#include "Font.h"
#include "DynamicMemory.h"
#include "Utility.h"

static WINDOWPOOLMANAGER WindowPoolManager;
static WINDOWMANAGER WindowManager;

static void InitWindowPool(void) {
	int i;
	void *WindowPoolAddress;
	MemSet(&WindowPoolManager , 0 , sizeof(WindowPoolManager));
	WindowPoolAddress = (void*)AllocateMemory(sizeof(WINDOW)*WINDOW_MAXCOUNT);
	if(WindowPoolAddress == NULL) {
		Printf(0x17 , "Window Pool Allocate Fail.\n");
		while(1) {
			;
		}
	}
	WindowPoolManager.StartAddress = (WINDOW*)WindowPoolAddress;
	MemSet(WindowPoolAddress , 0 , sizeof(WINDOW)*WINDOW_MAXCOUNT);
	for(i = 0; i < WINDOW_MAXCOUNT; i++) {
		WindowPoolManager.StartAddress[i].Link.ID = i;
	}
	WindowPoolManager.MaxCount = WINDOW_MAXCOUNT;
	WindowPoolManager.AllocatedCount = 1;
	InitMutex(&(WindowPoolManager.Lock));
}

static WINDOW *AllocateWindow(void) {
	WINDOW *EmptyWindow;
	int i;
	Lock(&(WindowPoolManager.Lock));
	if(WindowPoolManager.UseCount == WindowPoolManager.MaxCount) {
		UnLock(&WindowPoolManager.Lock);
		return NULL;
	}
	for(i = 0; i < WindowPoolManager.MaxCount; i++) {
		if((WindowPoolManager.StartAddress[i].Link.ID >> 32) == 0) {
			EmptyWindow = &(WindowPoolManager.StartAddress[i]);
			break;
		}
	}
	EmptyWindow->Link.ID = ((QWORD)WindowPoolManager.AllocatedCount << 32)|i;
	WindowPoolManager.UseCount++;
	WindowPoolManager.AllocatedCount++;
	if(WindowPoolManager.AllocatedCount == 0) {
		WindowPoolManager.AllocatedCount = 1;
	}
	UnLock(&(WindowPoolManager.Lock));
	InitMutex(&(EmptyWindow->Lock));
	return EmptyWindow;
}

static void FreeWindow(QWORD ID) {
	int i;
	i = GETWINDOWOFFSET(ID);
	Lock(&(WindowPoolManager.Lock));
	MemSet(&(WindowPoolManager.StartAddress[i]) , 0 , sizeof(WINDOW));
	WindowPoolManager.StartAddress[i].Link.ID = i;
	WindowPoolManager.UseCount--;
	UnLock(&(WindowPoolManager.Lock));
}

void InitGUISystem(void) {
	VBEMODEINFOBLOCK *ModeInfo;
	QWORD BackgroundWindowID;
	EVENT *EventBuffer;
	InitWindowPool();
	ModeInfo = GetVBEInfoBlock();
	WindowManager.VideoMemory = (COLOR*)((QWORD)ModeInfo->PhysicalBasePointer & 0xFFFFFFFF);
	WindowManager.MouseX = ModeInfo->XResolution/2;
	WindowManager.MouseY = ModeInfo->YResolution/2;
	WindowManager.ScreenArea.X1 = 0;
	WindowManager.ScreenArea.Y1 = 0;
	WindowManager.ScreenArea.X2 = ModeInfo->XResolution-1;
	WindowManager.ScreenArea.Y2 = ModeInfo->YResolution-1;
	InitMutex(&(WindowManager.Lock));
	InitList(&(WindowManager.WindowList));
	EventBuffer = (EVENT*)AllocateMemory(EVENTQUEUE_WINDOWMANAGERMAXCOUNT*sizeof(EVENT));
	if(EventBuffer == NULL) {
		Printf(0x17 , "Window Manager Event Queue Allocate Fail.\n");
		while(1) {
			;
		}
	}
	InitQueue(&(WindowManager.EventQueue) , EventBuffer , EVENTQUEUE_WINDOWMANAGERMAXCOUNT , sizeof(EVENT));
	WindowManager.PrevButtonStatus = 0;
	WindowManager.WindowMoveMode = FALSE;
	WindowManager.MovingWindowID = WINDOW_INVALIDID;
	
	BackgroundWindowID = CreateWindow(0 , 0 , ModeInfo->XResolution , ModeInfo->YResolution , 0 , WINDOW_BACKGROUNDWINDOWTITLE);
	WindowManager.BackgroundWindowID = BackgroundWindowID;
	DrawRect(BackgroundWindowID , 0 , 0 , ModeInfo->XResolution-1 , ModeInfo->YResolution-1 , WINDOW_COLOR_SYSTEMBACKGROUND , TRUE);
	ShowWindow(BackgroundWindowID , TRUE);
}

WINDOWMANAGER *GetWindowManager(void) {
	return &WindowManager;
}

QWORD GetBackgroundWindowID(void) {
	return WindowManager.BackgroundWindowID;
}

void GetScreenArea(RECT *ScreenArea) {
	MemCpy(ScreenArea , &(WindowManager.ScreenArea) , sizeof(RECT));
}

QWORD CreateWindow(int X , int Y , int Width , int Height , DWORD Flags , const char *Title) {
	WINDOW *Window;
	TCB *Task;
	QWORD ActiveWindowID;
	EVENT Event;
	if((Width <= 0)||(Height <= 0)) {
		return WINDOW_INVALIDID;
	}
	Window = AllocateWindow();
	if(Window == NULL) {
		return WINDOW_INVALIDID;
	}
	Window->Area.X1 = X;
	Window->Area.Y1 = Y;
	Window->Area.X2 = X+Width-1;
	Window->Area.Y2 = Y+Height-1;
	MemCpy(Window->WindowTitle , Title , WINDOW_TITLEMAXLENGTH);
	Window->WindowTitle[WINDOW_TITLEMAXLENGTH] = '\0';
	Window->WindowBuffer = (COLOR*)AllocateMemory(Width*Height*sizeof(COLOR));
	Window->EventBuffer = (EVENT*)AllocateMemory(EVENTQUEUE_WINDOWMAXCOUNT*sizeof(EVENT));
	if((Window->WindowBuffer == NULL)||(Window->EventBuffer == NULL)) {
		FreeMemory(Window->WindowBuffer);
		FreeMemory(Window->EventBuffer);
		FreeWindow(Window->Link.ID);
		return WINDOW_INVALIDID;
	}
	InitQueue(&(Window->EventQueue) , Window->EventBuffer , EVENTQUEUE_WINDOWMAXCOUNT , sizeof(EVENT));
	Task = GetRunningTask(GetAPICID());
	Window->TaskID = Task->Link.ID;
	Window->Flags = Flags;
	if(Flags & WINDOW_FLAGS_DRAWFRAME) {
		DrawWindowFrame(Window->Link.ID);
	}
	if(Flags & WINDOW_FLAGS_DRAWTITLE) {
		DrawWindowTitle(Window->Link.ID , Title , TRUE);
	}
	if(Flags & WINDOW_FLAGS_DRAWFRAME) {
		DrawWindowLastOne(Window->Link.ID);
	}
	DrawWindowBackground(Window->Link.ID);
	Lock(&(WindowManager.Lock));
	ActiveWindowID = GetTopWindowID();
	AddListToTail(&WindowManager.WindowList , Window);
	UnLock(&(WindowManager.Lock));
	UpdateScreenByID(Window->Link.ID);
	SetWindowEvent(Window->Link.ID , EVENT_WINDOW_SELECT , &Event);
	SendEventToWindow(Window->Link.ID , &Event);
	if(ActiveWindowID != WindowManager.BackgroundWindowID) {
		UpdateWindowTitle(ActiveWindowID , FALSE);
		SetWindowEvent(ActiveWindowID , EVENT_WINDOW_DESELECT , &Event);
		SendEventToWindow(ActiveWindowID , &Event);
	}
	return Window->Link.ID;
}

BOOL DeleteWindow(QWORD WindowID) {
	WINDOW *Window;
	RECT Area;
	QWORD ActivateWindowID;
	BOOL ActivateWindow;
	EVENT Event;
	Lock(&(WindowManager.Lock));
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		UnLock(&(WindowManager.Lock));
		return FALSE;
	}
	MemCpy(&Area , &(Window->Area) , sizeof(RECT));
	ActivateWindowID = GetTopWindowID();
	if(ActivateWindowID == WindowID) {
		ActivateWindow = TRUE;
	}
	else {
		ActivateWindow = FALSE;
	}
	if(RemoveList(&(WindowManager.WindowList) , WindowID) == NULL) {
		UnLock(&(Window->Lock));
		UnLock(&(WindowManager.Lock));
		return FALSE;
	}
	FreeMemory(Window->WindowBuffer);
	Window->WindowBuffer = NULL;
	FreeMemory(Window->EventBuffer);
	Window->EventBuffer = NULL;
	UnLock(&(Window->Lock));
	FreeWindow(WindowID);
	UnLock(&(WindowManager.Lock));
	UpdateScreenByScreenArea(&Area);
	if(ActivateWindow == TRUE) {
		ActivateWindow = GetTopWindowID();
		if(ActivateWindowID != WINDOW_INVALIDID) {
			UpdateWindowTitle(ActivateWindowID , TRUE);
			SetWindowEvent(ActivateWindowID , EVENT_WINDOW_SELECT , &Event);
			SendEventToWindow(ActivateWindowID , &Event);
		}
	}
	return TRUE;
}

BOOL DeleteAllWindowInTaskID(QWORD TaskID) {
	WINDOW *Window;
	WINDOW *NextWindow;
	Lock(&(WindowManager.Lock));
	Window = GetHeaderFromList(&(WindowManager.WindowList));
	while(Window != NULL) {
		NextWindow = GetNextFromList(&(WindowManager.WindowList) , Window);
		if((Window->Link.ID != WindowManager.BackgroundWindowID) && (Window->TaskID == TaskID)) {
			DeleteWindow(Window->Link.ID);
		}
		Window = NextWindow;
	}
	UnLock(&(WindowManager.Lock));
}

WINDOW *GetWindow(QWORD WindowID) {
	WINDOW *Window;
	if(GETWINDOWOFFSET(WindowID) >= WINDOW_MAXCOUNT) {
		return NULL;
	}
	Window = &WindowPoolManager.StartAddress[GETWINDOWOFFSET(WindowID)];
	if(Window->Link.ID == WindowID) {
		return Window;
	}
	return NULL;
}

WINDOW *GetWindowWithWindowLock(QWORD WindowID) {
	WINDOW *Window;
	BOOL Result;
	Window = GetWindow(WindowID);
	if(Window == NULL) {
		return NULL;
	}
	Lock(&(Window->Lock));
	Window = GetWindow(WindowID);
	if((Window == NULL)||(Window->EventBuffer == NULL)||(Window->WindowBuffer == NULL)) {
		UnLock(&(Window->Lock));
		return NULL;
	}
	return Window;
}

BOOL ShowWindow(QWORD WindowID , BOOL Show) {
	WINDOW *Window;
	RECT WindowArea;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	if(Show == TRUE) {
		Window->Flags |= WINDOW_FLAGS_SHOW;
	}
	else {
		Window->Flags &= ~WINDOW_FLAGS_SHOW;
	}
	UnLock(&(Window->Lock));
	if(Show == TRUE) {
		UpdateScreenByID(WindowID);
	}
	else {
		GetWindowArea(WindowID , &WindowArea);
		UpdateScreenByScreenArea(&WindowArea);
	}
	return TRUE;
}

BOOL ReadrawWindowByArea(const RECT *Area) {
	WINDOW *Window;
	WINDOW *TargetWindow = NULL;
	RECT OverLappedArea;
	RECT CursorArea;
	if(GetOverLappedRectangle(&(WindowManager.ScreenArea) , Area , &OverLappedArea) == FALSE) {
		return FALSE;
	}
	Lock(&(WindowManager.Lock));
	Window = GetHeaderFromList(&(WindowManager.WindowList));
	while(Window != NULL) {
		if((Window->Flags & WINDOW_FLAGS_SHOW) && (IsRectangleOverLapped(&(Window->Area) , &OverLappedArea) == TRUE)) {
			Lock(&(Window->Lock));
			CopyWindowBufferToFrameBuffer(Window , &OverLappedArea);
			UnLock(&(Window->Lock));
		}
		Window = GetNextFromList(&(WindowManager.WindowList) , Window);
	}
	UnLock(&(WindowManager.Lock));
	SetRectangleData(WindowManager.MouseX , WindowManager.MouseY , WindowManager.MouseX+MOUSE_CURSOR_WIDTH , WindowManager.MouseY+MOUSE_CURSOR_HEIGHT , &CursorArea);
	if(IsRectangleOverLapped(&OverLappedArea , &CursorArea) == TRUE) {
		DrawCursor(WindowManager.MouseX , WindowManager.MouseY);
	}
}

static void CopyWindowBufferToFrameBuffer(const WINDOW *Window , const RECT *CopyArea) {
	RECT TempArea;
	RECT OverLappedArea;
	int OverLappedWidth;
	int OverLappedHeight;
	int ScreenWidth;
	int WindowWidth;
	int i;
	COLOR *CurrentVideoMemoryAddress;
	COLOR *CurrentWindowBufferAddress;
	if(GetOverLappedRectangle(&(WindowManager.ScreenArea) , CopyArea , &TempArea) == FALSE) {
		return;
	}
	if(GetOverLappedRectangle(&TempArea , &(Window->Area) , &OverLappedArea) == FALSE) {
		return;
	}
	ScreenWidth = GetRectangleWidth(&(WindowManager.ScreenArea));
	WindowWidth = GetRectangleWidth(&(Window->Area));
	OverLappedWidth = GetRectangleWidth(&OverLappedArea);
	OverLappedHeight = GetRectangleHeight(&OverLappedArea);
	CurrentVideoMemoryAddress = WindowManager.VideoMemory+OverLappedArea.Y1*ScreenWidth+OverLappedArea.X1;
	CurrentWindowBufferAddress = Window->WindowBuffer+(OverLappedArea.Y1-Window->Area.Y1)*WindowWidth+(OverLappedArea.X1-Window->Area.X1);
	for(i = 0; i < OverLappedHeight; i++) {
		MemCpy(CurrentVideoMemoryAddress , CurrentWindowBufferAddress , OverLappedWidth*sizeof(COLOR));
		CurrentVideoMemoryAddress += ScreenWidth;
		CurrentWindowBufferAddress += WindowWidth;
	}
}

QWORD FindWindowByPoint(int X , int Y) {
	QWORD WindowID;
	WINDOW *Window;
	WindowID = WindowManager.BackgroundWindowID;
	Lock(&(WindowManager.Lock));
	Window = GetHeaderFromList(&(WindowManager.WindowList));
	do {
		Window = GetNextFromList(&(WindowManager.WindowList) , Window);
		if((Window != NULL) && (Window->Flags & WINDOW_FLAGS_SHOW) && (IsInRectangle(&(Window->Area) , X , Y) == TRUE)) {
			WindowID = Window->Link.ID;
		}
	}while(Window != NULL);
	UnLock(&(WindowManager.Lock));
	return WindowID;
}

QWORD FindWindowByTitle(const char *Title) {
	QWORD WindowID;
	WINDOW *Window;
	int TitleLength;
	Lock(&(WindowManager.Lock));
	Window = GetHeaderFromList(&(WindowManager.WindowList));
	while(Window != NULL) {
		if((StrLen(Window->WindowTitle) == TitleLength) && (MemCmp(Window->WindowTitle , Title , TitleLength) == 0)) {
			WindowID = Window->Link.ID;
			break;
		}
		Window = GetNextFromList(&(WindowManager.WindowList) , Window);
	}
	UnLock(&(WindowManager.Lock));
	return WindowID;
}

BOOL IsWindowExist(QWORD WindowID) {
	if(GetWindow(WindowID) == NULL) {
		return FALSE;
	}
	return TRUE;
}

QWORD GetTopWindowID(void) {
	WINDOW *ActivateWindow;
	QWORD ActivateWindowID;
	Lock(&(WindowManager.Lock));
	ActivateWindow = (WINDOW*)GetTailFromList(&(WindowManager.WindowList));
	if(ActivateWindow != NULL) {
		ActivateWindowID = ActivateWindow->Link.ID;
	}
	else {
		ActivateWindowID = WINDOW_INVALIDID;
	}
	UnLock(&(WindowManager.Lock));
	return ActivateWindowID;
}

BOOL MoveWindowToTop(QWORD WindowID) {
	WINDOW *Window;
	RECT Area;
	DWORD Flags;
	QWORD TopWindowID;
	EVENT Event;
	TopWindowID = GetTopWindowID();
	if(TopWindowID == WindowID) {
		return TRUE;
	}
	Lock(&(WindowManager.Lock));
	Window = RemoveList(&(WindowManager.WindowList) , WindowID);
	if(Window != NULL) {
		AddListToTail(&(WindowManager.WindowList) , Window);
		ConvertRectScreenToClient(WindowID , &(Window->Area) , &Area);
		Flags = Window->Flags;
	}
	UnLock(&(WindowManager.Lock));
	if(Window != NULL) {
		SetWindowEvent(WindowID , EVENT_WINDOW_SELECT , &Event);
		SendEventToWindow(WindowID , &Event);
		if(Flags & WINDOW_FLAGS_DRAWTITLE) {
			UpdateWindowTitle(WindowID , TRUE);
			Area.Y1 += WINDOW_TITLEBAR_HEIGHT;
			UpdateScreenByWindowArea(WindowID , &Area);
		}
		else {
			UpdateScreenByID(WindowID);
		}
		SetWindowEvent(TopWindowID , EVENT_WINDOW_DESELECT , &Event);
		SendEventToWindow(TopWindowID , &Event);
		UpdateWindowTitle(TopWindowID , FALSE);
		return TRUE;
	}
	return FALSE;
}

BOOL IsInTitleBar(QWORD WindowID , int X , int Y) {
	WINDOW *Window;
	Window = GetWindow(WindowID);
	if((Window == NULL)||((Window->Flags & WINDOW_FLAGS_DRAWTITLE) == 0)) {
		return FALSE;
	}
	if((Window->Area.X1 <= X) && (X <= Window->Area.X2) && (Window->Area.Y1 <= Y) && (Y <= Window->Area.Y1+WINDOW_TITLEBAR_HEIGHT)) {
		return TRUE;
	}
	return FALSE;
}

BOOL IsInCloseButton(QWORD WindowID , int X , int Y) {
	WINDOW *Window;
	Window = GetWindow(WindowID);
	if((Window == NULL) && (Window->Flags & WINDOW_FLAGS_DRAWTITLE) == 0) {
		return FALSE;
	}
	if(((Window->Area.X2-WINDOW_XBUTTON_SIZE-1) <= X) && (X <= (Window->Area.X2-1)) && ((Window->Area.Y1+1) <= Y) && (Y <= (Window->Area.Y1+1+WINDOW_XBUTTON_SIZE))) {
		return TRUE;
	}
	return FALSE;
}

BOOL MoveWindow(QWORD WindowID , int X , int Y) {
	WINDOW *Window;
	RECT PrevArea;
	int Width;
	int Height;
	EVENT Event;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	MemCpy(&PrevArea , &(Window->Area) , sizeof(RECT));
	Width = GetRectangleWidth(&PrevArea);
	Height = GetRectangleHeight(&PrevArea);
	SetRectangleData(X , Y , X+Width-1 , Y+Height-1 , &(Window->Area));
	UnLock(&(Window->Lock));
	UpdateScreenByScreenArea(&PrevArea);
	UpdateScreenByID(WindowID);
	SetWindowEvent(WindowID , EVENT_WINDOW_MOVE , &Event);
	SendEventToWindow(WindowID , &Event);
	return TRUE;
}

static BOOL UpdateWindowTitle(QWORD WindowID , BOOL SelectedTitle) {
	WINDOW *Window;
	RECT TitleBarArea;
	Window = GetWindow(WindowID);
	if((Window != NULL) && (Window->Flags & WINDOW_FLAGS_DRAWTITLE)) {
		DrawWindowTitle(Window->Link.ID , Window->WindowTitle , SelectedTitle);
		TitleBarArea.X1 = 0;
		TitleBarArea.Y1 = 0;
		TitleBarArea.X2 = GetRectangleWidth(&(Window->Area))-1;
		TitleBarArea.Y2 = WINDOW_TITLEBAR_HEIGHT;
		UpdateScreenByWindowArea(WindowID , &TitleBarArea);
		return TRUE;
	}
	return FALSE;
}

BOOL GetWindowArea(QWORD WindowID , RECT *Area) {
	WINDOW *Window;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	MemCpy(Area , &(Window->Area) , sizeof(RECT));
	UnLock(&(Window->Lock));
	return TRUE;
}

BOOL ConvertPointScreenToClient(QWORD WindowID , const POINT *XY , POINT *XYInWindow) {
	RECT Area;
	if(GetWindowArea(WindowID , &Area) == FALSE) {
		return FALSE;
	}
	XYInWindow->X = XY->X-Area.X1;
	XYInWindow->Y = XY->Y-Area.Y1;
	return TRUE;
}

BOOL ConvertPointClientToScreen(QWORD WindowID , const POINT *XY , POINT *XYInScreen) {
	RECT Area;
	if(GetWindowArea(WindowID , &Area) == FALSE) {
		return FALSE;
	}
	XYInScreen->X = XY->X+Area.X1;
	XYInScreen->Y = XY->Y+Area.Y1;
	return TRUE;
}

BOOL ConvertRectScreenToClient(QWORD WindowID , const RECT *Area , RECT *AreaInWindow) {
	RECT WindowArea;
	if(GetWindowArea(WindowID , &WindowArea) == FALSE) {
		return FALSE;
	}
	AreaInWindow->X1 = Area->X1-WindowArea.X1;
	AreaInWindow->Y1 = Area->Y1-WindowArea.Y1;
	AreaInWindow->X2 = Area->X2-WindowArea.X1;
	AreaInWindow->Y2 = Area->Y2-WindowArea.Y1;
	return TRUE;
}

BOOL ConvertRectClientToScreen(QWORD WindowID , const RECT *Area , RECT *AreaInScreen) {
	RECT WindowArea;
	if(GetWindowArea(WindowID , &WindowArea) == FALSE) {
		return FALSE;
	}
	AreaInScreen->X1 = Area->X1+WindowArea.X1;
	AreaInScreen->Y1 = Area->Y1+WindowArea.Y1;
	AreaInScreen->X2 = Area->X2+WindowArea.X1;
	AreaInScreen->Y2 = Area->Y2+WindowArea.Y1;
	return TRUE;
}

BOOL UpdateScreenByID(QWORD WindowID) {
	EVENT Event;
	WINDOW *Window;
	Window = GetWindow(WindowID);
	if((Window == NULL) && ((Window->Flags & WINDOW_FLAGS_SHOW) == 0)) {
		return FALSE;
	}
	Event.Type = EVENT_WINDOWMANAGER_UPDATESCREENBYID;
	Event.WindowEvent.WindowID = WindowID;
	return SendEventToWindowManager(&Event);
}

BOOL UpdateScreenByWindowArea(QWORD WindowID , const RECT *Area) {
	EVENT Event;
	WINDOW *Window;
	Window = GetWindow(WindowID);
	if((Window == NULL) && ((Window->Flags & WINDOW_FLAGS_SHOW) == 0)) {
		return FALSE;
	}
	Event.Type = EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA;
	Event.WindowEvent.WindowID = WindowID;
	MemCpy(&(Event.WindowEvent.Area) , Area , sizeof(RECT));
	return SendEventToWindowManager(&Event);
}

BOOL UpdateScreenByScreenArea(const RECT *Area) {
	EVENT Event;
	Event.Type = EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA;
	Event.WindowEvent.WindowID = WINDOW_INVALIDID;
	MemCpy(&(Event.WindowEvent.Area) , Area , sizeof(RECT));
	return SendEventToWindowManager(&Event);
}

BOOL SendEventToWindow(QWORD WindowID , const EVENT *Event) {
	WINDOW *Window;
	BOOL Result;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	Result = PutQueue(&(Window->EventQueue) , Event);
	UnLock(&(Window->Lock));
	return Result;
}

BOOL ReceiveEventFromWindowQueue(QWORD WindowID , EVENT *Event) {
	WINDOW *Window;
	BOOL Result;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	Result = GetQueue(&(Window->EventQueue) , Event);
	UnLock(&(Window->Lock));
	return Result;
}

BOOL SendEventToWindowManager(const EVENT *Event) {
	BOOL Result = FALSE;
	if(IsQueueFull(&(WindowManager.EventQueue)) == FALSE) {
		Lock(&(WindowManager.Lock));
		Result = PutQueue(&(WindowManager.EventQueue) , Event);
		UnLock(&(WindowManager.Lock));
	}
	return Result;
}

BOOL ReceiveEventFromWindowManagerQueue(EVENT *Event) {
	BOOL Result = FALSE;
	if(IsQueueEmpty(&(WindowManager.EventQueue)) == FALSE) {
		Lock(&(WindowManager.Lock));
		Result = GetQueue(&(WindowManager.EventQueue) , Event);
		UnLock(&(WindowManager.Lock));
	}
	return Result;
}

BOOL SetMouseEvent(QWORD WindowID , QWORD EventType , int MouseX , int MouseY , BYTE ButtonStatus , EVENT *Event) {
	POINT MouseXYInWindow;
	POINT MouseXY;
	switch(EventType) {
		case EVENT_MOUSE_MOVE:
		case EVENT_MOUSE_LBUTTONDOWN:
		case EVENT_MOUSE_LBUTTONUP:
		case EVENT_MOUSE_RBUTTONDOWN:
		case EVENT_MOUSE_RBUTTONUP:
		case EVENT_MOUSE_MBUTTONDOWN:
		case EVENT_MOUSE_MBUTTONUP:
			MouseXY.X = MouseX;
			MouseXY.Y = MouseY;
			if(ConvertPointScreenToClient(WindowID , &MouseXY , &MouseXYInWindow) == FALSE) {
				return FALSE;
			}
			Event->Type = EventType;
			Event->MouseEvent.WindowID = WindowID;
			Event->MouseEvent.ButtonStatus = ButtonStatus;
			MemCpy(&(Event->MouseEvent.Point) , &MouseXYInWindow , sizeof(POINT));
			break;
		default:
			return FALSE;
			break;
	}
	return TRUE;
}

BOOL SetWindowEvent(QWORD WindowID , QWORD EventType , EVENT *Event) {
	RECT Area;
	switch(EventType) {
		case EVENT_WINDOW_SELECT:
		case EVENT_WINDOW_DESELECT:
		case EVENT_WINDOW_MOVE:
		case EVENT_WINDOW_RESIZE:
		case EVENT_WINDOW_CLOSE:
			Event->Type = EventType;
			Event->WindowEvent.WindowID = WindowID;
			if(GetWindowArea(WindowID , &Area) == FALSE) {
				return FALSE;
			}
			MemCpy(&(Event->WindowEvent.Area) , &Area , sizeof(RECT));
			break;
		default:
			return FALSE;
			break;
	}
	return TRUE;
}

void SetKeyEvent(QWORD Window , const KEYDATA *KeyData , EVENT *Event) {
	if(KeyData->Flags & KEY_FLAGS_DOWN) {
		Event->Type = EVENT_KEY_DOWN;
	}
	else {
		Event->Type = EVENT_KEY_UP;
	}
	Event->KeyEvent.ASCIICode = KeyData->ASCIICode;
	Event->KeyEvent.ScanCode = KeyData->ScanCode;
	Event->KeyEvent.Flags = KeyData->Flags;
}

BOOL DrawWindowFrame(QWORD WindowID) {
	WINDOW *Window;
	RECT Area;
	int Width;
	int Height;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	Width = GetRectangleWidth(&(Window->Area));
	Height = GetRectangleHeight(&(Window->Area));
	SetRectangleData(0 , 0 , Width-1 , Height-1 , &Area);
	InternalDrawRect(&Area , Window->WindowBuffer , 0 , 0 , Width-1 , Height-1 , RGB(00 , 00 , 0xFF) , TRUE);
	InternalDrawRect(&Area , Window->WindowBuffer , 0 , 0 , Width , Height , RGB(00 , 00 , 00) , FALSE);
	UnLock(&(Window->Lock));
	return TRUE;
}

static BOOL DrawWindowLastOne(QWORD WindowID) {
	WINDOW *Window;
	RECT Area;
	int Width;
	int Height;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	Width = GetRectangleWidth(&(Window->Area));
	Height = GetRectangleHeight(&(Window->Area));
	SetRectangleData(0 , 0 , Width-1 , Height-1 , &Area);
	InternalDrawRect(&Area , Window->WindowBuffer , 0 , 0 , Width-1 , Height-1 , RGB(00 , 00 , 00) , FALSE);
	UnLock(&(Window->Lock));
	return TRUE;
}

BOOL DrawWindowBackground(QWORD WindowID) {
	WINDOW *Window;
	int Width;
	int Height;
	RECT Area;
	int X;
	int Y;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	Width = GetRectangleWidth(&(Window->Area));
	Height = GetRectangleHeight(&(Window->Area));
	SetRectangleData(0 , 0 , Width-1 , Height-1 , &Area);
	if(Window->Flags & WINDOW_FLAGS_DRAWTITLE) {
		Y = WINDOW_TITLEBAR_HEIGHT;
	}
	else {
		Y = 0;
	}
	if(Window->Flags & WINDOW_FLAGS_DRAWFRAME) {
		X = 5;
	}
	else {
		X = 0;
	}
	InternalDrawRect(&Area , Window->WindowBuffer , X , Y , Width-1-X , Height-X , WINDOW_COLOR_BACKGROUND , TRUE);
	UnLock(&(Window->Lock));
	return TRUE;
}

BOOL DrawWindowTitle(QWORD WindowID , const char *Title , BOOL SelectedTitle) {
	WINDOW *Window;
	int Width;
	int Height;
	RECT Area;
    RECT ButtonArea;
    COLOR TitleBarTextColor;
	int X;
	int Y;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	Width = GetRectangleWidth(&(Window->Area));
	Height = GetRectangleHeight(&(Window->Area));
	SetRectangleData(0 , 0 , Width-1 , Height-1 , &Area);
	if(SelectedTitle == TRUE) {
		TitleBarTextColor =  WINDOW_COLOR_TITLEBARTEXT;
	}
	else {
		TitleBarTextColor = RGB(0xBB , 0xBB , 0xBB);
	}
	InternalDrawRect(&Area , Window->WindowBuffer , 1 , 3 , Width-1 , WINDOW_TITLEBAR_HEIGHT-2 , RGB(00 , 00 , 0xFF) , TRUE);
	InternalDrawText(&Area , Window->WindowBuffer , 7 , 3 , TitleBarTextColor , RGB(00 , 00 , 0xFF) , Title , StrLen(Title));
	ButtonArea.X1 = Width-WINDOW_XBUTTON_SIZE-1;
	ButtonArea.Y1 = 1;
	ButtonArea.X2 = Width-2;
	ButtonArea.Y2 = WINDOW_XBUTTON_SIZE-1;
	DrawButton(WindowID , &ButtonArea , RGB(0xFF , 00 , 00) , "X" , WINDOW_COLOR_BACKGROUND);
	UnLock(&(Window->Lock));
	return TRUE;
}

BOOL DrawButton(QWORD WindowID , RECT *ButtonArea , COLOR BackgroundColor , const char *Text , COLOR TextColor) {
	WINDOW *Window;
	RECT Area;
	int WindowWidth;
	int WindowHeight;
	int TextLength;
	int TextWidth;
	int ButtonWidth;
	int ButtonHeight;
	int TextX;
	int TextY;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	WindowWidth = GetRectangleWidth(&(Window->Area));
	WindowHeight = GetRectangleHeight(&(Window->Area));
	SetRectangleData(0 , 0 , WindowWidth-1 , WindowHeight-1 , &Area);
	ButtonWidth = GetRectangleWidth(ButtonArea);
	ButtonHeight = GetRectangleHeight(ButtonArea);
	TextLength = StrLen(Text);
	TextWidth = TextLength*FONT_ENGLISHWIDTH;
	TextX = (ButtonArea->X1+ButtonWidth/2)-TextWidth/2;
	TextY = (ButtonArea->Y1+ButtonHeight/2)-FONT_ENGLISHHEIGHT/2;
	InternalDrawRect(&Area , Window->WindowBuffer , ButtonArea->X1+2 , ButtonArea->Y1+3 , ButtonArea->X2 , ButtonArea->Y2+3 , RGB(00 , 00 , 00) , TRUE);
	InternalDrawRect(&Area , Window->WindowBuffer , ButtonArea->X1 , ButtonArea->Y1+1 , ButtonArea->X2-2 , ButtonArea->Y2+1 , BackgroundColor , TRUE);
	InternalDrawText(&Area , Window->WindowBuffer , TextX , TextY , TextColor , BackgroundColor , Text , TextLength);
}

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

static void DrawCursor(int X , int Y) {
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
					InternalDrawPixel(&(WindowManager.ScreenArea) , WindowManager.VideoMemory , i+X , j+Y , RGB(00 , 00 , 0xFF));
					break;
				case 2:
					InternalDrawPixel(&(WindowManager.ScreenArea) , WindowManager.VideoMemory , i+X , j+Y , RGB(00 , 0xFF , 00));
					break;
			}
			CurrentPos++;
		}
	}
}

void MoveCursor(int X , int Y) {
	RECT PrevArea;
	if(X < WindowManager.ScreenArea.X1) {
		X = WindowManager.ScreenArea.X1;
	}
	else if(X > WindowManager.ScreenArea.X2) {
		X = WindowManager.ScreenArea.X2;
	}
	if(Y < WindowManager.ScreenArea.Y1) {
		Y = WindowManager.ScreenArea.Y1;
	}
	else if(Y > WindowManager.ScreenArea.Y2) {
		Y = WindowManager.ScreenArea.Y2;
	}
	Lock(&(WindowManager.Lock));
	PrevArea.X1 = WindowManager.MouseX;
	PrevArea.Y1 = WindowManager.MouseY;
	PrevArea.X2 = WindowManager.MouseX+MOUSE_CURSOR_WIDTH-1;
	PrevArea.Y2 = WindowManager.MouseY+MOUSE_CURSOR_HEIGHT-1;
	WindowManager.MouseX = X;
	WindowManager.MouseY = Y;
	UnLock(&(WindowManager.Lock));
	ReadrawWindowByArea(&PrevArea);
	DrawCursor(X , Y);
}

void GetCursorPosition(int *X , int *Y) {
	*X = WindowManager.MouseX;
	*Y = WindowManager.MouseY;
}

BOOL DrawPixel(QWORD WindowID , int X , int Y , COLOR Color) {
	WINDOW *Window;
	RECT Area;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	SetRectangleData(0 , 0 , Window->Area.X2-Window->Area.X1 , Window->Area.Y2-Window->Area.Y1 , &Area);
	InternalDrawPixel(&Area , Window->WindowBuffer , X , Y , Color);
	UnLock(&Window->Lock);
	return TRUE;
}

BOOL DrawLine(QWORD WindowID , int X1 , int Y1 , int X2 , int Y2 , COLOR Color) {
	WINDOW *Window;
	RECT Area;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	SetRectangleData(0 , 0 , Window->Area.X2-Window->Area.X1 , Window->Area.Y2-Window->Area.Y1 , &Area);
	InternalDrawLine(&Area , Window->WindowBuffer , X1 , Y1 , X2 , Y2 , Color);
	UnLock(&Window->Lock);
	return TRUE;
}

BOOL DrawRect(QWORD WindowID , int X1 , int Y1 , int X2 , int Y2 , COLOR Color , BOOL Fill) {
	WINDOW *Window;
	RECT Area;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	SetRectangleData(0 , 0 , Window->Area.X2-Window->Area.X1 , Window->Area.Y2-Window->Area.Y1 , &Area);
	InternalDrawRect(&Area , Window->WindowBuffer , X1 , Y1 , X2 , Y2 , Color , Fill);
	UnLock(&Window->Lock);
	return TRUE;
}

BOOL DrawCircle(QWORD WindowID , int X , int Y , int Radius , COLOR Color , BOOL Fill) {
	WINDOW *Window;
	RECT Area;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	SetRectangleData(0 , 0 , Window->Area.X2-Window->Area.X1 , Window->Area.Y2-Window->Area.Y1 , &Area);
	InternalDrawCircle(&Area , Window->WindowBuffer , X , Y , Radius , Color , Fill);
	UnLock(&Window->Lock);
}

BOOL DrawText(QWORD WindowID , int X , int Y , COLOR TextColor , COLOR BackgroundColor , const char *String , int Length) {
	WINDOW *Window;
	RECT Area;
	Window = GetWindowWithWindowLock(WindowID);
	if(Window == NULL) {
		return FALSE;
	}
	SetRectangleData(0 , 0 , Window->Area.X2-Window->Area.X1 , Window->Area.Y2-Window->Area.Y1 , &Area);
	InternalDrawText(&Area , Window->WindowBuffer , X , Y , TextColor , BackgroundColor , String , Length);
	UnLock(&Window->Lock);
	return TRUE;
}
