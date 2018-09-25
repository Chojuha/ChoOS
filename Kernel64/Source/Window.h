#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "Types.h"
#include "Synchronization.h"
#include "2DGraphics.h"
#include "List.h"
#include "Queue.h"
#include "Keyboard.h"

#define WINDOW_MAXCOUNT 2048
#define GETWINDOWOFFSET(x) ((x) & 0xFFFFFFFF)
#define WINDOW_TITLEMAXLENGTH 40
#define WINDOW_INVALIDID 0xFFFFFFFFFFFFFFFF
#define WINDOW_FLAGS_SHOW 0x00000001
#define WINDOW_FLAGS_DRAWFRAME 0x00000002
#define WINDOW_FLAGS_DRAWTITLE 0x00000004
#define WINDOW_FLAGS_DEFAULT (WINDOW_FLAGS_SHOW|WINDOW_FLAGS_DRAWFRAME|WINDOW_FLAGS_DRAWTITLE)

#define WINDOW_TITLEBAR_HEIGHT 24
#define WINDOW_XBUTTON_SIZE 20
#define WINDOW_COLOR_FRAME                  RGB(00 , 00 , 0xFF)
#define WINDOW_COLOR_BACKGROUND             RGB(0xFF , 0xFF , 0xFF)
#define WINDOW_COLOR_TITLEBARTEXT           RGB(0xFF , 0xFF , 0xFF)
#define WINDOW_COLOR_TITLEBARBACKGROUND     RGB(00 , 00 , 0xFF)
#define WINDOW_COLOR_TITLEBARACTIVEBACKGROUND RGB(00 , 00 , 0xFF)
#define WINDOW_COLOR_TITLEBARINACTIVEBACKGROUND RGB(00 , 00 , 0xCC)
#define WINDOW_COLOR_SYSTEMBACKGROUND       RGB(00 , 0xFF , 0xFF)
#define WINDOW_COLOR_XBUTTONLINECOLOR       RGB(0xFF , 0xFF , 0xFF)
#define WINDOW_BACKGROUNDWINDOWTITLE "SYSTEM_BACKGROUNDWINDOW"
#define MOUSE_CURSOR_WIDTH 16
#define MOUSE_CURSOR_HEIGHT 16
#define MOUSE_CURSOR_OUTER RGB(00 , 00 , 0xFF)
#define MOUSE_CURSOR_INNER RGB(00 , 0xFF , 00)
#define EVENTQUEUE_WINDOWMAXCOUNT 100
#define EVENTQUEUE_WINDOWMANAGERMAXCOUNT WINDOW_MAXCOUNT
#define EVENT_UNKNOWN 0
#define EVENT_MOUSE_MOVE 1
#define EVENT_MOUSE_LBUTTONDOWN 2
#define EVENT_MOUSE_LBUTTONUP 3
#define EVENT_MOUSE_RBUTTONDOWN 4
#define EVENT_MOUSE_RBUTTONUP 5
#define EVENT_MOUSE_MBUTTONDOWN 6
#define EVENT_MOUSE_MBUTTONUP 7
#define EVENT_WINDOW_SELECT 8
#define EVENT_WINDOW_DESELECT 9
#define EVENT_WINDOW_MOVE 10
#define EVENT_WINDOW_RESIZE 11
#define EVENT_WINDOW_CLOSE 12
#define EVENT_KEY_DOWN 13
#define EVENT_KEY_UP 14
#define EVENT_WINDOWMANAGER_UPDATESCREENBYID 15
#define EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA 16
#define EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA 17

typedef struct MouseEventStruct {
	QWORD WindowID;
	POINT Point;
	BYTE ButtonStatus;
}MOUSEEVENT;

typedef struct KeyEventStruct {
	QWORD WindowID;
	BYTE ASCIICode;
	BYTE ScanCode;
	BYTE Flags;
}KEYEVENT;

typedef struct WindowEventStruct {
	QWORD WindowID;
	RECT Area;
}WINDOWEVENT;

typedef struct EventStruct {
	QWORD Type;
	union {
		MOUSEEVENT MouseEvent;
		KEYEVENT KeyEvent;
		WINDOWEVENT WindowEvent;
		QWORD Data[3];
	};
}EVENT;

typedef struct WindowStruct {
	LISTLINK Link;
	MUTEX Lock;
	RECT Area;
	COLOR *WindowBuffer;
	QWORD TaskID;
	DWORD Flags;
	QUEUE EventQueue;                                          
	EVENT *EventBuffer;
	char WindowTitle[WINDOW_TITLEMAXLENGTH+1];
}WINDOW;

