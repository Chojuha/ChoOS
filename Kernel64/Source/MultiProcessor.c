#include "MultiProcessor.h"
#include "MPConfigurationTable.h"
#include "AssemblyUtility.h"
#include "LocalAPIC.h"
#include "PIT.h"

volatile int WakeUpApplicationProcessorCount = 0;
volatile QWORD APICIDAddress = 0;

BOOL StartUpApplicationProcessor(CONSOLECOLOR Color) {
	if(AnalysisMPConfigurationTable(Color) == FALSE) {
		return FALSE;
	}
	EnableGlobalLocalAPIC();
	EnableSoftwareLocalAPIC();
	if(WakeUpApplicationProcessor() == FALSE) {
		return FALSE;
	}
	return TRUE;
}

static BOOL WakeUpApplicationProcessor(void) {
	MPCONFIGURATIONMANAGER *MPManager;
	MPCONFIGURATIONTABLEHEADER *MPHeader;
	QWORD LocalAPICBaseAddress;
	BOOL InterruptFlag;
	int i;
	InterruptFlag = SetInterruptFlag(FALSE);
	MPManager = GetMPConfigurationManager();
	MPHeader = MPManager->MPConfigurationTableHeader;
	LocalAPICBaseAddress = MPHeader->MemoryMapIOAddressOfLocalAPIC;
	APICIDAddress = LocalAPICBaseAddress+APIC_REGISTER_APICID;
	*(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_ICR_LOWER) = 
        APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
        APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_INIT;
	WaitUsingDirectPIT(MSTOCOUNT(10));
	if(*(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING) {
		InitPIT(MSTOCOUNT(1) , TRUE);
		SetInterruptFlag(InterruptFlag);
		return FALSE;
	}
	for(i = 0; i < 2; i++) {
		*(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_ICR_LOWER) = APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF|APIC_TRIGGERMODE_EDGE|APIC_LEVEL_ASSERT|APIC_DESTINATIONMODE_PHYSICAL|APIC_DELIVERYMODE_STARTUP|0x10;
		WaitUsingDirectPIT(USTOCOUNT(200));
	}
	if(*(DWORD*)(LocalAPICBaseAddress+APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING) {
		InitPIT(MSTOCOUNT(1) , TRUE);
		SetInterruptFlag(InterruptFlag);
		return FALSE;
	}
	InitPIT(MSTOCOUNT(1) , TRUE);
	SetInterruptFlag(InterruptFlag);
	while(WakeUpApplicationProcessorCount < (MPManager->ProcessorCount-1)) {
		Sleep(50);
	}
	return TRUE;
}

BYTE GetAPICID(void) {
	MPCONFIGURATIONTABLEHEADER *MPHeader;
	QWORD LocalAPICBaseAddress;
	if(APICIDAddress == 0) {
		MPHeader = GetMPConfigurationManager()->MPConfigurationTableHeader;
		if(MPHeader == NULL) {
			return 0;
		}
		LocalAPICBaseAddress = MPHeader->MemoryMapIOAddressOfLocalAPIC;
		APICIDAddress = LocalAPICBaseAddress+APIC_REGISTER_APICID;
	}
	return *((DWORD*)APICIDAddress) >> 24;
}
