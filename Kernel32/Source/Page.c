#include "Page.h"

void InitPageTables(void) {
	PML4TENTRY* PML4TEntry;
	PDPTENTRY* PDPTEntry;
	PDENTRY* PDEntry;
	DWORD MappingAddress;
	int i;
	PML4TEntry = (PML4TENTRY* )0x100000;
	SetPageEntryData(&(PML4TEntry[0]) , 0x00 , 0x101000 , PAGE_FLAGS_DEFAULT , 0);
	for(i = 1; i < PAGE_MAXENTRYCOUNT; i++) {
		SetPageEntryData(&(PML4TEntry[i]) , 0 , 0 , 0 , 0);
	}
	PDPTEntry = (PDPTENTRY* )0x101000;
	for(i = 0; i < 64; i++) {
		SetPageEntryData(&(PDPTEntry[i]) , 0 , 0x102000+(i * PAGE_TABLESIZE) , PAGE_FLAGS_DEFAULT , 0);
	}
	for(i = 64; i < PAGE_MAXENTRYCOUNT; i++) {
		SetPageEntryData(&(PDPTEntry[i]) , 0 , 0 , 0 , 0);
	}
	PDEntry = (PDENTRY* )0x102000;
	MappingAddress = 0;
	for(i = 0; i < PAGE_MAXENTRYCOUNT * 64; i++) {
		SetPageEntryData(&(PDEntry[i]) , (i * (PAGE_DEFAULTSIZE >> 20)) >> 12 , MappingAddress , PAGE_FLAGS_DEFAULT|PAGE_FLAGS_PS , 0);
		MappingAddress += PAGE_DEFAULTSIZE;
	}
}

void SetPageEntryData(PTENTRY* PTEntry , DWORD UpperBaseAddress , DWORD LowerBaseAddress , DWORD LowerFlags , DWORD UpperFlags) {
	PTEntry->AttributeAndLowerBaseAddress = LowerBaseAddress|LowerFlags;
	PTEntry->UpperBaseAddressAndEXB = (UpperBaseAddress & 0xFF)|UpperFlags;
}