typedef struct WindowPoolManagerStruct {
	MUTEX Lock;
	WINDOW *StartAddress;
	int MaxCount;
	int UseCount;
	int AllocatedCount;
}WINDOWPOOLMANAGER;

typedef struct WindowManagerStruct {
	MUTEX Lock;
	LIST WindowList;
	int MouseX;
	int MouseY;
	RECT ScreenArea;
	COLOR *VideoMemory;
	QWORD BackgroundWindowID;
	QUEUE EventQueue;
	EVENT *EventBuffer;
	BYTE PrevButtonStatus;
	QWORD MovingWindowID;
	BOOL WindowMoveMode;
}WINDOWMANAGER;

static void InitWindowPool(void);
static WINDOW *AllocateWindow(void);
static void FreeWindow(QWORD ID);
void InitGUISystem(void);
WINDOWMANAGER *GetWindowManager(void);
QWORD GetBackgroundWindowID(void);
void GetScreenArea(RECT *ScreenArea);
QWORD CreateWindow(int X , int Y , int Width , int Height , DWORD Flags , const char *Title);
BOOL DeleteWindow(QWORD WindowID);
BOOL DeleteAllWindowInTaskID(QWORD TaskID);
WINDOW *GetWindow(QWORD WindowID);
WINDOW *GetWindowWithWindowLock(QWORD WindowID);
BOOL ShowWindow(QWORD WindowID , BOOL Show);
BOOL ReadrawWindowByArea(const RECT *Area);
static void CopyWindowBufferToFrameBuffer(const WINDOW *Window , const RECT *CopyArea);
QWORD FindWindowByPoint(int X , int Y);
QWORD FindWindowByTitle(const char *Title);
BOOL IsWindowExist(QWORD WindowID);
QWORD GetTopWindowID(void);
BOOL MoveWindowToTop(QWORD WindowID);
BOOL IsInTitleBar(QWORD WindowID , int X , int Y);
BOOL IsInCloseButton(QWORD WindowID , int X , int Y);
BOOL MoveWindow(QWORD WindowID , int X , int Y);
static BOOL UpdateWindowTitle(QWORD WindowID , BOOL SelectedTitle);
BOOL GetWindowArea(QWORD WindowID , RECT *Area);
BOOL ConvertPointScreenToClient(QWORD WindowID , const POINT *XY , POINT *XYInWindow);
BOOL ConvertPointClientToScreen(QWORD WindowID , const POINT *Area , POINT *XYInScreen);
BOOL ConvertRectScreenToClient(QWORD WindowID , const RECT *Area , RECT *AreaInWindow);
BOOL ConvertRectScreenToClient(QWORD WindowID , const RECT *Area , RECT *AreaInScreen);
BOOL UpdateScreenByID(QWORD WindowID);
BOOL UpdateScreenByWindowArea(QWORD WindowID , const RECT *Area);
BOOL UpdateScreenByScreenArea(const RECT *Area);
BOOL SendEventToWindow(QWORD WindowID , const EVENT *Event);
BOOL SendEventToWindowManager(const EVENT *Event);
BOOL SetMouseEvent(QWORD WindowID , QWORD EventType , int MouseX , int MouseY , BYTE ButtonStatus , EVENT *Event);
BOOL SetWindowEvent(QWORD WindowID , QWORD EventType , EVENT *Event);
void SetKeyEvent(QWORD Window , const KEYDATA *KeyData , EVENT *Event);

BOOL DrawWindowFrame(QWORD WindowID);
static BOOL DrawWindowLastOne(QWORD WindowID);
BOOL DrawWindowBackground(QWORD WindowID);
BOOL DrawWindowTitle(QWORD WindowID , const char *Title , BOOL SelectedTitle);
BOOL DrawButton(QWORD WindowID , RECT *ButtonArea , COLOR BackgroundColor , const char *Text , COLOR TextColor);
static void DrawCursor(int X , int Y);
void MoveCursor(int X , int Y);
void GetCursorPosition(int *X , int *Y);
BOOL DrawPixel(QWORD WindowID , int X , int Y , COLOR Color);
BOOL DrawLine(QWORD WindowID , int X1 , int Y1 , int X2 , int Y2 , COLOR Color);
BOOL DrawRect(QWORD WindowID , int X1 , int Y1 , int X2 , int Y2 , COLOR Color , BOOL Fill);
BOOL DrawCircle(QWORD WindowID , int X , int Y , int Radius , COLOR Color , BOOL Fill);
BOOL DrawText(QWORD WindowID , int X , int Y , COLOR TextColor , COLOR BackgroundColor , const char *String , int Length);

#endif
