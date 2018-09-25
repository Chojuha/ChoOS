#ifndef _MPCONFIGURATIONTABLE_H_
#define _MPCONFIGURATIONTABLE_H_

#include "Types.h"
#include "Colors.h"

#define MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE 0x00
#define MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE 0x80
#define MP_ENTRYTYPE_PROCESSOR 0
#define MP_ENTRYTYPE_BUS 1
#define MP_ENTRYTYPE_IOAPIC 2
#define MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT 3
#define MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT 4
#define MP_PROCESSOR_CPUFLAGS_ENABLE 0x01
#define MP_PROCESSOR_CPUFLAGS_BSP 0x02
#define MP_BUS_TYPESTRING_ISA "ISA"
#define MP_BUS_TYPESTRING_PCI "PCI"
#define MP_BUS_TYPESTRING_PCMISA "PCMISA"
#define MP_BUS_TYPESTRING_VESALOCALBUS "VL"
#define MP_INTERRUPTTYPE_INT 0
#define MP_INTERRUPTTYPE_NMI 1
#define MP_INTERRUPTTYPE_SMI 2
#define MP_INTERRUPTTYPE_EXTINT 3
#define MP_INTERRUPT_FLAGS_CONFORMPOLARITY 0x00
#define MP_INTERRUPT_FLAGS_ACTIVEHIGH 0x01
#define MP_INTERRUPT_FLAGS_ACTIVELOW 0x03
#define MP_INTERRUPT_FLAGS_CONFORMTRIGGER 0x00
#define MP_INTERRUPT_FLAGS_EDGETRIGGERED 0x04
#define MP_INTERRUPT_FLAGS_LEVELTRIGGERED 0x0C

#pragma pack(push , 1)

typedef struct MPFloatingPointerStruct {
	char Signature[4];
	DWORD MPConfigurationTableAddress;
	BYTE Length;
	BYTE Revision;
	BYTE CheckSum;
	BYTE MPFeatureByte[5];
}MPFLOATINGPOINTER;	

typedef struct MPConfigurationTableHeaderStruct {
	char Signature[4];
	WORD BaseTableLength;
	BYTE Revision;
	BYTE CheckSum;
	char OEMIDString[8];
	char ProductIDString[12];
	DWORD OEMTablePointerAddress;
	WORD OEMTableSize;
	WORD EntryCount;
	DWORD MemoryMapIOAddressOfLocalAPIC;
	WORD ExtendedTableLength;
	BYTE ExtendedTableCheckSum;
	BYTE Reserved;
}MPCONFIGURATIONTABLEHEADER;

typedef struct ProcessorEntryStruct {
	BYTE EntryTtpe;
	BYTE LocalAPICID;
	BYTE LocalAPICVersion;
	BYTE CPUFlags;
	BYTE CPUSignature[4];
	DWORD FeatureFlags;
	DWORD Reserved[2];
}PROCESSORENTRY;

typedef struct BusEntryStruct {
	BYTE EntryType;
	BYTE BusID;
	char BusTypeString[6];
}BUSENTRY;

typedef struct IOAPICEntryStruct {
	BYTE EntryType;
	BYTE IOAPICID;
	BYTE IOAPICVersion;
	BYTE IOAPICFlags;
	DWORD MemoryMapAddress;
}IOAPICENTRY;

typedef struct IOInterruptAssignmentEntryStruct {
	BYTE EntryType;
	BYTE InterruptType;
	WORD InterruptFlags;
	BYTE SourceBUSID;
	BYTE SourceBUSIRQ;
	BYTE DestinationIOAPICID;
	BYTE DestinationIOAPICINTIN;
}IOINTERRUPTASSIGNMENTENTRY;

typedef struct LocalInterruptEntryStruct {
	BYTE EntryType;
	BYTE InterruptType;
	WORD InterruptFlags;
	BYTE SourceBUSID;
	BYTE SourceBUSIRQ;
	BYTE DestinationIOAPICID;
	BYTE DestinationIOAPICINTIN;
}LOCALINTERRUPTASSIGNMENTENTRY;

#pragma pack(pop)

typedef struct MPConfigurationManagerStruct {
	MPFLOATINGPOINTER *MPFloatingPointer;
	MPCONFIGURATIONTABLEHEADER *MPConfigurationTableHeader;
	QWORD BaseEntryStartAddress;
	int ProcessorCount;
	BOOL UsePICMode;
	BYTE ISABusID;
}MPCONFIGURATIONMANAGER;

BOOL FindMPFloatingPointerAddress(CONSOLECOLOR Color , QWORD *Address);
BOOL AnalysisMPConfigurationTable(CONSOLECOLOR Color);
MPCONFIGURATIONMANAGER *GetMPConfigurationManager(void);
void PrintMPConfigurationTable(CONSOLECOLOR Color);
IOAPICENTRY *FindIOAPICEntryForISA(void);
int GetProcessorCount(void);

#endif
