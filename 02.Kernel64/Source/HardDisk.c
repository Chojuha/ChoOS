#include "HardDisk.h"

static HDDMANAGER HDDManager;

BOOL InitHDD(void) {
	InitMutex(&(HDDManager.Mutex));
	HDDManager.PrimaryInterruptOccur = FALSE;
	HDDManager.SecondaryInterruptOccur = FALSE;
	OutPortByte(HDD_PORT_PRIMARYBASE+HDD_PORT_INDEX_DIGITALOUTPUT , 0);
	OutPortByte(HDD_PORT_SECONDARYBASE+HDD_PORT_INDEX_DIGITALOUTPUT , 0);
	if(ReadHDDInformation(TRUE , TRUE , &(HDDManager.HDDInformation)) == FALSE) {
		HDDManager.HDDDetected = FALSE;
		HDDManager.CanWrite = FALSE;
		return FALSE;
	}
	HDDManager.HDDDetected = TRUE;
	if(MemCmp(HDDManager.HDDInformation.ModelNumber , "QEMU" , 4) == 0) {
		HDDManager.CanWrite = TRUE;
	}
	else {
		HDDManager.CanWrite = FALSE;
	}
	return TRUE;
}

static BYTE ReadHDDStatus(BOOL Primary) {
	if(Primary == TRUE) {
		return InPortByte(HDD_PORT_PRIMARYBASE+HDD_PORT_INDEX_STATUS);
	}
	return InPortByte(HDD_PORT_SECONDARYBASE+HDD_PORT_INDEX_STATUS);
}

static BOOL WaitForHDDNoBusy(BOOL Primary) {
	QWORD StartTickCount;
	BYTE Status;
	StartTickCount = GetTickCount();
	while((GetTickCount()-StartTickCount) <= HDD_WAITTIME) {
		Status = ReadHDDStatus(Primary);
		if((Status & HDD_STATUS_BUSY) != HDD_STATUS_BUSY) {
			return TRUE;
		}
		Sleep(1);
	}
	return FALSE;
}

static BOOL WaitForHDDReady(BOOL Primary) {
	QWORD StartTickCount;
	BYTE Status;
	StartTickCount = GetTickCount();
	while((GetTickCount()-StartTickCount) <= HDD_WAITTIME) {
		Status = ReadHDDStatus(Primary);
		if((Status & HDD_STATUS_READY) == HDD_STATUS_READY) {
			return TRUE;
		}
		Sleep(1);
	}
	return FALSE;
}

void SetHDDInterruptFlag(BOOL Primary , BOOL Flag) {
	if(Primary == TRUE) {
		HDDManager.PrimaryInterruptOccur = Flag;
	}
	else {
		HDDManager.SecondaryInterruptOccur = Flag;
	}
}

static BOOL WaitForHDDInterrupt(BOOL Primary) {
	QWORD TickCount;
	TickCount = GetTickCount();
	while((GetTickCount()-TickCount) <= HDD_WAITTIME) {
		if((Primary == TRUE) && (HDDManager.PrimaryInterruptOccur == TRUE)) {
			return TRUE;
		}
		else if((Primary == FALSE) && (HDDManager.SecondaryInterruptOccur == TRUE)) {
			return TRUE;
		}
	}
	return FALSE;
}

