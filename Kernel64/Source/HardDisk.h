#ifndef _HARDDISK_H_
#define _HARDDISK_H_

#include "Types.h"
#include "Synchronization.h"
#include "Colors.h"

#define HDD_PORT_PRIMARYBASE 0x1F0
#define HDD_PORT_SECONDARYBASE 0x170
#define HDD_PORT_INDEX_DATA 0x00
#define HDD_PORT_INDEX_SECTORCOUNT 0x02
#define HDD_PORT_INDEX_SECTORNUMBER 0x03
#define HDD_PORT_INDEX_CYLINDERLSB 0x04
#define HDD_PORT_INDEX_CYLINDERMSB 0x05
#define HDD_PORT_INDEX_DRIVEANDHEAD 0x06
#define HDD_PORT_INDEX_STATUS 0x07
#define HDD_PORT_INDEX_COMMAND 0x07
#define HDD_PORT_INDEX_DIGITALOUTPUT 0x206
#define HDD_COMMAND_READ 0x20
#define HDD_COMMAND_WRITE 0x30
#define HDD_COMMAND_IDENTIFY 0xEC
#define HDD_STATUS_ERROR 0x01
#define HDD_STATUS_INDEX 0x02
#define HDD_STATUS_CORRECTEDDTA 0x04
#define HDD_STATUS_DATAREQUEST 0x08
#define HDD_STATUS_SEEKCOMPLETE 0x10
#define HDD_STATUS_WRHITEFAULT 0x20
#define HDD_STATUS_READY 0x40
#define HDD_STATUS_BUSY 0x80
#define HDD_DRIVEANDHEAD_LBA 0xE0
#define HDD_DRIVEANDHEAD_SLAVE 0x10
#define HDD_DIGITALOUTPUT_RESET 0x04
#define HDD_DIGITALOUTPUT_DISABLEINTERRUPT 0x01
#define HDD_WAITTIME 500
#define HDD_MAXBULKSECTORCOUNT 256

#pragma pack(push , 1)

typedef struct HDDInformationStruct {
	WORD Configuation;
	WORD NumberOfCylinder;
	WORD Reserved1;
	WORD NumberOfHead;
	WORD UnformattedBytePerTrack;
	WORD UnformattedBytesPerSector;
	WORD NumberOfSectorPerCylinder;
	WORD InterSectorGap;
	WORD BytesInPhaseLock;
	WORD NumberOfVendorUniqueStatusWord;
	WORD SerialNumber[10];
	WORD ControllerType;
	WORD BufferSize;
	WORD NumberOfECCBytes;
	WORD FirmwareRevision[4];
	WORD ModelNumber[20];
	WORD Reserved2[13];
	DWORD TotalSectors;
	WORD Reserved3[196];
}HDDINFORMATION;

#pragma pack(pop)

typedef struct HDDManagerStruct {
	BOOL HDDDetected;
	BOOL CanWrite;
	volatile BOOL PrimaryInterruptOccur;
	volatile BOOL SecondaryInterruptOccur;
	MUTEX Mutex;
	HDDINFORMATION HDDInformation;
}HDDMANAGER;

BOOL InitHDD(void);
static BYTE ReadHDDStatus(BOOL Primary);
static BOOL WaitForHDDNoBusy(BOOL Primary);
static BOOL WaitForHDDReady(BOOL Primary);
void SetHDDInterruptFlag(BOOL Primary , BOOL Flag);
static BOOL WaitForHDDInterrupt(BOOL Primary);
BOOL ReadHDDInformation(BOOL Primary , BOOL Master , HDDINFORMATION *HDDInformation);
static void SwapByteInWord(WORD *Data , int WordCount);
int ReadHDDSector(BOOL bPrimary , BOOL bMaster , DWORD dwLBA , int iSectorCount , char *pcBuffer);
int WriteHDDSector(BOOL nPrimary , BOOL nMaster , DWORD dwLBA , int iSectorCount , char *pcBuffer);

#endif
