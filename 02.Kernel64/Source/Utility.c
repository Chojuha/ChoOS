#include "Utility.h"
#include "AssemblyUtility.h"
#include "Console.h"
#include "Keyboard.h"
#include "FileSystem.h"
#include "PIT.h"
#include "RTC.h"
#include <stdarg.h>
#include "VBE.h"

volatile QWORD TickCount = 0;

void nop(void) {
	;
}

void MemSet(void *Destination , BYTE Data , int Size) {
	int i;
	QWORD DefaultData;
	int RemainByteStartOffset;
	DefaultData = 0;
	for(i = 0; i < 8; i++) {
		DefaultData = (DefaultData << 8)|Data;
	}
	for(i = 0; i < (Size/8); i++) {
		((QWORD*)Destination)[i] = DefaultData;
	}
	RemainByteStartOffset = i*8;
	for(i = 0; i < (Size%8); i++) {
		((char*)Destination)[RemainByteStartOffset++] = Data;
	}
}

inline void MemSetWord(void *Destination , WORD Data , int WordSize) {
	int i;
	QWORD QData;
	int RemainWordStartOffset;
	QData = 0;
	for(i = 0; i < 4; i++) {
		QData = (QData << 16)|Data;
	}
	for(i = 0; i < (WordSize/4); i++) {
		((QWORD*)Destination)[i] = QData;
	}
	RemainWordStartOffset = i*4;
	for(i = 0 ; i < (WordSize%4); i++) {
		((WORD*)Destination)[RemainWordStartOffset++] = Data;
	}
}

int MemCpy(void *Destination , const void *Source , int Size) {
	int i;
	int RemainByteStartOffset;
	for(i = 0; i < (Size/8); i++) {
		((QWORD*)Destination)[i] = ((QWORD*)Source)[i];
	}
	RemainByteStartOffset = i*8;
	for(i = 0; i < (Size%8); i++) {
		((char*)Destination)[RemainByteStartOffset] = ((char*)Source)[RemainByteStartOffset];
		RemainByteStartOffset++;
	}
}

int MemCmp(const void *Destination , const void *Source , int Size) {
	int i;
	int RemainByteStartOffset;
	QWORD Val;
	char CVal;
	for(i = 0; i < (Size/8); i++) {
		Val = ((QWORD*)Destination)[i]-((QWORD*)Source)[i];
		if(Val != 0) {
			for(i = 0; i < 8; i++) {
				if(((Val >> (i*8)) & 0xFF) != 0) {
					return (Val >> (i*8)) & 0xFF;
				}
			}
		}
	}
	RemainByteStartOffset = i*8;
	for(i = 0; i < (Size%8); i++) {
		CVal = ((char*)Destination)[RemainByteStartOffset]-((char*)Source)[RemainByteStartOffset];
		if(CVal != 0) {
			return CVal;
		}
		RemainByteStartOffset++;
	}
	return 0;
}

BOOL SetInterruptFlag(BOOL EnableInterruptQ) {
	QWORD RFLAGS;
	RFLAGS = ReadRFLAGS();
	if(EnableInterruptQ == TRUE) {
		EnableInterrupt();
	}
	else {
		DisableInterrupt();
	}
	if(RFLAGS & 0x0200) {
		return TRUE;
	}
	return FALSE;
}

int StrLen(const char *Buffer) {
	int i;
	for(i = 0; ; i++) {
		if(Buffer[i] == '\0') {
			break;
		}
	}
	return i;
}

static TotalRamSize = 0;

void CheckTotalRamSize(void) {
	DWORD *CurrentAddress;
	DWORD PreVal;
	CurrentAddress = (DWORD*)0x4000000;
	while(1) {
		PreVal = *CurrentAddress;
		*CurrentAddress = 0x12345678;
		if(*CurrentAddress != 0x12345678) {
			break;
		}
		*CurrentAddress = PreVal;
		CurrentAddress += (0x400000/4);
	}
	TotalRamSize = (QWORD)CurrentAddress/0x100000;
}

QWORD GetTotalRamSize(void) {
	return TotalRamSize;
}

long atoi(const char *Buffer , int Radix) {
	long Return;
	switch(Radix) {
		case 16:
			Return = HexStringToQword(Buffer);
			break;
		case 10:
		default:
			Return = DecimalStringToLong(Buffer);
			break;
	}
	return Return;
}