BOOL ReadHDDInformation(BOOL Primary , BOOL Master , HDDINFORMATION *HDDInformation) {
	WORD PortBase;
	QWORD LastTickCount;
	BYTE Status;
	BYTE DriveFlag;
	int i;
	WORD Temp;
	BOOL WaitResult;
	if(Primary == TRUE) {
		PortBase = HDD_PORT_PRIMARYBASE;
	}
	else {
		PortBase = HDD_PORT_SECONDARYBASE;
	}
	Lock(&(HDDManager.Mutex));
	if(WaitForHDDNoBusy(Primary) == FALSE) {
		UnLock(&(HDDManager.Mutex));
		return FALSE;
	}
	if(Master == TRUE) {
		DriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else {
		DriveFlag = HDD_DRIVEANDHEAD_LBA|HDD_DRIVEANDHEAD_SLAVE;
	}
	OutPortByte(PortBase+HDD_PORT_INDEX_DRIVEANDHEAD , DriveFlag);
	if(WaitForHDDReady(Primary) == FALSE) {
		UnLock(&(HDDManager.Mutex));
		return FALSE;
	}
	SetHDDInterruptFlag(Primary , FALSE);
	OutPortByte(PortBase+HDD_PORT_INDEX_COMMAND , HDD_COMMAND_IDENTIFY);
	WaitResult = WaitForHDDInterrupt(Primary);
	Status = ReadHDDStatus(Primary);
	if((WaitResult == FALSE)||((Status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)) {
		UnLock(&(HDDManager.Mutex));
		return FALSE;
	}
	for(i = 0; i < 512/2; i++) {
		((WORD*)HDDInformation)[i] = InPortWord(PortBase+HDD_PORT_INDEX_DATA);
	}
	SwapByteInWord(HDDInformation->ModelNumber , sizeof(HDDInformation->ModelNumber)/2);
	SwapByteInWord(HDDInformation->SerialNumber , sizeof(HDDInformation->SerialNumber)/2);
	UnLock(&(HDDManager.Mutex));
	return TRUE;
}

static void SwapByteInWord(WORD *Data , int WordCount) {
	int i;
	WORD Temp;
	for(i = 0; i < WordCount; i++) {
		Temp = Data[i];
		Data[i] = (Temp >> 8)|(Temp << 8);
	}
}

int ReadHDDSector(BOOL Primary , BOOL Master , DWORD LBA , int SectorCount , char *Buffer) {
	WORD PortBase;
	int i;
	int j;
	BYTE DriveFlag;
	BYTE Status;
	long ReadCount = 0;
	BOOL WaitResult;
	if((HDDManager.HDDDetected == FALSE)||(SectorCount <= 0)||(256 < SectorCount)||((LBA+SectorCount) >= HDDManager.HDDInformation.TotalSectors)) {
		return 0;
	}
	if(Primary == TRUE) {
		PortBase = HDD_PORT_PRIMARYBASE;
	}
	else {
		PortBase = HDD_PORT_SECONDARYBASE;
	}
	Lock(&(HDDManager.Mutex));
	if(WaitForHDDNoBusy(Primary) == FALSE) {
		UnLock(&(HDDManager.Mutex));
		return FALSE;
	}
	OutPortByte(PortBase+HDD_PORT_INDEX_SECTORCOUNT , SectorCount);
	OutPortByte(PortBase+HDD_PORT_INDEX_SECTORNUMBER , LBA);
	OutPortByte(PortBase+HDD_PORT_INDEX_CYLINDERLSB , LBA >> 8);
	OutPortByte(PortBase+HDD_PORT_INDEX_CYLINDERMSB , LBA >> 16);
	if(Master == TRUE) {
		DriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else {
		DriveFlag = HDD_DRIVEANDHEAD_LBA|HDD_DRIVEANDHEAD_SLAVE;
	}
	OutPortByte(PortBase+HDD_PORT_INDEX_DRIVEANDHEAD , DriveFlag|((LBA >> 24) & 0x0F));
	if(WaitForHDDReady(Primary) == FALSE) {
		UnLock(&(HDDManager.Mutex));
		return FALSE;
	}
	SetHDDInterruptFlag(Primary , FALSE);
	OutPortByte(PortBase+HDD_PORT_INDEX_COMMAND , HDD_COMMAND_READ);
	for(i = 0; i < SectorCount; i++) {
		Status = ReadHDDStatus(Primary);
		if((Status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
			UnLock(&(HDDManager.Mutex));
			return i;
		}
		if((Status & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
			WaitResult = WaitForHDDInterrupt(Primary);
			SetHDDInterruptFlag(Primary , FALSE);
			if(WaitResult == FALSE) {
				UnLock(&(HDDManager.Mutex));
				return FALSE;
			}
		}
		for(j = 0; j < 512/2; j++) {
			((WORD*)Buffer)[ReadCount++] = InPortWord(PortBase+HDD_PORT_INDEX_DATA);
		}
	}
	UnLock(&(HDDManager.Mutex));
	return i;
}

int WriteHDDSector(BOOL Primary , BOOL Master , DWORD LBA , int SectorCount , char *Buffer) {
	WORD PortBase;
	WORD Temp;
	int i;
	int j;
	BYTE DriveFlag;
	BYTE Status;
	long ReadCount = 0;
	BOOL WaitResult;
	if((HDDManager.CanWrite == FALSE)||(SectorCount <= 0)||(256 < SectorCount)||((LBA+SectorCount) >= HDDManager.HDDInformation.TotalSectors)) {
		return 0;
	}
	if(Primary == TRUE) {
		PortBase = HDD_PORT_PRIMARYBASE;
	}
	else {
		PortBase = HDD_PORT_SECONDARYBASE;
	}
	if(WaitForHDDNoBusy(Primary) == FALSE) {
		return FALSE;
	}
	Lock(&(HDDManager.Mutex));
	OutPortByte(PortBase+HDD_PORT_INDEX_SECTORCOUNT , SectorCount);
	OutPortByte(PortBase+HDD_PORT_INDEX_SECTORNUMBER , LBA);
	OutPortByte(PortBase+HDD_PORT_INDEX_CYLINDERLSB , LBA >> 8);
	OutPortByte(PortBase+HDD_PORT_INDEX_CYLINDERMSB , LBA >> 16);
	if(Master == TRUE) {
		DriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else {
		DriveFlag = HDD_DRIVEANDHEAD_LBA|HDD_DRIVEANDHEAD_SLAVE;
	}
	OutPortByte(PortBase+HDD_PORT_INDEX_DRIVEANDHEAD , DriveFlag|((LBA >> 24) & 0x0F));
	if(WaitForHDDReady(Primary) == FALSE) {
		UnLock(&(HDDManager.Mutex));
		return FALSE;
	}
	OutPortByte(PortBase+HDD_PORT_INDEX_COMMAND , HDD_COMMAND_WRITE);
	while(1) {
		Status = ReadHDDStatus(Primary);
		if((Status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
			UnLock(&(HDDManager.Mutex));
			return 0;
		}
		if((Status & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST) {
			break;
		}
		Sleep(1);
	}
	for(i = 0; i < SectorCount; i++) {
		SetHDDInterruptFlag(Primary , FALSE);
		for(j = 0; j < 512/2; j++) {
			OutPortWord(PortBase+HDD_PORT_INDEX_DATA , ((WORD*)Buffer)[ReadCount++]);
		}
		Status = ReadHDDStatus(Primary);
		if((Status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
			UnLock(&(HDDManager.Mutex));
			return i;
		}
		if((Status & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
			WaitResult = WaitForHDDInterrupt(Primary);
			SetHDDInterruptFlag(Primary , FALSE);
			if(WaitResult == FALSE) {
				UnLock(&(HDDManager.Mutex));
				return FALSE;
			}
		}
	}
	UnLock(&(HDDManager.Mutex));
	return i;
}
