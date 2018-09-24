#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"
#include "Colors.h"

CONSOLEMANAGER ConsoleManager = {0 , };

void InitConsole(int X , int Y) {
	MemSet(&ConsoleManager , 0 , sizeof(ConsoleManager));
	SetCursor(X , Y);
}

void SetCursor(int X , int Y) {
	int LinearVal;
	LinearVal = Y*CONSOLE_WIDTH+X;
	OutPortByte(VGA_PORT_INDEX , VGA_INDEX_UPPERCURSOR);
	OutPortByte(VGA_PORT_DATA , LinearVal >> 8);
	OutPortByte(VGA_PORT_INDEX , VGA_INDEX_LOWERCURSOR);
	OutPortByte(VGA_PORT_DATA , LinearVal & 0xFF);
	ConsoleManager.CurrentPrintOffset = LinearVal;
}

void GetCursor(int *X , int *Y) {
	*X = ConsoleManager.CurrentPrintOffset % CONSOLE_WIDTH;
	*Y = ConsoleManager.CurrentPrintOffset/CONSOLE_WIDTH;
}

void Printf(CONSOLECOLOR Color , const char *Format , ...) {
	va_list ap;
	char Buffer[1024];
	int NextPrintOffset;
	va_start(ap , Format);
	VSPrintf(Buffer , Format , ap);
	va_end(ap);
	NextPrintOffset = ConsoleKernelPrintf(Color , Buffer);
	SetCursor(NextPrintOffset % CONSOLE_WIDTH , NextPrintOffset/CONSOLE_WIDTH);
}
	
int ConsoleKernelPrintf(CONSOLECOLOR Color , const char *Buffer) {
	CHARACTER *Screen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;
	int j;
	int Length;
	int PrintOffset;
	PrintOffset = ConsoleManager.CurrentPrintOffset;
	Length = StrLen(Buffer);
	for(i = 0; i < Length; i++) {
		if(Buffer[i] == '\n') {
			PrintOffset += (CONSOLE_WIDTH-(PrintOffset%CONSOLE_WIDTH));
		}
		else if(Buffer[i] == '\t') {
			PrintOffset += (8-(PrintOffset % 8));
		}
		else {
			Screen[PrintOffset].Charactor = Buffer[i];
			Screen[PrintOffset].Attribute = Color;
			PrintOffset++;
		}
		if(PrintOffset >= (CONSOLE_HEIGHT*CONSOLE_WIDTH)) {
			MemCpy(CONSOLE_VIDEOMEMORYADDRESS , CONSOLE_VIDEOMEMORYADDRESS+CONSOLE_WIDTH*sizeof(CHARACTER) , (CONSOLE_HEIGHT-1)*CONSOLE_WIDTH*sizeof(CHARACTER));
			for(j = (CONSOLE_HEIGHT-1)*(CONSOLE_WIDTH); j < (CONSOLE_HEIGHT*CONSOLE_WIDTH); j++) {
				Screen[j].Charactor = ' ';
				Screen[j].Attribute = Color;
			}
			PrintOffset = (CONSOLE_HEIGHT-1)*CONSOLE_WIDTH;
		}
	}
	return PrintOffset;
}

void ClearScreen(CONSOLECOLOR Color) {
	CHARACTER *Screen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;
	for(i = 0; i < CONSOLE_WIDTH*CONSOLE_HEIGHT; i++) {
		Screen[i].Charactor = ' ';
		Screen[i].Attribute = Color;
	}
	SetCursor(0 , 0);
}

BYTE GetCh(void) {
	KEYDATA Data;
	while(1) {
		while(GetKeyFromKeyQueue(&Data) == FALSE) {
			;
		}
		if(Data.Flags & KEY_FLAGS_DOWN) {
			return Data.ASCIICode;
		}
	}
}

BOOL kbhit(void) {
	KEYDATA Data;
	return GetKeyFromKeyQueue(&Data);
}

void KernelPrintfXY(int Color , int X , int Y , const char *String) {
	CHARACTER *Address;
	Address = (CHARACTER*)0xB8000;
	int i;
	Address += (Y*80)+X;
	for(i = 0; String[i] != 0; i++) {
		Address[i].Charactor = String[i];
		Address[i].Attribute = Color;
	}
}