QWORD HexStringToQword(const char *Buffer) {
	QWORD Val = 0;
	int i;
	for(i = 0; Buffer[i] != '\0'; i++) {
		Val *= 16;
		if(('A' <= Buffer[i]) && (Buffer[i] <= 'Z')) {
			Val += (Buffer[i]-'A')+10;
		}
		else if(('a' <= Buffer[i]) && (Buffer[i] <= 'z')) {
			Val += (Buffer[i]-'a')+10;
		}
		else {
			Val += Buffer[i]-'0';
		}
	}
	return Val;
}

long DecimalStringToLong(const char *Buffer) {
	long Val = 0;
	int i;
	if(Buffer[0] == '-') {
		i = 1;
	}
	else {
		i = 0;
	}
	for(; Buffer[i] != '\0'; i++) {
		Val *= 10;
		Val += Buffer[i]-'0';
	}
	if(Buffer[0] == '-') {
		Val = -Val;
	}
	return Val;
}

int itoa(long Val , char *Buffer , int Radix) {
	int Return;
	switch(Radix) {
		case 16:
			Return = HexToString(Val , Buffer);
			break;
		case 10:
		default:
			Return = DecimalToString(Val , Buffer);
			break;
	}
	return Return;
}

int HexToString(QWORD Val , char *Buffer) {
	QWORD i;
	QWORD CurrentVal;
	if(Val == 0) {
		Buffer[0] = '0';
		Buffer[1] = '\0';
		return 1;
	}
	for(i = 0; Val > 0; i++) {
		CurrentVal = Val % 16;
		if(CurrentVal >= 10) {
			Buffer[i] = 'A'+(CurrentVal-10);
		}
		else {
			Buffer[i] = '0'+CurrentVal;
		}
		Val = Val/16;
	}
	Buffer[i] = '\0';
	ReverseString(Buffer);
	return i;
}

int DecimalToString(long Val , char *Buffer) {
	long i;
	if(Val == 0) {
		Buffer[0] = '0';
		Buffer[1] = '\0';
		return 1;
	}
	if(Val < 0) {
		i = 1;
		Buffer[0] = '-';
		Val = -Val;
	}
	else {
		i = 0;
	}
	for(; Val > 0; i++) {
		Buffer[i] = '0'+Val % 10;
		Val = Val/10;
	}
	Buffer[i] = '\0';
	if(Buffer[0] == '-') {
		ReverseString(&(Buffer[1]));
	}
	else {
		ReverseString(Buffer);
	}
	return i;
}

void ReverseString(char *Buffer) {
	int Length;
	int i;
	char Temp;
	Length = StrLen(Buffer);
	for(i = 0; i < Length/2; i++) {
		Temp = Buffer[i];
		Buffer[i] = Buffer[Length-1-i];
		Buffer[Length-1-i] = Temp;
	}
}

int SPrintf(char *Buffer , const char *Format , ...) {
	va_list ap;
	int Return;
	va_start(ap , Format);
	Return = VSPrintf(Buffer , Format , ap);
	return Return;
}

