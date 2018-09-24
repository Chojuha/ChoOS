#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "Utility.h"
#include "CacheManager.h"
#include "RAMDisk.h"

static FILESYSTEMMANAGER FileSystemManager;
static BYTE gs_vbTempBuffer[FILESYSTEM_SECTORSPERCLUSTER * 512];
static BOOL IsRAMDiskUsedB;

TReadHDDInformation gs_pfReadHDDInformation = NULL;
TReadHDDSector gs_pfReadHDDSector = NULL;
TWriteHDDSector gs_pfWriteHDDSector = NULL;

BOOL InitFileSystem(void) {
	BOOL CacheEnable;
	CacheEnable = FALSE;
    MemSet(&FileSystemManager , 0, sizeof(FileSystemManager));
    InitMutex(&(FileSystemManager.Mutex));
    if(InitHDD() == TRUE)
    {
        gs_pfReadHDDInformation = ReadHDDInformation;
        gs_pfReadHDDSector = ReadHDDSector;
        gs_pfWriteHDDSector = WriteHDDSector;
    }
    else if(InitRAMDisk(RDD_TOTALSECTORCOUNT) == TRUE) {
    	IsRAMDiskUsedB = TRUE;
        gs_pfReadHDDInformation = ReadRAMDiskInformation;
        gs_pfReadHDDSector = ReadRAMDiskSector;
        gs_pfWriteHDDSector = WriteRAMDiskSector;
        if(FormatHDD() == FALSE) {
        	return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    if(MountHDD() == FALSE)
    {
        return FALSE;
    }
    FileSystemManager.HandlePool = (FILE*) AllocateMemory(
        FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));
    if(FileSystemManager.HandlePool == NULL)
    {
        FileSystemManager.Mounted = FALSE;
        return FALSE;
    }
    MemSet(FileSystemManager.HandlePool, 0, 
            FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));    
    if(CacheEnable == TRUE) {
    	FileSystemManager.CacheEnable = InitCacheManager();
    }
    return TRUE;
}

BOOL MountHDD(void)
{
    MBR* pstMBR;
    Lock(&(FileSystemManager.Mutex));
    if(gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE) {
        UnLock(&(FileSystemManager.Mutex));
        return FALSE;
    }
    pstMBR = (MBR*) gs_vbTempBuffer;
    if(pstMBR->Signature != FILESYSTEM_SIGNATURE)
    {
        UnLock(&(FileSystemManager.Mutex));
        return FALSE;
    }
    FileSystemManager.Mounted = TRUE;
    FileSystemManager.ReservedSectorCount = pstMBR->ReservedSectorCount;
    FileSystemManager.ClusterLinkAreaStartAddress =
        pstMBR->ReservedSectorCount + 1;
    FileSystemManager.ClusterLinkAreaSize = pstMBR->ClusterLinkSectorCount;
    FileSystemManager.DataAreaStartAddress = 
        pstMBR->ReservedSectorCount + pstMBR->ClusterLinkSectorCount + 1;
    FileSystemManager.TotalClusterCount = pstMBR->TotalClusterCount;
    UnLock(&(FileSystemManager.Mutex));
    return TRUE;
}

BOOL FormatHDD(void)
{
    HDDINFORMATION* pstHDD;
    MBR* pstMBR;
    DWORD dwTotalSectorCount, dwRemainSectorCount;
    DWORD dwMaxClusterCount, dwClsuterCount;
    DWORD ClusterLinkSectorCount;
    DWORD i;
    Lock(&(FileSystemManager.Mutex));
    pstHDD = (HDDINFORMATION*) gs_vbTempBuffer;
    if(gs_pfReadHDDInformation(TRUE, TRUE, pstHDD) == FALSE)
    {
        UnLock(&(FileSystemManager.Mutex));
        return FALSE;
    }    
    dwTotalSectorCount = pstHDD->TotalSectors;
    dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORSPERCLUSTER;
    ClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;
    dwRemainSectorCount = dwTotalSectorCount - ClusterLinkSectorCount - 1;
    dwClsuterCount = dwRemainSectorCount / FILESYSTEM_SECTORSPERCLUSTER;
    ClusterLinkSectorCount = (dwClsuterCount + 127) / 128;
    if(gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE)
    {
        UnLock(&(FileSystemManager.Mutex));
        return FALSE;
    }  
    pstMBR = (MBR*) gs_vbTempBuffer;
    MemSet(pstMBR->Partition, 0, sizeof(pstMBR->Partition));
    pstMBR->Signature = FILESYSTEM_SIGNATURE;
    pstMBR->ReservedSectorCount = 0;
    pstMBR->ClusterLinkSectorCount = ClusterLinkSectorCount;
    pstMBR->TotalClusterCount = dwClsuterCount;
    if(gs_pfWriteHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE)
    {
        UnLock(&(FileSystemManager.Mutex));
        return FALSE;
    }
    MemSet(gs_vbTempBuffer, 0, 512);
    for(i = 0 ; i < (ClusterLinkSectorCount + FILESYSTEM_SECTORSPERCLUSTER);
         i++)
    {
        if(i == 0)
        {
            ((DWORD*) (gs_vbTempBuffer))[0] = FILESYSTEM_LASTCLUSTER;
        }
        else
        {
            ((DWORD*) (gs_vbTempBuffer))[0] = FILESYSTEM_FREECLUSTER;
        }
        if(gs_pfWriteHDDSector(TRUE, TRUE, i + 1, 1, gs_vbTempBuffer) == FALSE)
        {
            UnLock(&(FileSystemManager.Mutex));
            return FALSE;
        }
    }
    if(FileSystemManager.CacheEnable == TRUE) {
		DiscardAllCacheBuffer(CACHE_CLUSTERLINKTABLEAREA);
		DiscardAllCacheBuffer(CACHE_DATAAREA);
	}
    UnLock(&(FileSystemManager.Mutex));
    return TRUE;
}

