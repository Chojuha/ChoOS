#ifndef _VBE_H_
#define _VBE_H_

#include "Types.h"

#define VBE_MODEINFOBLOCKADDRESS 0x7E00
#define VBE_STARTGRAPHICMODEFLAGADDRESS 0x7C0A

#pragma pack(push , 1)

typedef struct VBEInfoBlockStruct {
	WORD ModeAttribute;
	BYTE WinAAttribute;
	BYTE WinBAttribute;
	WORD WinGranlity;
	WORD WinSize;
	WORD WinASegment;
	WORD WinBSegment;
	DWORD WinFuncPtr;
	WORD BytesPerScanLine;
	
	WORD XResolution;
	WORD YResolution;
	BYTE XCharSize;
	BYTE YCharSize;
	BYTE NumberOfPlane;
	BYTE BitsPerPixel;
	BYTE NumberofBanks;
	BYTE MemoryModel;
	BYTE BankSize;
	BYTE NumberOfImagePages;
	BYTE Reserved;
	
	BYTE RedMaskSize;
	BYTE RedFeldPosition;
	BYTE GreenMaskSize;
    BYTE GreenFieldPosition;
    BYTE BlueMaskSize;
    BYTE BlueFieldPosition;
    BYTE ReservedMaskSize;
    BYTE ReservedFieldPosition;
    BYTE DirectColorModeInfo;
    
    DWORD PhysicalBasePointer;
    DWORD Reserved1;
    DWORD Reserved2;
    
    WORD LinearBytesPerScanLine;
    BYTE BankNumberOfImagePages;
    BYTE LinearNumberOfImagePages;
    BYTE LinearRedMaskSize;
    BYTE LinearRedFieldPosition;
    BYTE LinearGreenMaskSize;
    BYTE LinearGreenFieldPosition;
    BYTE LinearBlueMaskSize;
    BYTE LinearBlueFieldPosition;
    BYTE LinearReservedMaskSize;
    BYTE LinearReservedFieldPosition;
    DWORD MaxPixelClock;
    BYTE DfReserved[189];
}VBEMODEINFOBLOCK;

#pragma pack(pop)

VBEMODEINFOBLOCK *GetVBEInfoBlock(void);

#endif
