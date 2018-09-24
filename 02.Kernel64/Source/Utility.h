#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <stdarg.h>
#include "Types.h"
#include "FileSystem.h"

#define MIN(a , b) (((a) < (b)) ? (a):(b))
#define MAX(a , b) (((a) > (b)) ? (a):(b))
#define DEFAULT 0x01
#define PASSWORD 0x02
#define UnlockString LockString
#define FPrintf fprintf

void MemSet(void *Destination , BYTE Data , int Size);
inline void MemSetWord(void *Destination , WORD Data , int WordSize);
int MemCpy(void *Destination , const void *Source , int Size);
int MemCmp(const void *Destination , const void *Source , int Size);
BOOL SetInterruptFlag(BOOL EnableInterrupt);
void CheckTotalRamSize(void);
QWORD GetTotalRamSize(void);
void ReverseString(char *Buffer);
long atoi(const char *Buffer , int Radix);
QWORD HexStringToQword(const char *Buffer);
long DecimalStringToLong(const char *Buffer);
int itoa(long Val , char *Buffer , int Radix);
int HexToString(QWORD Val , char *Buffer);
int DecimalToString(long Val , char *Buffer);
int SPrintf(char *Buffer , const char *Format , ...);
int VSPrintf(char *Buffer , const char *Format , va_list ap);
int FPrintf(FILE *File , const char *Format , ...);
BOOL IsEqual(char *a , const char *b);
QWORD GetTickCount(void);
extern volatile QWORD TickCount;
void Sleep(QWORD MilliSecond);
QWORD Random(void);
int pow(int base , int exp);
char *StrCpy(char *dest , const char *src);
char *StrCat(char *dest , const char *src);
BYTE Gets(CONSOLECOLOR Color , char StopKey , char Return[1024] , OPTION Option);
BOOL Paused(CONSOLECOLOR Color);
void LockString(char String[1000] , QWORD LockNumber);
void nop(void);
BOOL IsFileExist(const char *FileName);
int GetFileSize(const char *FileName);
void CheckProcessorSpeed(void);
QWORD GetProcessorSpeed(void);

BOOL IsGraphicMode(void);

#endif 