BOOL GetHDDInformation(HDDINFORMATION* pstInformation)
{
    BOOL bResult;
    Lock(&(FileSystemManager.Mutex));
    bResult = gs_pfReadHDDInformation(TRUE, TRUE, pstInformation);
    UnLock(&(FileSystemManager.Mutex));
    
    return bResult;
}

BOOL IsRAMDiskUsed(void) {
	return IsRAMDiskUsedB;
}

static BOOL ReadClusterLinkTable(DWORD Offset, BYTE* Buffer) {
	if(FileSystemManager.CacheEnable == FALSE) {
		InternalReadClusterLinkTableWithoutCache(Offset , Buffer);
	}
	else {
		InternalReadClusterLinkTableWithCache(Offset , Buffer);
	}
}

static BOOL InternalReadClusterLinkTableWithoutCache(DWORD Offset , BYTE *Buffer) {
    return gs_pfReadHDDSector(TRUE, TRUE, Offset + FileSystemManager.ClusterLinkAreaStartAddress, 1, Buffer);
}

static BOOL InternalReadClusterLinkTableWithCache(DWORD Offset , BYTE *Buffer) {
	CACHEBUFFER *CacheBuffer;
	CacheBuffer = FindCacheBuffer(CACHE_CLUSTERLINKTABLEAREA , Offset);
	if(CacheBuffer != NULL) {
		MemCpy(Buffer , CacheBuffer->Buffer , 512);
		return TRUE;
	}
	if(InternalReadClusterLinkTableWithoutCache(Offset , Buffer) == FALSE) {
		return FALSE;
	}
	CacheBuffer = AllocateCacheBufferWithFlush(CACHE_CLUSTERLINKTABLEAREA);
	if(CacheBuffer == NULL) {
		return FALSE;
	}
	MemCpy(CacheBuffer->Buffer , Buffer , 512);
	CacheBuffer->Tag = Offset;
	CacheBuffer->Change = FALSE;
	return TRUE;
}

static CACHEBUFFER *AllocateCacheBufferWithFlush(int CacheTableIndex) {
	CACHEBUFFER *CacheBuffer;
	CacheBuffer = AllocateCacheBuffer(CacheTableIndex);
	if(CacheBuffer == NULL) {
		CacheBuffer = GetVictimInCacheBuffer(CacheTableIndex);
		if(CacheBuffer == NULL) {
			return NULL;
		}
		if(CacheBuffer->Change == TRUE) {
			switch(CacheTableIndex) {
				case CACHE_CLUSTERLINKTABLEAREA:
					if(InternalWriteClusterLinkTableWithoutCache(CacheBuffer->Tag , CacheBuffer->Buffer) == FALSE) {
						Printf(0x07 , "Cache Buffer Write Fail\n");
						return NULL;
					}
					break;
				case CACHE_DATAAREA:
					if(InternalWriteClusterWithoutCache(CacheBuffer->Tag , CacheBuffer->Buffer) == FALSE) {
						Printf(0x07 , "Cache Buffer Write Fail\n");
						return NULL;
					}
					break;
				default:
					Printf(0x07 , "AllocateCacheBufferWithFlush() Fail\n");
					return NULL;
					break;
			}
		}
	}
	return CacheBuffer;
}
						
static BOOL WriteClusterLinkTable(DWORD Offset, BYTE* Buffer) {
	if(FileSystemManager.CacheEnable == FALSE) {
		return InternalWriteClusterLinkTableWithoutCache(Offset , Buffer);
	}
	else {
		return InternalWriteClusterLinkTableWithCache(Offset , Buffer);
	}
}

static BOOL InternalWriteClusterLinkTableWithoutCache(DWORD Offset , BYTE *Buffer) {
	return gs_pfWriteHDDSector(TRUE , TRUE , Offset+FileSystemManager.ClusterLinkAreaStartAddress , 1 , Buffer);
}

static BOOL InternalWriteClusterLinkTableWithCache(DWORD Offset , BYTE *Buffer) {
	CACHEBUFFER *CacheBuffer;
	CacheBuffer = FindCacheBuffer(CACHE_CLUSTERLINKTABLEAREA , Offset);
	if(CacheBuffer != NULL) {
		MemCpy(CacheBuffer->Buffer , Buffer , 512);
		CacheBuffer->Change = TRUE;
		return TRUE;
	}
	CacheBuffer = AllocateCacheBufferWithFlush(CACHE_DATAAREA);
	if(CacheBuffer == NULL) {
		return FALSE;
	}
	MemCpy(CacheBuffer->Buffer , Buffer , 512);
	CacheBuffer->Tag = Offset;
	CacheBuffer->Change = TRUE;
	return TRUE;
}

