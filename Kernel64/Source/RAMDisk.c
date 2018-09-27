#include "RAMDisk.h"
#include "Utility.h"
#include "DynamicMemory.h"

static RDDMANAGER RAMDiskManager;

BOOL InitRAMDisk(DWORD TotalSectorCount) {
	MemSet(&RAMDiskManager , 0 , sizeof(RAMDiskManager));
	RAMDiskManager.Buffer = (BYTE*)AllocateMemory(TotalSectorCount*512);
	if(RAMDiskManager.Buffer == NULL) {
		return FALSE;
	}
	RAMDiskManager.TotalSectorCount = TotalSectorCount;
	InitMutex(&(RAMDiskManager.Mutex));
	return TRUE;
}

BOOL ReadRAMDiskInformation(BOOL Primary , BOOL Master , HDDINFORMATION *HDDInformation) {
	MemSet(HDDInformation , 0 , sizeof(HDDINFORMATION));
	HDDInformation->TotalSectors = RAMDiskManager.TotalSectorCount;
	MemCpy(HDDInformation->SerialNumber , "CRDD-0001" , 9);
	MemCpy(HDDInformation->ModelNumber , "ChoOS RAM Disk" , 14);
	return TRUE;
}

int ReadRAMDiskSector(BOOL Primary , BOOL Master , DWORD LBA , int SectorCount , char *Buffer) {
	int RealReadCount;
	RealReadCount = MIN(RAMDiskManager.TotalSectorCount-(LBA+SectorCount) , SectorCount);
	MemCpy(Buffer , RAMDiskManager.Buffer+(LBA*512) , RealReadCount*512);
	return RealReadCount;
}

int WriteRAMDiskSector(BOOL Primary , BOOL Master , DWORD LBA , int SectorCount , char *Buffer) {
	int RealWriteCount;
	RealWriteCount = MIN(RAMDiskManager.TotalSectorCount-(LBA+SectorCount) , SectorCount);
	MemCpy(RAMDiskManager.Buffer+(LBA*512) , Buffer , RealWriteCount*512);
	return RealWriteCount;
}
