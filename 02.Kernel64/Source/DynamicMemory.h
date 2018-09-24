#ifndef _DYNAMICMEMORY_H_
#define _DYNAMICMEMORY_H_

#include "Types.h"
#include "Synchronization.h"

#define DYNAMICMEMORY_STARTADDRESS ((TASK_STACKPOOLADDRESS+(TASK_STACKSIZE*TASK_MAXCOUNT)+0xfffff) & 0xfffffffffff00000)
#define DYNAMICMEMORY_MINSIZE (1*1024)
#define DYNAMICMEMORY_EXIST 0x01
#define DYNAMICMEMORY_EMPTY 0x00

typedef struct BitmapStruct {
	BYTE *Bitmap;
	QWORD ExistBitCount;
}BITMAP;

typedef struct DynamicMemoryManagerStruct {
	SPINLOCK SpinLock;
	int MaxLevelCount;
	int BlockCountOfSmallestBlock;
	QWORD UsedSize;
	QWORD StartAddress;
	QWORD EndAddress;
	BYTE *AllocatedBlockListIndex;
	BITMAP *BitmapOfLevel;
}DYNAMICMEMORY;

void InitDynamicMemory(void);
static QWORD CalculateDynamicMemorySize(void);
static int CalculateMetaBlockCount(QWORD DynamicRAMSize);
void *AllocateMemory(QWORD Size);
static QWORD GetBuddyBlockSize(QWORD Size);
static int AllocationBuddyBlock(QWORD AlignedSize);
static int GetBlockListIndexOfMatchSize(QWORD AlignedSize);
static int FindFreeBlockInBitmap(int BlockListIndex);
static void SetFlagInBitmap(int BlockListIndex , int Offset , BYTE Flag);
BOOL FreeMemory(void *Address);
static BOOL FreeBuddyBlock(int BlockListIndex , int BlockOffset);
static BYTE GetFlagInBitmap(int BlockListIndex , int Offset);
void GetDynamicMemoryInformation(QWORD *DynamicMemoryStartAddress , QWORD *DynamicMemoryTotalSize , QWORD *MetaDataSize , QWORD *UsedMemorySize);
DYNAMICMEMORY *GetDynamicMemoryManager(void);

#endif