static BOOL ReadCluster(DWORD Offset, BYTE* Buffer) {
	if(FileSystemManager.CacheEnable == FALSE) {
		InternalReadClusterWithoutCache(Offset , Buffer);
	}
	else {
		InternalReadClusterWithCache(Offset , Buffer);
	}
}

static BOOL InternalReadClusterWithoutCache(DWORD Offset , BYTE *Buffer) {
    return gs_pfReadHDDSector(TRUE, TRUE, (Offset * FILESYSTEM_SECTORSPERCLUSTER) + FileSystemManager.DataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, Buffer);
}

static BOOL InternalReadClusterWithCache(DWORD Offset , BYTE *Buffer) {
	CACHEBUFFER *CacheBuffer;
	CacheBuffer = FindCacheBuffer(CACHE_DATAAREA , Offset);
	if(CacheBuffer != NULL) {
		MemCpy(Buffer , CacheBuffer->Buffer , FILESYSTEM_CLUSTERSIZE);
		CacheBuffer->Change = TRUE;
		return TRUE;
	}
	if(InternalReadClusterWithoutCache(Offset , Buffer) == FALSE) {
		return FALSE;
	}
	CacheBuffer = AllocateCacheBufferWithFlush(CACHE_DATAAREA);
	if(CacheBuffer == NULL) {
		return FALSE;
	}
	MemCpy(CacheBuffer->Buffer , Buffer , FILESYSTEM_CLUSTERSIZE);
	CacheBuffer->Tag = Offset;
	CacheBuffer->Change = TRUE;
	return TRUE;
}

static BOOL WriteCluster(DWORD Offset, BYTE* Buffer) {
	if(FileSystemManager.CacheEnable == FALSE) {
		InternalWriteClusterWithoutCache(Offset , Buffer);
	}
	else {
		InternalWriteClusterWithCache(Offset , Buffer);
	}
}

static BOOL InternalWriteClusterWithoutCache(DWORD Offset , BYTE *Buffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE, (Offset * FILESYSTEM_SECTORSPERCLUSTER) + FileSystemManager.DataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, Buffer);
}

static BOOL InternalWriteClusterWithCache(DWORD Offset , BYTE *Buffer) {
	CACHEBUFFER *CacheBuffer;
	CacheBuffer = FindCacheBuffer(CACHE_DATAAREA , Offset);
	if(CacheBuffer != NULL) {
		MemCpy(CacheBuffer->Buffer , Buffer , FILESYSTEM_CLUSTERSIZE);
		CacheBuffer->Change = TRUE;
		return TRUE;
	}
	CacheBuffer = AllocateCacheBufferWithFlush(CACHE_DATAAREA);
	if(CacheBuffer == NULL) {
		return FALSE;
	}
	MemCpy(CacheBuffer->Buffer , Buffer , FILESYSTEM_CLUSTERSIZE);
	CacheBuffer->Tag = Offset;
	CacheBuffer->Change = TRUE;
	return TRUE;
}

static DWORD FindFreeCluster(void) {
    DWORD dwLinkCountInSector;
    DWORD dwLastSectorOffset, dwCurrentSectorOffset;
    DWORD i, j;
    if(FileSystemManager.Mounted == FALSE)
    {
        return FILESYSTEM_LASTCLUSTER;
    }
    dwLastSectorOffset = FileSystemManager.LastAllocatedClusterLinkSectorOffset;
    for(i = 0 ; i < FileSystemManager.ClusterLinkAreaSize ; i++)
    {
        if((dwLastSectorOffset + i) == 
            (FileSystemManager.ClusterLinkAreaSize - 1))
        {
            dwLinkCountInSector = FileSystemManager.TotalClusterCount % 128; 
        }
        else
        {
            dwLinkCountInSector = 128;
        }
        dwCurrentSectorOffset = (dwLastSectorOffset + i) % 
            FileSystemManager.ClusterLinkAreaSize;
        if(ReadClusterLinkTable(dwCurrentSectorOffset, gs_vbTempBuffer) == FALSE)
        {
            return FILESYSTEM_LASTCLUSTER;
        }
        for(j = 0 ; j < dwLinkCountInSector ; j++)
        {
            if(((DWORD*) gs_vbTempBuffer)[j] == FILESYSTEM_FREECLUSTER)
            {
                break;
            }
        }
        if(j != dwLinkCountInSector)
        {
            FileSystemManager.LastAllocatedClusterLinkSectorOffset = 
                dwCurrentSectorOffset;
            return (dwCurrentSectorOffset * 128) + j;
        }
    }
    
    return FILESYSTEM_LASTCLUSTER;
}

static BOOL SetClusterLinkData(DWORD dwClusterIndex, DWORD dwData)
{
    DWORD dwSectorOffset;
    if(FileSystemManager.Mounted == FALSE)
    {
        return FALSE;
    }
    dwSectorOffset = dwClusterIndex / 128;
    if(ReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE)
    {
        return FALSE;
    }    
    ((DWORD*) gs_vbTempBuffer)[dwClusterIndex % 128] = dwData;
    if(WriteClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE)
    {
        return FALSE;
    }
    return TRUE;
}

