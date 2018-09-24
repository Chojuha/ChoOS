#include "2DGraphics.h"
#include "VBE.h"
#include "Font.h"
#include "Utility.h"

inline BOOL IsInRectangle(const RECT *Area , int X , int Y) {
	if((X < Area->X1)||(Area->X2 < X)||(Y < Area->Y1)||(Area->Y2 < Y)) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

inline int GetRectangleWidth(const RECT *Area) {
	int Width;
	Width = Area->X2-Area->X1+1;
	if(Width < 0) {
		return -Width;
	}
	return Width;
}

inline int GetRectangleHeight(const RECT *Area) {
	int Height;
	Height = Area->Y2-Area->Y1+1;
	if(Height < 0) {
		return -Height;
	}
	return Height;
}

inline BOOL IsRectangleOverLapped(const RECT *Area1 , const RECT *Area2) {
	if((Area1->X1 > Area2->X2)||(Area1->X2 < Area2->X1)||(Area1->Y1 > Area2->Y2)||(Area1->Y2 < Area2->Y1)) {
		return FALSE;
	}
	return TRUE;
}

inline BOOL GetOverLappedRectangle(const RECT *Area1 , const RECT *Area2 , RECT *InterSection) {
	int MaxX1;
	int MinX2;
	int MaxY1;
	int MinY2;
	MaxX1 = MAX(Area1->X1 , Area2->X1);
	MinX2 = MIN(Area1->X2 , Area2->X2);
	if(MinX2 < MaxX1) {
		return FALSE;
	}
	MaxY1 = MAX(Area1->Y1 , Area2->Y1);
	MinY2 = MIN(Area1->Y2 , Area2->Y2);
	if(MinY2 < MaxY1) {
		return FALSE;
	}
	InterSection->X1 = MaxX1;
	InterSection->Y1 = MaxY1;
	InterSection->X2 = MinX2;
	InterSection->Y2 = MinY2;
	return TRUE;
}

inline void SetRectangleData(int X1 , int Y1 , int X2 , int Y2 , RECT *Rect) {
	if(X1 < X2) {
		Rect->X1 = X1;
		Rect->X2 = X2;
	}
	else {
		Rect->X1 = X2;
		Rect->X2 = X1;
	}
	if(Y1 < Y2) {
		Rect->Y1 = Y1;
		Rect->Y2 = Y2;
	}
	else {
		Rect->Y1 = Y2;
		Rect->Y2 = Y1;
	}
}
	
inline void InternalDrawPixel(const RECT *MemoryArea , COLOR *MemoryAddress , int X , int Y , COLOR Color) {
	int Width;
	if(IsInRectangle(MemoryArea , X , Y) == FALSE) {
		return;
	}
	Width = GetRectangleWidth(MemoryArea);
	*(MemoryAddress+(Width*Y)+X) = Color;
}

void InternalDrawLine(const RECT *MemoryArea , COLOR *MemoryAddress , int X1 , int Y1 , int X2 , int Y2 , COLOR Color) {
	int dX;
	int dY;
	int Error = 0;
	int dError;
	int X;
	int Y;
	int StepX;
	int StepY;
	RECT LineArea;
	SetRectangleData(X1 , Y1 , X2 , Y2 , &LineArea);
	if(IsRectangleOverLapped(MemoryArea , &LineArea) == FALSE) {
		return;
	}
	dX = X2-X1;
	dY = Y2-Y1;
	if(dX < 0) {
		dX = -dX;
		StepX = -1;
	}
	else {
		StepX = 1;
	}
	if(dY < 0) {
		dY = -dY;
		StepY = -1;
	}
	else {
		StepY = 1;
	}
	if(dX > dY) {
		dError = dY << 1;
		Y = Y1;
		for(X = X1; X != X2; X += StepX) {
			InternalDrawPixel(MemoryArea , MemoryAddress , X , Y , Color);
			Error += dError;
			if(Error >= dX) {
				Y += StepY;
				Error -= dX << 1;
			}
		}
		InternalDrawPixel(MemoryArea , MemoryAddress , X , Y , Color);
	}
	else {
		dError = dX << 1;
		X = X1;
		for(Y = Y1; Y != Y2; Y += StepY) {
			InternalDrawPixel(MemoryArea , MemoryAddress , X , Y , Color);
			Error += dError;
			if(Error >= dY) {
				X += StepX;
				Error -= dY << 1;
			}
		}
		InternalDrawPixel(MemoryArea , MemoryAddress , X , Y , Color);
	}
}

void InternalDrawRect(const RECT *MemoryArea , COLOR *MemoryAddress , int X1 , int Y1 , int X2 , int Y2 , COLOR Color , BOOL Fill) {
	int Width;
	int Temp;
	int Y;
	int MemoryAreaWidth;
	int StepY;
	COLOR *VideoMemoryAddress;
	VBEMODEINFOBLOCK *ModeInfo;
	RECT DrawRect;
	RECT OverLappedArea;
	if(Fill == FALSE) {
		InternalDrawLine(MemoryArea , MemoryAddress , X1 , Y1 , X2 , Y1 , Color);
		InternalDrawLine(MemoryArea , MemoryAddress , X1 , Y1 , X1 , Y2 , Color);
		InternalDrawLine(MemoryArea , MemoryAddress , X2 , Y1 , X2 , Y2 , Color);
		InternalDrawLine(MemoryArea , MemoryAddress , X1 , Y2 , X2 , Y2 , Color);
	}
	else {
		SetRectangleData(X1 , Y1 , X2 , Y2 , &DrawRect);
		if(GetOverLappedRectangle(MemoryArea , &DrawRect , &OverLappedArea) == FALSE) {
			return;
		}
		Width = GetRectangleWidth(&OverLappedArea);
		MemoryAreaWidth = GetRectangleWidth(MemoryArea);
		MemoryAddress += OverLappedArea.Y1*MemoryAreaWidth+OverLappedArea.X1;
		for(Y = OverLappedArea.Y1; Y < OverLappedArea.Y2; Y++) {
			MemSetWord(MemoryAddress , Color , Width);
			MemoryAddress += MemoryAreaWidth;
		}
		MemSetWord(MemoryAddress , Color , Width);
	}
}

void InternalDrawCircle(const RECT *MemoryArea , COLOR *MemoryAddress , int X , int Y , int Radius , COLOR Color , BOOL Fill) {
	int CX;
	int CY;
	int Distance;
	if(Radius < 0) {
		return;
	}
	CY = Radius;
	if(Fill == FALSE) {
		InternalDrawPixel(MemoryArea , MemoryAddress , 0+X , Radius+Y , Color);
		InternalDrawPixel(MemoryArea , MemoryAddress , 0+X , -Radius+Y , Color);
		InternalDrawPixel(MemoryArea , MemoryAddress , Radius+X , 0+Y , Color);
		InternalDrawPixel(MemoryArea , MemoryAddress , -Radius+X , 0+Y , Color);
	}
	else {
		InternalDrawLine(MemoryArea , MemoryAddress , 0+X , Radius+Y , 0+X , -Radius+Y , Color);
		InternalDrawLine(MemoryArea , MemoryAddress , Radius+X , 0+Y , -Radius+X , 0+Y , Color);
	}
	Distance = -Radius;
	for(CX = 1; CX <= CY; CX++) {
		Distance += (CX << 1)-1;
		if(Distance >= 0) {
			CY--;
			Distance += (-CY << 1)+2;
		}
		if(Fill == FALSE) {
			InternalDrawPixel(MemoryArea , MemoryAddress , CX+X , CY+Y , Color);
			InternalDrawPixel(MemoryArea , MemoryAddress , CX+X , -CY+Y , Color);
			InternalDrawPixel(MemoryArea , MemoryAddress , -CX+X , CY+Y , Color);
			InternalDrawPixel(MemoryArea , MemoryAddress , -CX+X , -CY+Y , Color);
			InternalDrawPixel(MemoryArea , MemoryAddress , CY+X , CX+Y , Color);
			InternalDrawPixel(MemoryArea , MemoryAddress , CY+X , -CX+Y , Color);
			InternalDrawPixel(MemoryArea , MemoryAddress , -CY+X , CX+Y , Color);
			InternalDrawPixel(MemoryArea , MemoryAddress , -CY+X , -CX+Y , Color);
		}
		else {
			InternalDrawRect(MemoryArea , MemoryAddress , -CX+X , CY+Y , CX+X , CY+Y , Color , TRUE);
			InternalDrawRect(MemoryArea , MemoryAddress , -CX+X , -CY+Y , CX+X , -CY+Y , Color , TRUE);
			InternalDrawRect(MemoryArea , MemoryAddress , -CY+X , -CX+Y , CY+X , CX+Y , Color , TRUE);
			InternalDrawRect(MemoryArea , MemoryAddress , -CY+X , -CX+Y , CY+X , -CX+Y , Color , TRUE);
		}
	}
}

void InternalDrawText(const RECT *MemoryArea , COLOR *MemoryAddress , int X , int Y , COLOR TextColor , COLOR BackgroundColor , const char *String , int Length) {
	int CurrentX;
	int CurrentY;
	int i;
	int j;
	int k;
	BYTE Bitmap;
	BYTE CurrentBitmask;
	int BitmapStartIndex;
	int MemoryAreaWidth;
	RECT FontArea;
	RECT OverLappedArea;
	int StartYOffset;
	int StartXOffset;
	int OverLappedWidth;
	int OverLappedHeight;
	CurrentX = X;
	MemoryAreaWidth = GetRectangleWidth(MemoryArea);
	for(k = 0; k < Length; k++) {
		CurrentY = Y*MemoryAreaWidth;
		SetRectangleData(CurrentX , Y , CurrentX+FONT_ENGLISHWIDTH-1 , Y+FONT_ENGLISHHEIGHT-1 , &FontArea);
		if(GetOverLappedRectangle(MemoryArea , &FontArea , &OverLappedArea) == FALSE) {
			CurrentX += FONT_ENGLISHWIDTH;
			continue;
		}
		BitmapStartIndex = String[k]*FONT_ENGLISHHEIGHT;
		StartXOffset = OverLappedArea.X1-CurrentX;
		StartYOffset = OverLappedArea.Y1-Y;
		OverLappedWidth = GetRectangleWidth(&OverLappedArea);
		OverLappedHeight = GetRectangleHeight(&OverLappedArea);
		BitmapStartIndex += StartYOffset;
		for(j = StartYOffset; j < OverLappedHeight; j++) {
			Bitmap = EnglishFont[BitmapStartIndex++];
			CurrentBitmask = 0x01 << (FONT_ENGLISHWIDTH-1-StartXOffset); 
			for(i = StartXOffset; i < OverLappedWidth; i++) {
				if(Bitmap & CurrentBitmask) {
					MemoryAddress[CurrentY+CurrentX+i] = TextColor;
				}
				else {
					MemoryAddress[CurrentY+CurrentX+i] = BackgroundColor;
				}
				CurrentBitmask = CurrentBitmask >> 1;
			}
			CurrentY += MemoryAreaWidth;
		}         
		CurrentX += FONT_ENGLISHWIDTH;
	}
}

void InternalDrawTextInFont(const RECT *MemoryArea , COLOR *MemoryAddress , int X , int Y , COLOR TextColor , COLOR BackgroundColor , const char *String , int Length , int FontWidth , int FontHeight , unsigned char Font[]) {
	int CurrentX;
	int CurrentY;
	int i;
	int j;
	int k;
	BYTE Bitmap;
	BYTE CurrentBitmask;
	int BitmapStartIndex;
	int MemoryAreaWidth;
	RECT FontArea;
	RECT OverLappedArea;
	int StartYOffset;
	int StartXOffset;
	int OverLappedWidth;
	int OverLappedHeight;
	CurrentX = X;
	MemoryAreaWidth = GetRectangleWidth(MemoryArea);
	for(k = 0; k < Length; k++) {
		CurrentY = Y*MemoryAreaWidth;
		SetRectangleData(CurrentX , Y , CurrentX+FontWidth-1 , Y+FontHeight-1 , &FontArea);
		if(GetOverLappedRectangle(MemoryArea , &FontArea , &OverLappedArea) == FALSE) {
			CurrentX += FontWidth;
			continue;
		}
		BitmapStartIndex = String[k]*FontHeight;
		StartXOffset = OverLappedArea.X1-CurrentX;
		StartYOffset = OverLappedArea.Y1-Y;
		OverLappedWidth = GetRectangleWidth(&OverLappedArea);
		OverLappedHeight = GetRectangleHeight(&OverLappedArea);
		BitmapStartIndex += StartYOffset;
		for(j = StartYOffset; j < OverLappedHeight; j++) {
			Bitmap = Font[BitmapStartIndex++];
			CurrentBitmask = 0x01 << (FontWidth-1-StartXOffset); 
			for(i = StartXOffset; i < OverLappedWidth; i++) {
				if(Bitmap & CurrentBitmask) {
					MemoryAddress[CurrentY+CurrentX+i] = TextColor;
				}
				else {
					MemoryAddress[CurrentY+CurrentX+i] = BackgroundColor;
				}
				CurrentBitmask = CurrentBitmask >> 1;
			}
			CurrentY += MemoryAreaWidth;
		}         
		CurrentX += FontWidth;
	}
}
