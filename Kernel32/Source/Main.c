#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void nop(void);
void KernelPrintf(int Color , int X , int Y , const char *String);
BOOL InitKernel64(void);
BOOL CheckMemory(void);
void CopyKernel64ImageTo2MByte(void);

#define BOOTSTRAPPROCESSOR_FLAGADDRESS 0x7C09

void Main(void) {
	DWORD i;
	DWORD EAX , EBX , ECX , EDX;
	char VendorString[13] = {0 , };
	if(*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0) {
		SwitchAndExecute64bitKernel();
		while(1);
	}
	if(CheckMemory() == FALSE) {
		KernelPrintf(0x04 , 0 , 2 , "[!] Error:Memory Space is Small(CEHCK_MEMORYSPACE)");
		while(1);
	}
	if(InitKernel64() == FALSE) {
		KernelPrintf(0x04 , 0 , 2 , "[!] Error:Kernel64 Initializaing Failed(INITERR_KERNEL64)");
		while(1);
	}
	InitPageTables();
	ReadCPUID(0x00, &EAX, &EBX, &ECX, &EDX);
    *(DWORD*)VendorString = EBX;
    *((DWORD*)VendorString + 1) = EDX;
    *((DWORD*)VendorString + 2) = ECX;
    KernelPrintf(0x07 , 0 , 2 , "[I] Vendor String:[");
    int k;
	for(k = 0; VendorString[k] != 0; k++) {
		KernelPrintf(0x07 , 19+k , 3 , " ");
	}
	KernelPrintf(0x07 , 19+k , 2 , "]");
    KernelPrintf(0x07 , 19 , 2 , VendorString);
    ReadCPUID(0x80000001 , &EAX , &EBX , &ECX , &EDX);
    if(EDX & (1 << 29)) {
		nop();
    }
    else {
        KernelPrintf(0x04 , 0 , 3 , "[!] Error:64Bit is Not Supported this pc(NSPT_N64)");
        while(1);
    }
    CopyKernel64ImageTo2MByte();
	SwitchAndExecute64bitKernel();
	while(1);
}

void nop(void) {
	return;
}

void KernelPrintf(int Color , int X , int Y , const char *String) {
	CHARACTER *Address;
	Address = (CHARACTER*)0xB8000;
	int i;
	Address += (Y*80)+X;
	for(i = 0; String[i] != 0; i++) {
		Address[i].Charactor = String[i];
		Address[i].Attribute = Color;
	}
}

BOOL InitKernel64(void) {
	DWORD *Address;
	Address = (DWORD*)0x100000;
	while((DWORD)Address < 0x600000) {
		*Address = 0x00;
		if(*Address != 0) {
			return FALSE;
		}
		Address++;
	}
	return TRUE;
}

BOOL CheckMemory(void) {
	DWORD *Address;
	Address = (DWORD*)0x100000;
	while((DWORD)Address < 0x4000000) {
		*Address = 0x12345678;
		if(*Address != 0x12345678) {
			return FALSE;
		}
		Address += (0x100000/4);
	}
	return TRUE;
}

void CopyKernel64ImageTo2MByte(void) {
	WORD Kernel32SectorCount , TotalKernelSectorCount;
	DWORD* SourceAddress ,* DestiantionAddress;
	int i;
	TotalKernelSectorCount = *((WORD*)0x7C05);
	Kernel32SectorCount = *((WORD*)0x7C07);
	SourceAddress = (DWORD*)(0x10000+(Kernel32SectorCount*512));
	DestiantionAddress = (DWORD*)0x200000;
	for(i = 0; i < 512*(TotalKernelSectorCount-Kernel32SectorCount)/4; i++) {
		*DestiantionAddress = *SourceAddress;
		DestiantionAddress++;
		SourceAddress++;
	}
}