static BOOL GetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData)
{
    DWORD dwSectorOffset;
    if(FileSystemManager.Mounted == FALSE)
    {
        return FALSE;
    }
    dwSectorOffset = dwClusterIndex / 128;
    if(dwSectorOffset > FileSystemManager.ClusterLinkAreaSize)
    {
        return FALSE;
    }
    if(ReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE)
    {
        return FALSE;
    }
    *pdwData = ((DWORD*) gs_vbTempBuffer)[dwClusterIndex % 128];
    return TRUE;
}

static int FindFreeDirectoryEntry(void)
{
    DIRECTORYENTRY* pstEntry;
    int i;
    if(FileSystemManager.Mounted == FALSE)
    {
        return -1;
    }
    if(ReadCluster(0, gs_vbTempBuffer) == FALSE)
    {
        return -1;
    }
    pstEntry = (DIRECTORYENTRY*) gs_vbTempBuffer;
    for(i = 0 ; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT ; i++)
    {
        if(pstEntry[i].StartClusterIndex == 0)
        {
            return i;
        }
    }
    return -1;
}

static BOOL SetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry)
{
    DIRECTORYENTRY* pstRootEntry;
    if((FileSystemManager.Mounted == FALSE) ||
        (iIndex < 0)||(iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT))
    {
        return FALSE;
    }
    if(ReadCluster(0, gs_vbTempBuffer) == FALSE)
    {
        return FALSE;
    }
    pstRootEntry = (DIRECTORYENTRY*) gs_vbTempBuffer;
    MemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));
    if(WriteCluster(0, gs_vbTempBuffer) == FALSE)
    {
        return FALSE;
    }    
    return TRUE;
}

static BOOL GetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry)
{
    DIRECTORYENTRY* pstRootEntry;
    if((FileSystemManager.Mounted == FALSE) ||
        (iIndex < 0)||(iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT))
    {
        return FALSE;
    }
    if(ReadCluster(0, gs_vbTempBuffer) == FALSE)
    {
        return FALSE;
    } 
    pstRootEntry = (DIRECTORYENTRY*) gs_vbTempBuffer;
    MemCpy(pstEntry, pstRootEntry + iIndex, sizeof(DIRECTORYENTRY));
    return TRUE;
}

static int FindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry)
{
    DIRECTORYENTRY* pstRootEntry;
    int i;
    int iLength;
    if(FileSystemManager.Mounted == FALSE)
    {
        return -1;
    }
    if(ReadCluster(0, gs_vbTempBuffer) == FALSE)
    {
        return -1;
    }
    
    iLength = StrLen(pcFileName);
    pstRootEntry = (DIRECTORYENTRY*) gs_vbTempBuffer;
    for(i = 0 ; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT ; i++)
    {
        if(MemCmp(pstRootEntry[i].FileName, pcFileName, iLength) == 0)
        {
            MemCpy(pstEntry, pstRootEntry + i, sizeof(DIRECTORYENTRY));
            return i;
        }
    }
    return -1;
}

void GetFileSystemInformation(FILESYSTEMMANAGER* pstManager)
{
    MemCpy(pstManager, &FileSystemManager, sizeof(FileSystemManager));
}

static void* AllocateFileDirectoryHandle(void)
{
    int i;
    FILE* pstFile;
    pstFile = FileSystemManager.HandlePool;
    for(i = 0 ; i < FILESYSTEM_HANDLE_MAXCOUNT ; i++)
    {
        if(pstFile->Type == FILESYSTEM_TYPE_FREE)
        {
            pstFile->Type = FILESYSTEM_TYPE_FILE;
            return pstFile;
        }
        pstFile++;
    }
    
    return NULL;
}

static void FreeFileDirectoryHandle(FILE* pstFile)
{
    MemSet(pstFile, 0, sizeof(FILE));
    pstFile->Type = FILESYSTEM_TYPE_FREE;
}

static BOOL CreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry, 
        int* piDirectoryEntryIndex)
{
    DWORD dwCluster;
    dwCluster = FindFreeCluster();
    if((dwCluster == FILESYSTEM_LASTCLUSTER) ||
        (SetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE))
    {
        return FALSE;
    }
    *piDirectoryEntryIndex = FindFreeDirectoryEntry();
    if(*piDirectoryEntryIndex == -1)
    {
        SetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
        return FALSE;
    }
    MemCpy(pstEntry->FileName, pcFileName, StrLen(pcFileName) + 1);
    pstEntry->StartClusterIndex = dwCluster;
    pstEntry->FileSize = 0;
    if(SetDirectoryEntryData(*piDirectoryEntryIndex, pstEntry) == FALSE)
    {
        SetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
        return FALSE;
    }
    return TRUE;
}

static BOOL FreeClusterUntilEnd(DWORD dwClusterIndex)
{
    DWORD CurrentClusterIndex;
    DWORD dwNextClusterIndex;
    CurrentClusterIndex = dwClusterIndex;
    while(CurrentClusterIndex != FILESYSTEM_LASTCLUSTER) {
        if(GetClusterLinkData(CurrentClusterIndex, &dwNextClusterIndex)
                == FALSE)
        {
            return FALSE;
        }
        if(SetClusterLinkData(CurrentClusterIndex, FILESYSTEM_FREECLUSTER)
                == FALSE)
        {
            return FALSE;
        }
        CurrentClusterIndex = dwNextClusterIndex;
    }
    return TRUE;
}

