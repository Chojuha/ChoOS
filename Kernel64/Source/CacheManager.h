#ifndef _CACHEMANAGER_H_
#define _CACHEMANAGER_H_

#include "Types.h"

#define CACHE_MAXCLUSTERLINKTABLEAREACOUNT 16
#define CACHE_MAXDATAAREACOUNT 32
#define CACHE_INVALIDTAG 0xFFFFFFFF
#define CACHE_MAXCACHETABLEINDEX 2
#define CACHE_CLUSTERLINKTABLEAREA 0
#define CACHE_DATAAREA 1

typedef struct CacheBufferStruct {
	DWORD Tag;
	DWORD AccessTime;
	BOOL Change;
	BYTE *Buffer;
}CACHEBUFFER;

typedef struct CacheManagerStruct {
	DWORD AccessTime[CACHE_MAXCACHETABLEINDEX];
	BYTE *Buffer[CACHE_MAXCACHETABLEINDEX];
	CACHEBUFFER CacheBuffer[CACHE_MAXCACHETABLEINDEX][CACHE_MAXDATAAREACOUNT];
	DWORD MaxCount[CACHE_MAXCACHETABLEINDEX];
}CACHEMANAGER;

BOOL InitCacheManager(void);
CACHEBUFFER *AllocateCacheBuffer(int CacheTableIndex);
CACHEBUFFER *FindCacheBuffer(int CacheTableIndex , DWORD Tag);
static void CutDownAccessTime(int CacheTableIndex);
CACHEBUFFER *GetVictimInCacheBuffer(int CacheTableIndex);
void DiscardAllCacheBuffer(int CacheTableIndex);
BOOL GetCacheBufferAndCount(int CacheTableIndex , CACHEBUFFER **CacheBuffer , int *MaxCount);

#endif
