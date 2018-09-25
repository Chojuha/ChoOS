#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "Types.h"
#include "Colors.h"

#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 24
#define CONSOLE_VIDEOMEMORYADDRESS 0xB8000
#define VGA_PORT_INDEX 0x3D4
#define VGA_PORT_DATA 0x3D5
#define VGA_INDEX_UPPERCURSOR 0x0E
#define VGA_INDEX_LOWERCURSOR 0x0F

#pragma pack(push , 1)

typedef struct ConsoleManagerStruct {
	int CurrentPrintOffset;
}CONSOLEMANAGER;

#pragma pack(pop)

void InitConsole(int X , int Y);
void SetCursor(int X , int Y);
void GetCursor(int *X , int *Y);
void Printf(CONSOLECOLOR Color , const char *Format , ...);
int ConsoleKernelPrintf(CONSOLECOLOR Color , const char *Buffer);
void ClearScreen(CONSOLECOLOR Color);
BYTE GetCh(void);
BOOL kbhit(void);
void KernelPrintfXY(int Color , int X , int Y , const char *String);

#endif