FILE* OpenFile(const char* pcFileName, const char* pcMode)
{
    DIRECTORYENTRY stEntry;
    int DirectoryEntryOffset;
    int iFileNameLength;
    DWORD dwSecondCluster;
    FILE* pstFile;
    iFileNameLength = StrLen(pcFileName);
    if((iFileNameLength > (sizeof(stEntry.FileName) - 1))||
        (iFileNameLength == 0))
    {
        return NULL;
    }
    Lock(&(FileSystemManager.Mutex));
    DirectoryEntryOffset = FindDirectoryEntry(pcFileName, &stEntry);
    if(DirectoryEntryOffset == -1)
    {
        if(pcMode[0] == 'r')
        {
            UnLock(&(FileSystemManager.Mutex));
            return NULL;
        }
        if(CreateFile(pcFileName, &stEntry, &DirectoryEntryOffset) == FALSE)
        {
            UnLock(&(FileSystemManager.Mutex));
            return NULL;
        }
    } 
    else if(pcMode[0] == 'w')
    {
        if(GetClusterLinkData(stEntry.StartClusterIndex, &dwSecondCluster)
                == FALSE)
        {
            UnLock(&(FileSystemManager.Mutex));
            return NULL;
        }
        if(SetClusterLinkData(stEntry.StartClusterIndex, 
               FILESYSTEM_LASTCLUSTER) == FALSE)
        {
            UnLock(&(FileSystemManager.Mutex));
            return NULL;
        }
        if(FreeClusterUntilEnd(dwSecondCluster) == FALSE)
        {
            UnLock(&(FileSystemManager.Mutex));
            return NULL;
        }
        stEntry.FileSize = 0;
        if(SetDirectoryEntryData(DirectoryEntryOffset, &stEntry) == FALSE)
        {
            UnLock(&(FileSystemManager.Mutex));
            return NULL;
        }
    }
    pstFile = AllocateFileDirectoryHandle();
    if(pstFile == NULL)
    {
        UnLock(&(FileSystemManager.Mutex));
        return NULL;
    }
    pstFile->Type = FILESYSTEM_TYPE_FILE;
    pstFile->FileHandle.DirectoryEntryOffset = DirectoryEntryOffset;
    pstFile->FileHandle.FileSize = stEntry.FileSize;
    pstFile->FileHandle.StartClusterIndex = stEntry.StartClusterIndex;
    pstFile->FileHandle.CurrentClusterIndex = stEntry.StartClusterIndex;
    pstFile->FileHandle.PrevClusterIndex = stEntry.StartClusterIndex;
    pstFile->FileHandle.CurrentOffset = 0;
    if(pcMode[0] == 'a')
    {
        SeekFile(pstFile, 0, FILESYSTEM_SEEK_END);
    }
    UnLock(&(FileSystemManager.Mutex));
    return pstFile;
}

DWORD ReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile)
{
    DWORD dwTotalCount;
    DWORD dwReadCount;
    DWORD OffsetInCluster;
    DWORD dwCopySize;
    FILEHANDLE* pFileHandle;
    DWORD dwNextClusterIndex;
    if((pstFile == NULL) ||
        (pstFile->Type != FILESYSTEM_TYPE_FILE))
    {
        return 0;
    }
    pFileHandle = &(pstFile->FileHandle);
    if((pFileHandle->CurrentOffset == pFileHandle->FileSize) ||
        (pFileHandle->CurrentClusterIndex == FILESYSTEM_LASTCLUSTER))
    {
        return 0;
    }
    dwTotalCount = MIN(dwSize * dwCount, pFileHandle->FileSize - 
                        pFileHandle->CurrentOffset);
    Lock(&(FileSystemManager.Mutex));
    dwReadCount = 0;
    while(dwReadCount != dwTotalCount)
    {
        if(ReadCluster(pFileHandle->CurrentClusterIndex, gs_vbTempBuffer)
                == FALSE)
        {
            break;
        }
        OffsetInCluster = pFileHandle->CurrentOffset % FILESYSTEM_CLUSTERSIZE;
        dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - OffsetInCluster, 
                          dwTotalCount - dwReadCount);
        MemCpy((char*) pvBuffer + dwReadCount, 
                gs_vbTempBuffer + OffsetInCluster, dwCopySize);
        dwReadCount += dwCopySize;
        pFileHandle->CurrentOffset += dwCopySize;
        if((pFileHandle->CurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0)
        {
            if(GetClusterLinkData(pFileHandle->CurrentClusterIndex, 
                                     &dwNextClusterIndex) == FALSE)
            {
                break;
            }
            pFileHandle->PrevClusterIndex = 
                pFileHandle->CurrentClusterIndex;
            pFileHandle->CurrentClusterIndex = dwNextClusterIndex;
        }
    }
    UnLock(&(FileSystemManager.Mutex));
    return (dwReadCount / dwSize);
}

