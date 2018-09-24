#ifndef _RAMDISK_H_
#define _RAMDISK_H_

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

#define RDD_TOTALSECTORCOUNT (10*1024*1024/512)

#pragma pack(push , 1)

typedef struct RDDManagerStruct {
	BYTE *Buffer;
	DWORD TotalSectorCount;
	MUTEX Mutex;
}RDDMANAGER;

#pragma pack(pop)

BOOL InitRAMDisk(DWORD TotalSectorCount);
BOOL ReadRAMDiskInformation(BOOL Primary , BOOL Master , HDDINFORMATION *HDDInformation);
int ReadRAMDiskSector(BOOL Primary , BOOL Master , DWORD LBA , int SectorCount , char *Buffer);
int WriteRAMDiskSector(BOOL Primary , BOOL Master , DWORD LBA , int SectorCount , char *Buffer);

#endif
