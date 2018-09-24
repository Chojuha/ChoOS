#ifndef _2DGRAPHICS_H_
#define _2DGRAPHICS_H_

#include "Types.h"

typedef WORD COLOR;

#define RGB(red , green , blue) (((BYTE)(red) >> 3) << 11|(((BYTE)(green) >> 2)) << 5|((BYTE)(blue) >> 3))

typedef struct RectangeStruct {
	int X1;
	int Y1;
	int X2;
	int Y2;
}RECT;

typedef struct PointStruct {
	int X;
	int Y;
}POINT;

inline BOOL IsInRectangle(const RECT *Area , int X , int Y);
inline int GetRectangleWidth(const RECT *Area);
inline int GetRectangleHeight(const RECT *Area);
inline BOOL IsRectangleOverLapped(const RECT *Area1 , const RECT *Area2);
inline BOOL GetOverLappedRectangle(const RECT *Area1 , const RECT *Area2 , RECT *InterSection);
inline void SetRectangleData(int X1 , int Y1 , int X2 , int Y2 , RECT *Rect);
inline void InternalDrawPixel(const RECT *MemoryArea , COLOR *MemoryAddress , int X , int Y , COLOR Color);
void InternalDrawLine(const RECT *MemoryArea , COLOR *MemoryAddress , int X1 , int Y1 , int X2 , int Y2 , COLOR Color);
void InternalDrawRect(const RECT *MemoryArea , COLOR *MemoryAddress , int X1 , int Y1 , int X2 , int Y2 , COLOR Color , BOOL Fill);
void InternalDrawCircle(const RECT *MemoryArea , COLOR *MemoryAddress , int X , int Y , int Radius , COLOR Color , BOOL Fill);
void InternalDrawText(const RECT *MemoryArea , COLOR *MemoryAddress , int X , int Y , COLOR TextColor , COLOR BackgroundColor , const char *String , int Length);
void InternalDrawTextInFont(const RECT *MemoryArea , COLOR *MemoryAddress , int X , int Y , COLOR TextColor , COLOR BackgroundColor , const char *String , int Length , int FontWidth , int FontHeight , unsigned char Font[]);

#endif