static BOOL UpdateDirectoryEntry(FILEHANDLE* pFileHandle)
{
    DIRECTORYENTRY stEntry;
    if((pFileHandle == NULL) ||
        (GetDirectoryEntryData(pFileHandle->DirectoryEntryOffset, &stEntry)
            == FALSE))
    {
        return FALSE;
    }
    stEntry.FileSize = pFileHandle->FileSize;
    stEntry.StartClusterIndex = pFileHandle->StartClusterIndex;
    if(SetDirectoryEntryData(pFileHandle->DirectoryEntryOffset, &stEntry)
            == FALSE)
    {
        return FALSE;
    }
    return TRUE;
}

DWORD WriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile)
{
    DWORD dwWriteCount;
    DWORD dwTotalCount;
    DWORD OffsetInCluster;
    DWORD dwCopySize;
    DWORD dwAllocatedClusterIndex;
    DWORD dwNextClusterIndex;
    FILEHANDLE* pFileHandle;
    if((pstFile == NULL) ||
        (pstFile->Type != FILESYSTEM_TYPE_FILE))
    {
        return 0;
    }
    pFileHandle = &(pstFile->FileHandle);
    dwTotalCount = dwSize * dwCount;
    Lock(&(FileSystemManager.Mutex));
    dwWriteCount = 0;
    while(dwWriteCount != dwTotalCount)
    {
        if(pFileHandle->CurrentClusterIndex == FILESYSTEM_LASTCLUSTER)
        {
            dwAllocatedClusterIndex = FindFreeCluster();
            if(dwAllocatedClusterIndex == FILESYSTEM_LASTCLUSTER)
            {
                break;
            }
            if(SetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_LASTCLUSTER)
                    == FALSE)
            {
                break;
            }
            if(SetClusterLinkData(pFileHandle->PrevClusterIndex, 
                                     dwAllocatedClusterIndex) == FALSE)
            {
                SetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_FREECLUSTER);
                break;
            }
            pFileHandle->CurrentClusterIndex = dwAllocatedClusterIndex;
            MemSet(gs_vbTempBuffer, 0, FILESYSTEM_LASTCLUSTER);
        }
        else if(((pFileHandle->CurrentOffset % FILESYSTEM_CLUSTERSIZE) != 0) ||
                 ((dwTotalCount - dwWriteCount) < FILESYSTEM_CLUSTERSIZE))
        {
            if(ReadCluster(pFileHandle->CurrentClusterIndex, 
                              gs_vbTempBuffer) == FALSE)
            {
                break;
            }
        }
        OffsetInCluster = pFileHandle->CurrentOffset % FILESYSTEM_CLUSTERSIZE;
        dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - OffsetInCluster, 
                          dwTotalCount - dwWriteCount);
        MemCpy(gs_vbTempBuffer + OffsetInCluster, (char*) pvBuffer + 
                 dwWriteCount, dwCopySize);
        if(WriteCluster(pFileHandle->CurrentClusterIndex, gs_vbTempBuffer) 
                == FALSE)
        {
            break;
        }
        dwWriteCount += dwCopySize;
        pFileHandle->CurrentOffset += dwCopySize;
		if((pFileHandle->CurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0)
        {
            if(GetClusterLinkData(pFileHandle->CurrentClusterIndex, 
                                     &dwNextClusterIndex) == FALSE)
            {
                break;
            }
            pFileHandle->PrevClusterIndex = 
                pFileHandle->CurrentClusterIndex;
            pFileHandle->CurrentClusterIndex = dwNextClusterIndex;
        }
    }
	if(pFileHandle->FileSize < pFileHandle->CurrentOffset)
    {
        pFileHandle->FileSize = pFileHandle->CurrentOffset;
        UpdateDirectoryEntry(pFileHandle);
    }
    UnLock(&(FileSystemManager.Mutex));
    return (dwWriteCount / dwSize);
}