int VSPrintf(char *Buffer , const char *Format , va_list ap) {
	QWORD i;
	QWORD j;
	QWORD k;
	int BufferIndex = 0;
	int FormatLength;
	int CopyLength;
	char *CopyString;
	QWORD Val;
	int ValI;
	double ValD;
	FormatLength = StrLen(Format);
	for(i = 0; i< FormatLength; i++) {
		if(Format[i] == '%') {
			i++;
			switch(Format[i]) {
				case 's':
					CopyString = (char*)(va_arg(ap , char*));
					CopyLength = StrLen(CopyString);
					MemCpy(Buffer+BufferIndex , CopyString , CopyLength);
					BufferIndex += CopyLength;
					break;
				case 'c':
					Buffer[BufferIndex] = (char)(va_arg(ap , int));
					BufferIndex++;
					break;
				case 'd':
				case 'i':
					Val = (int)(va_arg(ap , int));
					BufferIndex += itoa(Val , Buffer+BufferIndex , 10);
					break;
				case 'x':
				case 'X':
					Val = (DWORD)(va_arg(ap , DWORD)) & 0xFFFFFFFF;
					BufferIndex += itoa(Val , Buffer+BufferIndex , 16);
					break;
				case 'q':
				case 'Q':
				case 'p':
					Val = (QWORD)(va_arg(ap , QWORD));
					BufferIndex += itoa(Val , Buffer+BufferIndex , 16);
					break;
				case 'f':
					ValD = (double)(va_arg(ap , double));
					ValD += 0.00000000005;
					Buffer[BufferIndex] = '0'+(QWORD)(ValD*10000000000) % 10;
					Buffer[BufferIndex+1] = '0'+(QWORD)(ValD*1000000000) % 10;
					Buffer[BufferIndex+2] = '0'+(QWORD)(ValD*100000000) % 10;
					Buffer[BufferIndex+3] = '0'+(QWORD)(ValD*10000000) % 10;
					Buffer[BufferIndex+4] = '0'+(QWORD)(ValD*1000000) % 10;
					Buffer[BufferIndex+5] = '0'+(QWORD)(ValD*100000) % 10;
					Buffer[BufferIndex+6] = '0'+(QWORD)(ValD*10000) % 10;
					Buffer[BufferIndex+7] = '0'+(QWORD)(ValD*1000) % 10;
					Buffer[BufferIndex+8] = '0'+(QWORD)(ValD*100) % 10;
					Buffer[BufferIndex+9] = '0'+(QWORD)(ValD*10) % 10;
					Buffer[BufferIndex+10] = '.';
					for(k = 0; ; k++) {
						if(((QWORD)ValD == 0) && (k != 0)) {
							break;
						}
						Buffer[BufferIndex+11+k] = '0'+((QWORD)ValD % 10);
						ValD = ValD/10;
					}
					Buffer[BufferIndex+11+k] = '\0';
					ReverseString(Buffer+BufferIndex);
					BufferIndex += 11+k;
					break;
				case 'b':
					ValI = (int)(va_arg(ap , int));
					if(ValI == TRUE) {
						Buffer[BufferIndex] = 't';
						Buffer[BufferIndex+1] = 'r';
						Buffer[BufferIndex+2] = 'u';
						Buffer[BufferIndex+3] = 'e';
						BufferIndex += 4;
						break;
					}
					else {
						Buffer[BufferIndex] = 'f';
						Buffer[BufferIndex+1] = 'a';
						Buffer[BufferIndex+2] = 'l';
						Buffer[BufferIndex+3] = 's';
						Buffer[BufferIndex+4] = 'e';
						BufferIndex += 5;
						break;
					}
					break;
				case 'B':
					
					ValI = (int)(va_arg(ap , int));
					if(ValI == TRUE) {
						Buffer[BufferIndex] = 'T';
						Buffer[BufferIndex+1] = 'R';
						Buffer[BufferIndex+2] = 'U';
						Buffer[BufferIndex+3] = 'E';
						BufferIndex += 4;
						break;
					}
					else {
						Buffer[BufferIndex] = 'F';
						Buffer[BufferIndex+1] = 'A';
						Buffer[BufferIndex+2] = 'L';
						Buffer[BufferIndex+3] = 'S';
						Buffer[BufferIndex+4] = 'E';
						BufferIndex += 5;
						break;
					}
					break;
				default:
					Buffer[BufferIndex] = Format[i];
					BufferIndex++;
					break;
			}
		}
		else {
			Buffer[BufferIndex] = Format[i];
			BufferIndex++;
		}
	}
	Buffer[BufferIndex] = '\0';
	return BufferIndex;
}

BOOL IsEqual(char *a , const char *b) {
	int i;
	for(i = 0; a[i] != 0||b[i] != 0; i++) {
		if(a[i] != b[i]) {
			return FALSE;
		}
	}
	return TRUE;
}

QWORD GetTickCount(void) {
	return TickCount;
}

void Sleep(QWORD MilliSecond) {
	QWORD LastTickCount;
	LastTickCount = TickCount;
	while((TickCount-LastTickCount) <= MilliSecond) {
		Schedule();
	}
}

static volatile QWORD RandomVal = 0;

static QWORD holdrand = 1531016791;

QWORD Random(void) {
	RandomVal = (((holdrand = holdrand*214013L + 2531011L) >> 16) & 0x7FFF);
	return RandomVal;
}

int pow(int base , int exp) {
	int Return = 1;
	if(exp == 0) {
		return 1;
	}
	else if(exp < 0) {
		return 1;
	}
	int temp;
	temp = pow(base , exp/2);
	if((exp % 2) == 0) {
		return temp*temp;
	}
	else {
		return (base*temp*temp);
	}	
}

char *StrCpy(char *dest , const char *src) {
	char *TempDest = dest;
	const char *TempSrc = src;
	while(*TempSrc != NULL) {
		*TempDest++ = *TempSrc++;
	}
	return dest;
}

