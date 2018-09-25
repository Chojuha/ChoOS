#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "CacheManager.h"

#define FILESYSTEM_SIGNATURE 0x7E38CF10
#define FILESYSTEM_SECTORSPERCLUSTER 8
#define FILESYSTEM_LASTCLUSTER 0xFFFFFFFF
#define FILESYSTEM_FREECLUSTER 0x00
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT ((FILESYSTEM_SECTORSPERCLUSTER*512)/sizeof(DIRECTORYENTRY))
#define FILESYSTEM_CLUSTERSIZE (FILESYSTEM_SECTORSPERCLUSTER*512)
#define FILESYSTEM_HANDLE_MAXCOUNT (TASK_MAXCOUNT*3)
#define FILESYSTEM_MAXFILENAMELENGTH 24
#define FILESYSTEM_TYPE_FREE 0
#define FILESYSTEM_TYPE_FILE 1
#define FILE_TYPE_SYSTEM 0
#define FILESYSTEM_TYPE_DIRECTORY 2
#define FILESYSTEM_SEEK_SET 0
#define FILESYSTEM_SEEK_CUR 1
#define FILESYSTEM_SEEK_END 2

typedef BOOL(*TReadHDDInformation)(BOOL Primary , BOOL Master , HDDINFORMATION *HDDInformation);
typedef int(*TReadHDDSector)(BOOL Primary , BOOL Master , DWORD LBA , int SectorCount , char *Buffer);
typedef int(*TWriteHDDSector)(BOOL Primary , BOOL Master , DWORD LBA , int SectorCount , char *Buffer);

#define fopen OpenFile
#define fread ReadFile
#define fwrite WriteFile
#define fseek SeekFile
#define fclose CloseFile
#define remove RemoveFile
#define opendir OpenDirectory
#define readdir ReadDirectory
#define rewinddir RewindDirectory
#define closedir CloseDirectory
#define SEEK_SET FILESYSTEM_SEEK_SET
#define SEEK_CUR FILESYSTEM_SEEK_CUR
#define SEEK_END FILESYSTEM_SEEK_END
#define size_t DWORD
#define dirent DirectoryEntryStruct
#define d_name FileName

#pragma pack(push , 1)

typedef struct PartitionStruct {
	BYTE BootableFlag;
	BYTE StartingCHSAddress[3];
	BYTE PartitionType;
	BYTE EndingCHSAddress[3];
	DWORD StartingLBAAddress;
	DWORD SizeInSector;
}PARTITION;

typedef struct MBRStruct {
	BYTE BootCode[430];
	DWORD Signature;
	DWORD ReservedSectorCount;
	DWORD ClusterLinkSectorCount;
	DWORD TotalClusterCount;
	PARTITION Partition[4];
	BYTE BootLoaderSignature[2];
}MBR;

typedef struct DirectoryEntryStruct {
	char FileName[FILESYSTEM_MAXFILENAMELENGTH];
	DWORD FileSize;
	DWORD StartClusterIndex;
}DIRECTORYENTRY;

#pragma pack(pop)

typedef struct FileHandlerStruct {
	int DirectoryEntryOffset;
	DWORD FileSize;
	DWORD StartClusterIndex;
	DWORD CurrentClusterIndex;
	DWORD PrevClusterIndex;
	DWORD CurrentOffset;
}FILEHANDLE;

typedef struct DirectoryHandleStruct {
	DIRECTORYENTRY *DirectoryBuffer;
	int CurrentOffset;
}DIRECTORYHANDLE;

typedef struct FileDirectoryHandleStruct {
	BYTE Type;
	union {
		FILEHANDLE FileHandle;
		DIRECTORYHANDLE DirectoryHandle;
	};
}FILE , DIR;

typedef struct FileSystemManagerStruct {
	BOOL Mounted;
	DWORD ReservedSectorCount;
	DWORD ClusterLinkAreaStartAddress;
	DWORD ClusterLinkAreaSize;
	DWORD DataAreaStartAddress;
	DWORD TotalClusterCount;
	DWORD LastAllocatedClusterLinkSectorOffset;
	MUTEX Mutex;
	FILE *HandlePool;
	BOOL CacheEnable;
}FILESYSTEMMANAGER;

BOOL InitFileSystem(void);
BOOL MountHDD(void);
BOOL FormatHDD(void);
BOOL GetHDDInformation(HDDINFORMATION *pstInformation);
BOOL IsRAMDiskUsed(void);
static BOOL ReadClusterLinkTable(DWORD Offset , BYTE *Buffer);
static BOOL InternalReadClusterLinkTableWithoutCache(DWORD Offset , BYTE *Buffer);
static BOOL InternalReadClusterLinkTableWithCache(DWORD Offset , BYTE *Buffer);
static CACHEBUFFER *AllocateCacheBufferWithFlush(int CacheTableIndex);
static BOOL InternalWriteClusterLinkTableWithoutCache(DWORD Offset , BYTE *Buffer);
static BOOL InternalWriteClusterLinkTableWithCache(DWORD Offset , BYTE *Buffer);
static BOOL InternalReadClusterWithoutCache(DWORD Offset , BYTE *Buffer);
static BOOL InternalReadClusterWithCache(DWORD Offset , BYTE *Buffer);
static BOOL WriteClusterLinkTable(DWORD Offset , BYTE *Buffer);
static BOOL ReadCluster(DWORD Offset , BYTE *Buffer);
static BOOL WriteCluster(DWORD Offset , BYTE *Buffer);
static BOOL InternalWriteClusterWithoutCache(DWORD Offset , BYTE *Buffer);
static BOOL InternalWriteClusterWithCache(DWORD Offset , BYTE *Buffer);
static DWORD FindFreeCluster(void);
static BOOL SetClusterLinkData(DWORD dwClusterIndex , DWORD Data);
static BOOL GetClusterLinkData(DWORD dwClusterIndex , DWORD *Data);
static int FindFreeDirectoryEntry(void);
static BOOL SetDirectoryEntryData(int iIndex , DIRECTORYENTRY *pstEntry);
static BOOL GetDirectoryEntryData(int iIndex , DIRECTORYENTRY *pstEntry);
static int FindDirectoryEntry(const char *pcFileName , DIRECTORYENTRY *pstEntry);
void GetFileSystemInformation(FILESYSTEMMANAGER *pstManager);
static void *AllocateFileDirectoryHandle(void);
static void FreeFileDirectoryHandle(FILE *pstFile);
static BOOL CreateFile(const char *pcFileName , DIRECTORYENTRY *pstEntry , int *piDirectoryEntryIndex);
static BOOL FreeClusterUntilEnd(DWORD dwClusterIndex);
FILE *OpenFile(const char *pcFileName , const char *pcMode);
DWORD ReadFile(void *pvBuffer , DWORD dwSize , DWORD dwCount , FILE *pstFile);
static BOOL UpdateDirectoryEntry(FILEHANDLE *pFileHandle);
DWORD WriteFile(const void *pvBuffer , DWORD dwSize , DWORD dwCount , FILE *pstFile);
BOOL WriteZero(FILE *pstFile , DWORD dwCount);
int SeekFile(FILE *pstFile , int iOffset , int iOrigin);
int CloseFile(FILE *pstFile);
BOOL IsFileOpened(const DIRECTORYENTRY *pstEntry);
int RemoveFile(const char *pcFileName);
DIR *OpenDirectory(const char *pcDirectoryName);
struct DirectoryEntryStruct *ReadDirectory(DIR *pstDirectory);
void RewindDirectory(DIR *pstDirectory);
int CloseDirectory(DIR *pstDirectory);
BOOL FlushFileSystemCache(void);

#endif