BOOL WriteZero(FILE* pstFile, DWORD dwCount)
{
    BYTE* Buffer;
    DWORD dwRemainCount;
    DWORD dwWriteCount;
    if(pstFile == NULL)
    {
        return FALSE;
    }
    Buffer = (BYTE*) AllocateMemory(FILESYSTEM_CLUSTERSIZE);
    if(Buffer == NULL)
    {
        return FALSE;
    }
    MemSet(Buffer, 0, FILESYSTEM_CLUSTERSIZE);
    dwRemainCount = dwCount;
    while(dwRemainCount != 0)
    {
        dwWriteCount = MIN(dwRemainCount , FILESYSTEM_CLUSTERSIZE);
        if(WriteFile(Buffer, 1, dwWriteCount, pstFile) != dwWriteCount)
        {
            FreeMemory(Buffer);
            return FALSE;
        }
        dwRemainCount -= dwWriteCount;
    }
    FreeMemory(Buffer);
    return TRUE;
}
int SeekFile(FILE* pstFile, int iOffset, int iOrigin)
{
    DWORD dwRealOffset;
    DWORD dwClusterOffsetToMove;
    DWORD dwCurrentClusterOffset;
    DWORD dwLastClusterOffset;
    DWORD dwMoveCount;
    DWORD i;
    DWORD StartClusterIndex;
    DWORD PrevClusterIndex;
    DWORD CurrentClusterIndex;
    FILEHANDLE* pFileHandle;
    if((pstFile == NULL) ||
        (pstFile->Type != FILESYSTEM_TYPE_FILE))
    {
        return 0;
    }
    pFileHandle = &(pstFile->FileHandle);
    switch(iOrigin)
    {
    case FILESYSTEM_SEEK_SET:
        if(iOffset <= 0)
        {
            dwRealOffset = 0;
        }
        else
        {
            dwRealOffset = iOffset;
        }
        break;
    case FILESYSTEM_SEEK_CUR:
        if((iOffset < 0) && 
            (pFileHandle->CurrentOffset <= (DWORD) -iOffset))
        {
            dwRealOffset = 0;
        }
        else
        {
            dwRealOffset = pFileHandle->CurrentOffset + iOffset;
        }
        break;
    case FILESYSTEM_SEEK_END:
        if((iOffset < 0) && 
            (pFileHandle->FileSize <= (DWORD) -iOffset))
        {
            dwRealOffset = 0;
        }
        else
        {
            dwRealOffset = pFileHandle->FileSize + iOffset;
        }
        break;
    }
    dwLastClusterOffset = pFileHandle->FileSize / FILESYSTEM_CLUSTERSIZE;
    dwClusterOffsetToMove = dwRealOffset / FILESYSTEM_CLUSTERSIZE;
    dwCurrentClusterOffset = pFileHandle->CurrentOffset / FILESYSTEM_CLUSTERSIZE;
    if(dwLastClusterOffset < dwClusterOffsetToMove)
    {
        dwMoveCount = dwLastClusterOffset - dwCurrentClusterOffset;
        StartClusterIndex = pFileHandle->CurrentClusterIndex;
    }
    else if(dwCurrentClusterOffset <= dwClusterOffsetToMove)
    {
        dwMoveCount = dwClusterOffsetToMove - dwCurrentClusterOffset;
        StartClusterIndex = pFileHandle->CurrentClusterIndex;
    }
    else
    {
        dwMoveCount = dwClusterOffsetToMove;
        StartClusterIndex = pFileHandle->StartClusterIndex;
    }
    Lock(&(FileSystemManager.Mutex));
    CurrentClusterIndex = StartClusterIndex;
    for(i = 0 ; i < dwMoveCount ; i++)
    {
        PrevClusterIndex = CurrentClusterIndex;
        if(GetClusterLinkData(PrevClusterIndex, &CurrentClusterIndex) ==
            FALSE)
        {
            UnLock(&(FileSystemManager.Mutex));
            return -1;
        }
    }
    if(dwMoveCount > 0)
    {
        pFileHandle->PrevClusterIndex = PrevClusterIndex;
        pFileHandle->CurrentClusterIndex = CurrentClusterIndex;
    }
	else if(StartClusterIndex == pFileHandle->StartClusterIndex)
    {
        pFileHandle->PrevClusterIndex = pFileHandle->StartClusterIndex;
        pFileHandle->CurrentClusterIndex = pFileHandle->StartClusterIndex;
    }
    if(dwLastClusterOffset < dwClusterOffsetToMove)
    {
        pFileHandle->CurrentOffset = pFileHandle->FileSize;
        UnLock(&(FileSystemManager.Mutex));
        if(WriteZero(pstFile, dwRealOffset - pFileHandle->FileSize)
                == FALSE)
        {
            return 0;
        }
    }

    pFileHandle->CurrentOffset = dwRealOffset;
    UnLock(&(FileSystemManager.Mutex));

    return 0;    
}

int CloseFile(FILE* pstFile) {
    if((pstFile == NULL) ||
        (pstFile->Type != FILESYSTEM_TYPE_FILE))
    {
        return -1;
    }
    FreeFileDirectoryHandle(pstFile);
    return 0;
}

BOOL IsFileOpened(const DIRECTORYENTRY* pstEntry)
{
    int i;
    FILE* pstFile;
    pstFile = FileSystemManager.HandlePool;
    for(i = 0 ; i < FILESYSTEM_HANDLE_MAXCOUNT ; i++)
    {
        if((pstFile[i].Type == FILESYSTEM_TYPE_FILE) &&
            (pstFile[i].FileHandle.StartClusterIndex == 
              pstEntry->StartClusterIndex))
        {
            return TRUE;
        }
    }
    return FALSE;
}

int RemoveFile(const char* pcFileName)
{
    DIRECTORYENTRY stEntry;
    int DirectoryEntryOffset;
    int iFileNameLength;
    iFileNameLength = StrLen(pcFileName);
    if((iFileNameLength > (sizeof(stEntry.FileName) - 1))||
        (iFileNameLength == 0))
    {
        return NULL;
    }
    Lock(&(FileSystemManager.Mutex));
    DirectoryEntryOffset = FindDirectoryEntry(pcFileName, &stEntry);
    if(DirectoryEntryOffset == -1) 
    { 
        UnLock(&(FileSystemManager.Mutex));
        return -1;
    }
    if(IsFileOpened(&stEntry) == TRUE)
    {
        UnLock(&(FileSystemManager.Mutex));
        return -1;
    }
    if(FreeClusterUntilEnd(stEntry.StartClusterIndex) == FALSE)
    { 
        UnLock(&(FileSystemManager.Mutex));
        return -1;
    }
    MemSet(&stEntry, 0, sizeof(stEntry));
    if(SetDirectoryEntryData(DirectoryEntryOffset, &stEntry) == FALSE)
    {
        UnLock(&(FileSystemManager.Mutex));
        return -1;
    }
    UnLock(&(FileSystemManager.Mutex));
    return 0;
}

