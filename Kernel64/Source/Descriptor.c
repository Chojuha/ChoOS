#include "Descriptor.h"
#include "Utility.h"
#include "ISR.h"
#include "MultiProcessor.h"

void InitGDTTableAndTSS(void) {
	GDTR *Gdtr;
	GDTENTRY8 *Entry;
	TSSSEGMENT *TSS;
	int i;
	Gdtr = (GDTR*)GDTR_STARTADDRESS;
	Entry = (GDTENTRY8*)(GDTR_STARTADDRESS+sizeof(GDTR));
	Gdtr->Limit = GDT_TABLESIZE-1;
	Gdtr->BaseAddress = (QWORD)Entry;
	TSS = (TSSSEGMENT*)((QWORD)Entry+GDT_TABLESIZE);
	SetGDTEntry8(&(Entry[0]) , 0 , 0 , 0 , 0 , 0);
	SetGDTEntry8(&(Entry[1]) , 0 , 0xFFFFF , GDT_FLAGS_UPPER_CODE , GDT_FLAGS_LOWER_KERNELCODE , GDT_TYPE_CODE);
	SetGDTEntry8(&(Entry[2]) , 0 , 0xFFFFF , GDT_FLAGS_UPPER_DATA , GDT_FLAGS_LOWER_KERNELDATA , GDT_TYPE_DATA);
	for(i = 0; i < MAXPROCESSORCOUNT; i++) {
		SetGDTEntry16((GDTENTRY16*)&(Entry[GDT_MAXENTRY8COUNT+(i*2)]) , (QWORD)TSS+(i*sizeof(TSSSEGMENT)) , sizeof(TSSSEGMENT)-1 , GDT_FLAGS_UPPER_TSS , GDT_FLAGS_LOWER_TSS , GDT_TYPE_TSS);
	}
	InitTSSSegment(TSS);
}

void SetGDTEntry8(GDTENTRY8 *Entry , DWORD BaseAddress , DWORD Limit , BYTE UpperFlags , BYTE LowerFlags , BYTE Type) {
	Entry->LowerLimit = Limit & 0xFFFF;
	Entry->LowerBaseAddress = BaseAddress & 0xFFFF;
	Entry->UpperBaseAddress1 = (BaseAddress >> 16) & 0xFF;
	Entry->TypeAndLowerFlag = LowerFlags|Type;
	Entry->UpperLimitAndUpperFlag = ((Limit >> 16) & 0x0F)|UpperFlags;
	Entry->UpperBaseAddress2 = (BaseAddress >> 24) & 0xFF; 
}

void SetGDTEntry16(GDTENTRY16 *Entry , QWORD BaseAddress , DWORD Limit , BYTE UpperFlags , BYTE LowerFlags , BYTE Type) {
	Entry->LowerLimit = Limit & 0xFFFF;
	Entry->LowerBaseAddress = BaseAddress & 0xFFFF;
	Entry->MiddleBaseAddress1 = (BaseAddress >> 16) & 0xFF;
	Entry->TypeAndLowerFlag = LowerFlags|Type;
	Entry->UpperLimitAndUpperFlag = ((Limit >> 16) & 0x0F)|UpperFlags;
	Entry->MiddleBaseAddress2 = (BaseAddress >> 24) & 0xFF; 
	Entry->UpperBaseAddress = BaseAddress >> 32;
	Entry->Reserved = 0;
}

void InitTSSSegment(TSSSEGMENT *TSS) {
	int i;
	for(i = 0; i < MAXPROCESSORCOUNT; i++) {
		MemSet(TSS , 0 , sizeof(TSSSEGMENT));
		TSS->IST[0] = IST_STARTADDRESS+IST_SIZE-(IST_SIZE/MAXPROCESSORCOUNT*i);
		TSS->IOMapBaseAddress = 0xFFFF;
		TSS++;
	}
}

void InitIDTTable(void) {
	IDTR *Idtr;
	IDTENTRY *Entry;
	int i;
	Idtr = (IDTR*)IDTR_STARTADDRESS;
	Entry = (IDTENTRY*)(IDTR_STARTADDRESS+sizeof(IDTR));
	Idtr->BaseAddress = (QWORD)Entry;
	Idtr->Limit = IDT_TABLESIZE-1;
	SetIDTEntry(&(Entry[0]) , ISRDivideError , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[1]) , ISRDebug , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[2]) , ISRNMI , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[3]) , ISRBreakPoint , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[4]) , ISROverflow , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[5]) , ISRBoundRangeExceeded , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[6]) , ISRInvalidOpcode , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[7]) , ISRDeviceNotAvailable , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[8]) , ISRDoubleFault , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[9]) , ISRCoprocessorSegmentOverrun , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[10]) , ISRInvalidTSS , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[11]) , ISRSegmentNotPresent , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[12]) , ISRStackSegmentFault , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[13]) , ISRGeneralProtection , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[14]) , ISRPageFault , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[15]) , ISR15 , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[16]) , ISRFPUError , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[17]) , ISRAlignmentCheck , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[18]) , ISRMachineCheck , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[19]) , ISRSIMDError , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[20]) , ISRETCException , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	for(i = 21; i < 32; i++) {
		SetIDTEntry(&(Entry[i]) , ISRETCException , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	}
	SetIDTEntry(&(Entry[32]) , ISRTimer , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[33]) , ISRKeyboard , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[34]) , ISRSlavePIC , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[35]) , ISRSerial2 , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[36]) , ISRSerial1 , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[37]) , ISRParallel2 , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[38]) , ISRFloppy , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[39]) , ISRParallel1 , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[40]) , ISRRTC , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[41]) , ISRReserved , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[42]) , ISRNotUsed1 , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[43]) , ISRNotUsed2 , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[44]) , ISRMouse , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[45]) , ISRCoprocessor , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[46]) , ISRHDD1 , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	SetIDTEntry(&(Entry[47]) , ISRHDD2 , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	for(i = 48; i < IDT_MAXENTRYCOUNT; i++) {
		SetIDTEntry(&(Entry[i]) , ISRETCInterrupt , 0x08 , IDT_FLAGS_IST1 , IDT_FLAGS_KERNEL , IDT_TYPE_INTERRUPT);
	}
}

void SetIDTEntry(IDTENTRY *Entry , void *Handler , WORD Selector , BYTE IST , BYTE Flags , BYTE Types) {
	Entry->LowerBaseAddress = (QWORD)Handler & 0xFFFF;
	Entry->SegmentSelector = Selector;
	Entry->IST = IST & 0x3;
	Entry->TypeAndFlags = Types|Flags;
	Entry->MiddleBaseAddress = ((QWORD)Handler >> 16) & 0xFFFF;
	Entry->UpperBaseAddress = (QWORD)Handler >> 32;
	Entry->Reserved = 0;
}

void DummyHandler(void) {
	int i;
	for(i = 0; i <= 80*25*2; i++) {
		KernelPrintfXY(0x40 , i , 0 , " ");
	}
	KernelPrintfXY(0x47 , 0 , 0 , "[An exception]");
	KernelPrintfXY(0x47 , 0 , 2 , "ErrorCode:ERR_EXCEPTION_NF");
	KernelPrintfXY(0x47 , 0 , 4 , "ChoOS Is Halted.");
	while(1);
}