char *StrCat(char *dest , const char *src) {
	int i = 0;
	int j = 0;
	while(dest[i] != '\0') {
		i++;
	}
	while(src[i] != '\0') {
		dest[i++] = src[j++];
	}
	dest[i] = '\0';
	return dest;
}

BYTE Gets(CONSOLECOLOR Color , char StopKey , char Return[1024] , OPTION Option) {
	char CommandBuffer[1000];
	int CommandBufferIndex = 0;
	BYTE Key;
	int CursorX;
	int CursorY;
	while(1) {
		Key = GetCh();
		if(Key == KEY_BACKSPACE) {
			if(CommandBufferIndex > 0) {
				GetCursor(&CursorX , &CursorY);
				if(CursorX != 0) {
					KernelPrintfXY(Color , CursorX-1 , CursorY , " ");
					SetCursor(CursorX-1 , CursorY);
					CommandBufferIndex--;
				}
			}
		}
		else if(Key == StopKey) {
			Printf(Color , "\n");
			if(CommandBufferIndex > 0) {
				CommandBuffer[CommandBufferIndex] = '\0';
				StrCpy(Return , CommandBuffer);
			}
			MemSet(CommandBuffer , '\0' , 1000);
			CommandBufferIndex = 0;
			return StopKey;
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
		else {
			if(Key == KEY_TAB) {
				Key = ' ';
			}
			if(CommandBufferIndex < 1000) {
				CommandBuffer[CommandBufferIndex++] = Key;
				if(Option == PASSWORD) {
					Printf(Color , "*");
				}
				else {
					Printf(Color , "%c" , Key);
				}
			}
		}
	}
}

BOOL Paused(CONSOLECOLOR Color) {
	Printf(Color , "<Paused('e' Key to Exit)>\n");
	if(GetCh() == 'e') {
		int X;
		int Y;
		GetCursor(&X , &Y);
		SetCursor(X , Y-1);
		Printf(Color , "                         \n");
		GetCursor(&X , &Y);
		SetCursor(X , Y-1);
		return TRUE;
	}
	int X;
	int Y;
	GetCursor(&X , &Y);
	SetCursor(X , Y-1);
	Printf(Color , "                         \n");
	GetCursor(&X , &Y);
	SetCursor(X , Y-1);
	return FALSE;
}

int FPrintf(FILE *File , const char *Format , ...) {
	va_list ap;
	char Buffer[1024];
	va_start(ap , Format);
	VSPrintf(Buffer , Format , ap);
	va_end(ap);
	return fwrite(Buffer , StrLen(Buffer) , 1 , File);
}

void LockString(char String[1000] , QWORD LockNumber) {
	int i;
	for(i = 0; String[i] != 0; i++) {
		String[i] ^= LockNumber;
	}
}

BOOL IsFileExist(const char *FileName) {
	FILE *Check;
	Check = fopen(FileName , "r");
	if(Check == NULL) {
		fclose(Check);
		return FALSE;
	}
	else {
		fclose(Check);
		return TRUE;
	}
	fclose(Check);
	return TRUE;
}

int GetFileSize(const char *FileName) {
	FILE *fp;
	fp = fopen(FileName , "r");
	if(fp == 0) {
		fclose(fp);
		return 0;
	}
	fclose(fp);
	int FileSize = 0;
	DIR *Directory;
	struct dirent *Entry;
	Directory = opendir("/");
	if(Directory == NULL) {
		return 0;
	}
	rewinddir(Directory);
	while(1) {
		Entry = readdir(Directory);
		if(Entry == NULL) {
			break;
		}
		if(IsEqual(Entry->d_name , FileName) == TRUE) {
			FileSize = Entry->FileSize;
		}
	}
	closedir(Directory);
	return FileSize;
}

static volatile QWORD fuerh = 0;

void CheckProcessorSpeed(void) {
	int i;
	QWORD LastTSC;
	QWORD TotalTSC = 0;
	DisableInterrupt();
	for(i = 0; i < 200; i++) {
		LastTSC = ReadTSC();
		Printf(COLOR_BLUE_WHITE , "#");
		WaitUsingDirectPIT(MSTOCOUNT(50));
		TotalTSC += ReadTSC()-LastTSC;
	}
	InitPIT(MSTOCOUNT(1) , TRUE);
	EnableInterrupt();
	fuerh = TotalTSC;
}

QWORD GetProcessorSpeed(void) {
	return fuerh;
}

BOOL IsGraphicMode(void) {
	if(*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0) {
		return FALSE;
	}
	return  TRUE;
}