DIR* OpenDirectory(const char* pcDirectoryName)
{
    DIR* pstDirectory;
    DIRECTORYENTRY* DirectoryBuffer;
    Lock(&(FileSystemManager.Mutex));pstDirectory = AllocateFileDirectoryHandle();
    if(pstDirectory == NULL)
    {
        UnLock(&(FileSystemManager.Mutex));
        return NULL;
    }
    DirectoryBuffer = (DIRECTORYENTRY*) AllocateMemory(FILESYSTEM_CLUSTERSIZE);
    if(pstDirectory == NULL)
    {
        FreeFileDirectoryHandle(pstDirectory);
        UnLock(&(FileSystemManager.Mutex));
        return NULL;
    }
    if(ReadCluster(0 , (BYTE*) DirectoryBuffer) == FALSE)
    {
        FreeFileDirectoryHandle(pstDirectory);
        FreeMemory(DirectoryBuffer);
        UnLock(&(FileSystemManager.Mutex));
        return NULL;
        
    }
    pstDirectory->Type = FILESYSTEM_TYPE_DIRECTORY;
    pstDirectory->DirectoryHandle.CurrentOffset = 0;
    pstDirectory->DirectoryHandle.DirectoryBuffer = DirectoryBuffer;
    UnLock(&(FileSystemManager.Mutex));
    return pstDirectory;
}

struct DirectoryEntryStruct* ReadDirectory(DIR* pstDirectory)
{
    DIRECTORYHANDLE* pDirectoryHandle;
    DIRECTORYENTRY* pstEntry;
    if((pstDirectory == NULL) ||
        (pstDirectory->Type != FILESYSTEM_TYPE_DIRECTORY))
    {
        return NULL;
    }
    pDirectoryHandle = &(pstDirectory->DirectoryHandle);
    if((pDirectoryHandle->CurrentOffset < 0) ||
        (pDirectoryHandle->CurrentOffset >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT))
    {
        return NULL;
    }
    Lock(&(FileSystemManager.Mutex));
    pstEntry = pDirectoryHandle->DirectoryBuffer;
    while(pDirectoryHandle->CurrentOffset < FILESYSTEM_MAXDIRECTORYENTRYCOUNT)
    {
        if(pstEntry[pDirectoryHandle->CurrentOffset].StartClusterIndex
                != 0)
        {
            UnLock(&(FileSystemManager.Mutex));
            return &(pstEntry[pDirectoryHandle->CurrentOffset++]);
        }
        
        pDirectoryHandle->CurrentOffset++;
    }
    UnLock(&(FileSystemManager.Mutex));
    return NULL;
}


void RewindDirectory(DIR* pstDirectory)
{
    DIRECTORYHANDLE* pDirectoryHandle;
    if((pstDirectory == NULL) ||
        (pstDirectory->Type != FILESYSTEM_TYPE_DIRECTORY))
    {
        return ;
    }
    pDirectoryHandle = &(pstDirectory->DirectoryHandle);
    Lock(&(FileSystemManager.Mutex));
    pDirectoryHandle->CurrentOffset = 0;
    UnLock(&(FileSystemManager.Mutex));
}

int CloseDirectory(DIR* pstDirectory)
{
    DIRECTORYHANDLE* pDirectoryHandle;
    if((pstDirectory == NULL) ||
        (pstDirectory->Type != FILESYSTEM_TYPE_DIRECTORY))
    {
        return -1;
    }
    pDirectoryHandle = &(pstDirectory->DirectoryHandle);
    Lock(&(FileSystemManager.Mutex));
    FreeMemory(pDirectoryHandle->DirectoryBuffer);
    FreeFileDirectoryHandle(pstDirectory);
    UnLock(&(FileSystemManager.Mutex));

    return 0;
}

BOOL FlushFileSystemCache(void) {
	CACHEBUFFER *CacheBuffer;
	int CacheCount;
	int i;
	if(FileSystemManager.CacheEnable == FALSE) {
		return TRUE;
	}
	Lock(&(FileSystemManager.Mutex));
	GetCacheBufferAndCount(CACHE_CLUSTERLINKTABLEAREA , &CacheBuffer , &CacheCount);
	for(i = 0; i < CacheCount; i++) {
		if(CacheBuffer[i].Change == TRUE) {
			if(InternalWriteClusterLinkTableWithoutCache(CacheBuffer[i].Tag , CacheBuffer[i].Buffer) == FALSE) {
				return FALSE;
			}
			CacheBuffer[i].Change = FALSE;
		}
	}
	GetCacheBufferAndCount(CACHE_DATAAREA , &CacheBuffer , &CacheCount);
	for(i = 0; i < CacheCount; i++) {
		if(CacheBuffer[i].Change == TRUE) {
			if(InternalWriteClusterWithoutCache(CacheBuffer[i].Tag , CacheBuffer[i].Buffer) == FALSE) {
				return FALSE;
			}
			CacheBuffer[i].Change = FALSE;
		}
	}
	UnLock(&(FileSystemManager.Mutex));
	return TRUE;
}
