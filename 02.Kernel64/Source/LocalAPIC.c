#include "LocalAPIC.h"
#include "MPConfigurationTable.h"

QWORD GetLocalAPICBaseAddress(void) {
	MPCONFIGURATIONTABLEHEADER *MPHeader;
	MPHeader = GetMPConfigurationManager()->MPConfigurationTableHeader;
	return MPHeader->MemoryMapIOAddressOfLocalAPIC;
}

void EnableSoftwareLocalAPIC(void) {
	QWORD LocalAPICBaseAddress;
	LocalAPICBaseAddress = GetLocalAPICBaseAddress();
	*(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_SVR) |= 0x100;
}

void SendEOIToLocalAPIC(void) {
	QWORD LocalAPICBaseAddress;
	LocalAPICBaseAddress = GetLocalAPICBaseAddress();
	*(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_EOI) = 0;
}

void SetTaskPriority(BYTE Priority) {
	QWORD LocalAPICBaseAddress;
	LocalAPICBaseAddress = GetLocalAPICBaseAddress();
	*(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_TASKPRIORITY) = Priority;
}

void InitLocalVectorTable(void) {
	QWORD LocalAPICBaseAddress;
	DWORD TempVal;
	LocalAPICBaseAddress = GetLocalAPICBaseAddress();
	*(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_TIMER) |= APIC_INTERRUPT_MASK;
    *(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_LINT0) |= APIC_INTERRUPT_MASK;
    *(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_LINT1) = APIC_TRIGGERMODE_EDGE|APIC_POLARITY_ACTIVEHIGH|APIC_DELIVERYMODE_NMI;
    *(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_ERROR) |= APIC_INTERRUPT_MASK;
    *(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_PERFORMANCEMONITORINGCOUNTER) |= APIC_INTERRUPT_MASK;
    *(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_THERMALSENSOR) |= APIC_INTERRUPT_MASK;
}
