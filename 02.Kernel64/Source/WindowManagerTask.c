#include "Types.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"
#include "GUITask.h"

void StartWindowManager(void) {
	int MouseX;
	int MouseY;
	BOOL MouseDataResult;
	BOOL KeyDataResult;
	BOOL EventQueueResult;
	InitGUISystem();
	GetCursorPosition(&MouseX , &MouseY);
	MoveCursor(MouseX , MouseY);
	while(1) {
		MouseDataResult = ProcessMouseData();
		KeyDataResult = ProcessKeyData();
		EventQueueResult = FALSE;
		while(ProcessEventQueueData() == TRUE) {
			EventQueueResult = TRUE;
		}
		if((MouseDataResult == FALSE) && (KeyDataResult == FALSE) && (EventQueueResult == FALSE)) {
			Sleep(0);
		}
	}
}

BOOL ProcessMouseData(void) {
	QWORD WindowIDUnderMouse;
	BYTE ButtonStatus;
	int RelativeX;
	int RelativeY;
	int MouseX;
	int MouseY;
	int PrevMouseX;
	int PrevMouseY;
	BYTE ChangedButton;
	RECT WindowArea;
	EVENT Event;
	WINDOWMANAGER *WindowManager;
	char TempTitle[WINDOW_TITLEMAXLENGTH];
	if(GetMouseDataFromMouseQueue(&ButtonStatus , &RelativeX , &RelativeY) == FALSE) {
		return FALSE;
	}
	WindowManager = GetWindowManager();
	GetCursorPosition(&MouseX , &MouseY);
	PrevMouseX = MouseX;
	PrevMouseY = MouseY;
	MouseX += RelativeX;
	MouseY += RelativeY;
	MoveCursor(MouseX , MouseY);
	GetCursorPosition(&MouseX , &MouseY);
	WindowIDUnderMouse = FindWindowByPoint(MouseX , MouseY);
	ChangedButton = WindowManager->PrevButtonStatus^ButtonStatus;
	if(ChangedButton & MOUSE_LBUTTONDOWN) {
		if(ButtonStatus & MOUSE_LBUTTONDOWN) {
			if(WindowIDUnderMouse != WindowManager->BackgroundWindowID) {
				MoveWindowToTop(WindowIDUnderMouse);
			}
			if(IsInTitleBar(WindowIDUnderMouse , MouseX , MouseY) == TRUE) {
				if(IsInCloseButton(WindowIDUnderMouse , MouseX , MouseY) == TRUE) {
					SetWindowEvent(WindowIDUnderMouse , EVENT_WINDOW_CLOSE , &Event);
					SendEventToWindow(WindowIDUnderMouse , &Event);
				}
				else {
					WindowManager->WindowMoveMode = TRUE;
					WindowManager->MovingWindowID = WindowIDUnderMouse;
				}
			}
			else {
				SetMouseEvent(WindowIDUnderMouse , EVENT_MOUSE_LBUTTONDOWN , MouseX , MouseY , ButtonStatus , &Event);
				SendEventToWindow(WindowIDUnderMouse , &Event);
			}
		}
		else {
			if(WindowManager->WindowMoveMode == TRUE) {
				WindowManager->WindowMoveMode = FALSE;
				WindowManager->MovingWindowID = WINDOW_INVALIDID;
			}
			else {
				SetMouseEvent(WindowIDUnderMouse , EVENT_MOUSE_LBUTTONUP , MouseX , MouseY , ButtonStatus , &Event);
				SendEventToWindow(WindowIDUnderMouse , &Event);
			}
		}
	}
	else if(ChangedButton & MOUSE_RBUTTONDOWN) {
		if(ButtonStatus & MOUSE_RBUTTONDOWN) {
			SetMouseEvent(WindowIDUnderMouse , EVENT_MOUSE_RBUTTONDOWN , MouseX , MouseY , ButtonStatus , &Event);
			SendEventToWindow(WindowIDUnderMouse , &Event);
			CreateTask(TASK_FLAGS_LOW|TASK_FLAGS_THREAD , NULL , NULL , (QWORD)DemoGUITask , TASK_LOADBALANCINGID);
		}
		else {
			SetMouseEvent(WindowIDUnderMouse , EVENT_MOUSE_RBUTTONUP , MouseX , MouseY , ButtonStatus , &Event);
			SendEventToWindow(WindowIDUnderMouse , &Event);
		}
	}
	else if(ChangedButton & MOUSE_MBUTTONDOWN) {
		if(ButtonStatus & MOUSE_MBUTTONDOWN) {
			SetMouseEvent(WindowIDUnderMouse , EVENT_MOUSE_MBUTTONDOWN , MouseX , MouseY , ButtonStatus , &Event);
			SendEventToWindow(WindowIDUnderMouse , &Event);
		}
		else {
			SetMouseEvent(WindowIDUnderMouse , EVENT_MOUSE_MBUTTONUP , MouseX , MouseY , ButtonStatus , &Event);
			SendEventToWindow(WindowIDUnderMouse , &Event);
		}
	}
	else {
		SetMouseEvent(WindowIDUnderMouse , EVENT_MOUSE_MOVE , MouseX , MouseY , ButtonStatus , &Event);
		SendEventToWindow(WindowIDUnderMouse , &Event);
	}
	if(WindowManager->WindowMoveMode == TRUE) {
		if(GetWindowArea(WindowManager->MovingWindowID , &WindowArea) == TRUE) {
			MoveWindow(WindowManager->MovingWindowID , WindowArea.X1+MouseX-PrevMouseX , WindowArea.Y1+MouseY-PrevMouseY);
		}
		else {
			WindowManager->MovingWindowID = FALSE;
			WindowManager->MovingWindowID = WINDOW_INVALIDID;
		}
	}
	WindowManager->PrevButtonStatus = ButtonStatus;
	return TRUE;
}

BOOL ProcessKeyData(void) {
	KEYDATA KeyData;
	EVENT Event;
	QWORD ActiveWindowID;
	if(GetKeyFromKeyQueue(&KeyData) == FALSE) {
		return FALSE;
	}
	ActiveWindowID = GetTopWindowID();
	SetKeyEvent(ActiveWindowID , &KeyData , &Event);
	return SendEventToWindow(ActiveWindowID , &Event);
}

BOOL ProcessEventQueueData(void) {
	EVENT Event;
	WINDOWEVENT *WindowEvent;
	QWORD WindowID;
	RECT Area;
	if(ReceiveEventFromWindowManagerQueue(&Event) == FALSE) {
		return FALSE;
	}
	WindowEvent = &(Event.WindowEvent);
	switch(Event.Type) {
		case EVENT_WINDOWMANAGER_UPDATESCREENBYID:
			if(GetWindowArea(WindowEvent->WindowID , &Area) == TRUE) {
				ReadrawWindowByArea(&Area);
			}
			break;
		case EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA:
			if(ConvertRectClientToScreen(WindowEvent->WindowID , &(WindowEvent->Area) , &Area) == TRUE) {
				ReadrawWindowByArea(&Area);
			}
			break;
		case EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA:
			ReadrawWindowByArea(&(WindowEvent->Area));
			break;
		default:
			break;
	}
	return TRUE;
}
