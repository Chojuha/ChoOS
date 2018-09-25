#include "CacheManager.h"
#include "FileSystem.h"
#include "DynamicMemory.h"

static CACHEMANAGER CacheManager;

BOOL InitCacheManager(void) {
	int i;
	MemSet(&CacheManager , 0 , sizeof(CacheManager));
	CacheManager.AccessTime[CACHE_CLUSTERLINKTABLEAREA] = 0;
	CacheManager.AccessTime[CACHE_DATAAREA] = 0;
	CacheManager.MaxCount[CACHE_CLUSTERLINKTABLEAREA] = CACHE_MAXCLUSTERLINKTABLEAREACOUNT;
	CacheManager.MaxCount[CACHE_DATAAREA] = CACHE_MAXDATAAREACOUNT;
	CacheManager.Buffer[CACHE_CLUSTERLINKTABLEAREA] = (BYTE*)AllocateMemory(CACHE_MAXCLUSTERLINKTABLEAREACOUNT*512);
	if(CacheManager.Buffer[CACHE_CLUSTERLINKTABLEAREA] == NULL) {
		return FALSE;
	}
	for(i = 0; i < CACHE_MAXCLUSTERLINKTABLEAREACOUNT; i++) {
		CacheManager.CacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].Buffer = CacheManager.Buffer[CACHE_CLUSTERLINKTABLEAREA]+(i*512);
		CacheManager.CacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].Tag = CACHE_INVALIDTAG;
	}
	CacheManager.Buffer[CACHE_DATAAREA] = (BYTE*)AllocateMemory(CACHE_MAXDATAAREACOUNT*FILESYSTEM_CLUSTERSIZE);
	if(CacheManager.Buffer[CACHE_DATAAREA] == NULL) {
		FreeMemory(CacheManager.Buffer[CACHE_CLUSTERLINKTABLEAREA]);
		return FALSE;
	}
	for(i = 0; i < CACHE_MAXDATAAREACOUNT; i++) {
		CacheManager.CacheBuffer[CACHE_DATAAREA][i].Buffer = CacheManager.Buffer[CACHE_DATAAREA]+(i*FILESYSTEM_CLUSTERSIZE);
		CacheManager.CacheBuffer[CACHE_DATAAREA][i].Tag = CACHE_INVALIDTAG;
	}
	return TRUE;
}

CACHEBUFFER *AllocateCacheBuffer(int CacheTableIndex) {
	CACHEBUFFER *CacheBuffer;
	int i;
	if(CacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
		return FALSE;
	}
	CutDownAccessTime(CacheTableIndex);
	CacheBuffer = CacheManager.CacheBuffer[CacheTableIndex];
	for(i = 0; i < CacheManager.MaxCount[CacheTableIndex]; i++) {
		if(CacheBuffer[i].Tag == CACHE_INVALIDTAG) {
			CacheBuffer[i].Tag = CACHE_INVALIDTAG-1;
			CacheBuffer[i].AccessTime = CacheManager.AccessTime[CacheTableIndex]++;
			return &(CacheBuffer[i]);
		}
	}
	return NULL;
}

CACHEBUFFER *FindCacheBuffer(int CacheTableIndex , DWORD Tag) {
	CACHEBUFFER *CacheBuffer;
	int i;
	if(CacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
		return FALSE;
	}
	CutDownAccessTime(CacheTableIndex);
	CacheBuffer = CacheManager.CacheBuffer[CacheTableIndex];
	for(i = 0; i < CacheManager.MaxCount[CacheTableIndex]; i++) {
		if(CacheBuffer[i].Tag == Tag) {
			CacheBuffer[i].AccessTime = CacheManager.AccessTime[CacheTableIndex]++;
			return &(CacheBuffer[i]);
		}
	}
	return NULL;
}

static void CutDownAccessTime(int CacheTableIndex) {
	CACHEBUFFER Temp;
	CACHEBUFFER *CacheBuffer;
	BOOL Sorted;
	int i;
	int j;
	if(CacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
		return;
	}
	if(CacheManager.AccessTime[CacheTableIndex] < 0xfffffffe) {
		return;
	}
	CacheBuffer = CacheManager.CacheBuffer[CacheTableIndex];
	for(j = 0; j < CacheManager.MaxCount[CacheTableIndex]-1; j++) {
		Sorted = TRUE;
		for(i = 0; i < CacheManager.MaxCount[CacheTableIndex]-1-j; i++) {
			if(CacheBuffer[i].AccessTime > CacheBuffer[i+1].AccessTime) {
				Sorted = FALSE;
				MemCpy(&Temp , &(CacheBuffer[i]) , sizeof(CACHEBUFFER));
				MemCpy(&(CacheBuffer[i]) , &(CacheBuffer[i+1]) , sizeof(CACHEBUFFER));
				MemCpy(&(CacheBuffer[i+1]) , &Temp , sizeof(CACHEBUFFER));
			}
		}
		if(Sorted == TRUE) {
			break;
		}
	}
	for(i = 0; i < CacheManager.MaxCount[CacheTableIndex]; i++) {
		CacheBuffer[i].AccessTime = i;
	}
	CacheManager.AccessTime[CacheTableIndex] = i;
}

CACHEBUFFER *GetVictimInCacheBuffer(int CacheTableIndex) {
	DWORD OldTime;
	CACHEBUFFER *CacheBuffer;
	int OldIndex;
	int i;
	if(CacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
		return FALSE;
	}
	OldIndex = -1;
	OldTime - 0xFFFFFFFF;
	CacheBuffer = CacheManager.CacheBuffer[CacheTableIndex];
	for(i = 0; i < CacheManager.MaxCount[CacheTableIndex]; i++) {
		if(CacheBuffer[i].Tag == CACHE_INVALIDTAG) {
			OldIndex = i;
			break;
		}
		if(CacheBuffer[i].AccessTime < OldTime) {
			OldTime = CacheBuffer[i].AccessTime;
			OldIndex = i;
		}
	}
	if(OldIndex == -1) {
		return NULL;
	}
	CacheBuffer[OldIndex].AccessTime = CacheManager.AccessTime[CacheTableIndex]++;
	return &(CacheBuffer[OldIndex]);
}

void DiscardAllCacheBuffer(int CacheTableIndex) {
	CACHEBUFFER *CacheBuffer;
	int i;
	CacheBuffer = CacheManager.CacheBuffer[CacheTableIndex];
	for(i = 0; i < CacheManager.MaxCount[CacheTableIndex]; i++) {
		CacheBuffer[i].Tag = CACHE_INVALIDTAG;
	}
	CacheManager.AccessTime[CacheTableIndex] = 0;
}

BOOL GetCacheBufferAndCount(int CacheTableIndex , CACHEBUFFER **CacheBuffer , int *MaxCount) {
	if(CacheTableIndex > CACHE_MAXCACHETABLEINDEX) {
		return FALSE;
	}
	*CacheBuffer = CacheManager.CacheBuffer[CacheTableIndex];
	*MaxCount = CacheManager.MaxCount[CacheTableIndex];
	return TRUE;
}
