#ifndef _IOAPIC_H_
#define _IOAPIC_H_

#include "Types.h"
#include "Colors.h"

#define IOAPIC_REGISTER_IOREGISTERSELECTOR              0x00
#define IOAPIC_REGISTER_IOWINDOW                        0x10
#define IOAPIC_REGISTERINDEX_IOAPICID                   0x00
#define IOAPIC_REGISTERINDEX_IOAPICVERSION              0x01
#define IOAPIC_REGISTERINDEX_IOAPICARBID                0x02
#define IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE      0x10
#define IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE     0x11
#define IOAPIC_MAXIOREDIRECTIONTABLECOUNT               24
#define IOAPIC_INTERRUPT_MASK                           0x01
#define IOAPIC_TRIGGERMODE_LEVEL                        0x80
#define IOAPIC_TRIGGERMODE_EDGE                         0x00
#define IOAPIC_REMOTEIRR_EOI                            0x40
#define IOAPIC_REMOTEIRR_ACCEPT                         0x00
#define IOAPIC_POLARITY_ACTIVELOW                       0x20
#define IOAPIC_POLARITY_ACTIVEHIGH                      0x00
#define IOAPIC_DELIFVERYSTATUS_SENDPENDING              0x10
#define IOAPIC_DELIFVERYSTATUS_IDLE                     0x00
#define IOAPIC_DESTINATIONMODE_LOGICALMODE              0x08
#define IOAPIC_DESTINATIONMODE_PHYSICALMODE             0x00
#define IOAPIC_DELIVERYMODE_FIXED                       0x00
#define IOAPIC_DELIVERYMODE_LOWESTPRIORITY              0x01
#define IOAPIC_DELIVERYMODE_SMI                         0x02
#define IOAPIC_DELIVERYMODE_NMI                         0x04
#define IOAPIC_DELIVERYMODE_INIT                        0x05
#define IOAPIC_DELIVERYMODE_EXTINT                      0x07
#define IOAPIC_MAXIRQTOINTINMAPCOUNT                    16

#pragma pack(push , 1)

typedef struct IORedirectionTableStruct {
	BYTE Vector;
	BYTE FlagsAndDeliveryMode;
	BYTE InterruptMask;
	BYTE Reserved[4];
	BYTE Destination;
}IOREDIRECTIONTABLE;

#pragma pack(pop)

typedef struct IOAPICManagerStruct {
	QWORD IOAPICBaseAddressOfISA;
	BYTE IRQToINTINMap[IOAPIC_MAXIRQTOINTINMAPCOUNT];
}IOAPICMANAGER;

QWORD GetIOAPICBaseAddressOfISA(void);
void SetIOAPICRedirectionEntry(IOREDIRECTIONTABLE *Entry , BYTE APICID , BYTE InterruptMask , BYTE FlagsAndDeliveryMode , BYTE Vector);
void ReadIOAPICRedirectionTable(int INTIN , IOREDIRECTIONTABLE *Entry);
void WriteIOAPICRedirectionTable(int INTIN , IOREDIRECTIONTABLE *Entry);
void MaskAllInterruptInIOAPIC(void);
void InitIORedirectionTable(void);
void PrintIRQToINTINMap(CONSOLECOLOR Color);

#endif
