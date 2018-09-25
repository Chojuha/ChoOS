#include "DynamicMemory.h"
#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"

static DYNAMICMEMORY DynamicMemory;

void InitDynamicMemory(void) {
	QWORD DynamicMemorySize;
	int i;
	int j;
	BYTE *CurrentBitmapPosition;
	int BlockCountOfLevel;
	int MetaBlockCount;
	DynamicMemorySize = CalculateDynamicMemorySize();
	MetaBlockCount = CalculateMetaBlockCount(DynamicMemorySize);
	DynamicMemory.BlockCountOfSmallestBlock = (DynamicMemorySize/DYNAMICMEMORY_MINSIZE)-MetaBlockCount;
	for(i = 0; (DynamicMemory.BlockCountOfSmallestBlock >> i) > 0; i++) {
		;
	}
	DynamicMemory.MaxLevelCount = i;
	DynamicMemory.AllocatedBlockListIndex = (BYTE*)DYNAMICMEMORY_STARTADDRESS;
	for(i = 0; i < DynamicMemory.BlockCountOfSmallestBlock; i++) {
		DynamicMemory.AllocatedBlockListIndex[i] = 0xFF;
	}
	DynamicMemory.BitmapOfLevel = (BITMAP*)(DYNAMICMEMORY_STARTADDRESS+(sizeof(BYTE)*DynamicMemory.BlockCountOfSmallestBlock));
	CurrentBitmapPosition = ((BYTE*)DynamicMemory.BitmapOfLevel)+(sizeof(BITMAP)*DynamicMemory.MaxLevelCount);
	for(j = 0; j < DynamicMemory.MaxLevelCount; j++) {
		DynamicMemory.BitmapOfLevel[j].Bitmap = CurrentBitmapPosition;
		DynamicMemory.BitmapOfLevel[j].ExistBitCount = 0;
		BlockCountOfLevel = DynamicMemory.BlockCountOfSmallestBlock >> j;
		for(i = 0; i < BlockCountOfLevel/8; i++) {
			*CurrentBitmapPosition = 0x00;
			CurrentBitmapPosition++;
		}
		if((BlockCountOfLevel % 8) != 0) {
			*CurrentBitmapPosition = 0x00;
			i = BlockCountOfLevel % 8;
			if((i % 2) == 1) {
				*CurrentBitmapPosition |= (DYNAMICMEMORY_EXIST << (i-1));
				DynamicMemory.BitmapOfLevel[j].ExistBitCount = 1;
			}
			CurrentBitmapPosition++;
		}
	}
	DynamicMemory.StartAddress = DYNAMICMEMORY_STARTADDRESS+MetaBlockCount*DYNAMICMEMORY_MINSIZE;
	DynamicMemory.EndAddress = CalculateDynamicMemorySize()+DYNAMICMEMORY_STARTADDRESS;
	DynamicMemory.UsedSize = 0;
	InitSpinLock(&(DynamicMemory.SpinLock));
}

static QWORD CalculateDynamicMemorySize(void) {
	QWORD RAMSize;
	RAMSize = (GetTotalRamSize()*1024*1024);
	if(RAMSize > (QWORD)3*1024*1024*1024) {
		RAMSize = (QWORD)3*1024*1024*1024;
	}
	return RAMSize-DYNAMICMEMORY_STARTADDRESS;
}

static int CalculateMetaBlockCount(QWORD DynamicRAMSize) {
	long BlockCountOfSmallestBlock;
	DWORD SizeOfAllocatedBlockListIndex;
	DWORD SizeOfBitmap;
	long i;
	BlockCountOfSmallestBlock = DynamicRAMSize/DYNAMICMEMORY_MINSIZE;
	SizeOfAllocatedBlockListIndex = BlockCountOfSmallestBlock*sizeof(BYTE);
	SizeOfBitmap = 0;
	for(i = 0; (BlockCountOfSmallestBlock >> i) > 0; i++) {
		SizeOfBitmap += sizeof(BITMAP);
		SizeOfBitmap += ((BlockCountOfSmallestBlock >> i)+7)/8;
	}
	return (SizeOfAllocatedBlockListIndex+SizeOfBitmap+DYNAMICMEMORY_MINSIZE-1)/DYNAMICMEMORY_MINSIZE;
}

void *AllocateMemory(QWORD Size) {
	QWORD AlignedSize;
	QWORD RelativeAddress;
	long Offset;
	int SizeArrayOffset;
	int IndexOfBlockList;
	AlignedSize = GetBuddyBlockSize(Size);
	if(AlignedSize == 0) {
		return NULL;
	}
	if(DynamicMemory.StartAddress+DynamicMemory.UsedSize+AlignedSize > DynamicMemory.EndAddress) {
		return NULL;
	}
	Offset = AllocationBuddyBlock(AlignedSize);
	if(Offset == -1) {
		return NULL;
	}
	IndexOfBlockList = GetBlockListIndexOfMatchSize(AlignedSize);
	RelativeAddress = AlignedSize*Offset;
	SizeArrayOffset = RelativeAddress/DYNAMICMEMORY_MINSIZE;
	DynamicMemory.AllocatedBlockListIndex[SizeArrayOffset] = (BYTE)IndexOfBlockList;
	DynamicMemory.UsedSize += AlignedSize;
	return (void*)(RelativeAddress+DynamicMemory.StartAddress);
}

static QWORD GetBuddyBlockSize(QWORD Size) {
	long i;
	for(i = 0; i < DynamicMemory.MaxLevelCount; i++) {
		if(Size <= (DYNAMICMEMORY_MINSIZE << i)) {
			return (DYNAMICMEMORY_MINSIZE << i);
		}
	}
	return 0;
}

static int AllocationBuddyBlock(QWORD AlignedSize) {
	int BlockListIndex;
	int FreeOffset;
	int i;
	BlockListIndex = GetBlockListIndexOfMatchSize(AlignedSize);
	if(BlockListIndex == -1) {
		return -1;
	}
	LockForSpinLock(&(DynamicMemory.SpinLock));
	for(i = BlockListIndex; i < DynamicMemory.MaxLevelCount; i++) {
		FreeOffset = FindFreeBlockInBitmap(i);
		if(FreeOffset != -1) {
			break;
		}
	}
	if(FreeOffset == -1) {
		UnLockForSpinLock(&(DynamicMemory.SpinLock));
		return -1;
	}
	SetFlagInBitmap(i , FreeOffset , DYNAMICMEMORY_EMPTY);
	if(i > BlockListIndex) {
		for(i = i-1; i >= BlockListIndex; i--) {
			SetFlagInBitmap(i , FreeOffset*2 , DYNAMICMEMORY_EMPTY);
			SetFlagInBitmap(i , FreeOffset*2+1 , DYNAMICMEMORY_EXIST);
			FreeOffset = FreeOffset*2;
		}
	}
	UnLockForSpinLock(&(DynamicMemory.SpinLock));
	return FreeOffset;
}

static int GetBlockListIndexOfMatchSize(QWORD AlignedSize) {
	int i;
	for(i = 0; i < DynamicMemory.MaxLevelCount; i++) {
		if(AlignedSize <= (DYNAMICMEMORY_MINSIZE << i)) {
			return i;
		}
	}
	return -1;
}

static int FindFreeBlockInBitmap(int BlockListIndex) {
	int i;
	int MaxCount;
	BYTE *Bitmap;
	QWORD *BitmapQWORD;
	if(DynamicMemory.BitmapOfLevel[BlockListIndex].ExistBitCount == 0) {
		return -1;
	}
	MaxCount = DynamicMemory.BlockCountOfSmallestBlock >> BlockListIndex;
	Bitmap = DynamicMemory.BitmapOfLevel[BlockListIndex].Bitmap;
	for(i = 0; i < MaxCount;) {
		if(((MaxCount-i)/64) > 0) {
			BitmapQWORD = (QWORD*)&(Bitmap[i/8]);
			if(*BitmapQWORD == 0) {
				i += 64;
				continue;
			}
		}
		if((Bitmap[i/8] & (DYNAMICMEMORY_EXIST << (i % 8))) != 0) {
			return i;
		}
		i++;
	}
	return -1;
}

static void SetFlagInBitmap(int BlockListIndex , int Offset , BYTE Flag) {
	BYTE *Bitmap;
	Bitmap = DynamicMemory.BitmapOfLevel[BlockListIndex].Bitmap;
	if(Flag == DYNAMICMEMORY_EXIST) {
		if((Bitmap[Offset/8] & (0x01 << (Offset % 8))) == 0) {
			DynamicMemory.BitmapOfLevel[BlockListIndex].ExistBitCount++;
		}
		Bitmap[Offset/8] |= (0x01 << (Offset % 8));
	}
	else {
		if((Bitmap[Offset/8] & (0x01 << (Offset % 8))) == 0) {
			DynamicMemory.BitmapOfLevel[BlockListIndex].ExistBitCount--;
		}
		Bitmap[Offset/8] &= ~(0x01 << (Offset % 8));
	}
}

BOOL FreeMemory(void *Address) {
	QWORD RelativeAddress;
	int SizeArrayOffset;
	QWORD BlockSize;
	int BlockListIndex;
	int BitmapOffset;
	if(Address == NULL) {
		return FALSE;
	}
	RelativeAddress = ((QWORD)Address)-DynamicMemory.StartAddress;
	SizeArrayOffset = RelativeAddress/DYNAMICMEMORY_MINSIZE;
	if(DynamicMemory.AllocatedBlockListIndex[SizeArrayOffset] == 0xFF) {
		return FALSE;
	}
	BlockListIndex = (int)DynamicMemory.AllocatedBlockListIndex[SizeArrayOffset];
	DynamicMemory.AllocatedBlockListIndex[SizeArrayOffset] = 0xFF;
	BlockSize = DYNAMICMEMORY_MINSIZE << BlockListIndex;
	BitmapOffset = RelativeAddress/BlockSize;
	if(FreeBuddyBlock(BlockListIndex , BitmapOffset) == TRUE) {
		DynamicMemory.UsedSize -= BlockSize;
		return TRUE;
	}
	return FALSE;
}

static BOOL FreeBuddyBlock(int BlockListIndex , int BlockOffset) {
	int BuddyBlockOffset;
	int i;
	BOOL Flag;
	LockForSpinLock(&(DynamicMemory.SpinLock));
	for(i = 0; i < DynamicMemory.MaxLevelCount; i++) {
		SetFlagInBitmap(i , BlockOffset , DYNAMICMEMORY_EXIST);
		if((BlockOffset % 2) == 0) {
			BuddyBlockOffset = BlockOffset+1;
		}
		else {
			BuddyBlockOffset = BlockOffset-1;
		}
		Flag = GetFlagInBitmap(i , BuddyBlockOffset);
		if(Flag == DYNAMICMEMORY_EMPTY) {
			break;
		}
		SetFlagInBitmap(i , BuddyBlockOffset , DYNAMICMEMORY_EMPTY);
		SetFlagInBitmap(i , BlockOffset , DYNAMICMEMORY_EMPTY);
		BlockOffset = BlockOffset/2;
	}
	UnLockForSpinLock(&(DynamicMemory.SpinLock));
	return TRUE;
}

static BYTE GetFlagInBitmap(int BlockListIndex , int Offset) {
	BYTE *Bitmap;
	Bitmap = DynamicMemory.BitmapOfLevel[BlockListIndex].Bitmap;
	if((Bitmap[Offset/8] & (0x01 << (Offset % 8))) != 0x00) {
		return DYNAMICMEMORY_EXIST;
	}
	return DYNAMICMEMORY_EMPTY;
}

void GetDynamicMemoryInformation(QWORD *DynamicMemoryStartAddress , QWORD *DynamicMemoryTotalSize , QWORD *MetaDataSize , QWORD *UsedMemorySize) {
	*DynamicMemoryStartAddress = DYNAMICMEMORY_STARTADDRESS;
	*DynamicMemoryTotalSize = CalculateDynamicMemorySize();
	*MetaDataSize = CalculateMetaBlockCount(*DynamicMemoryTotalSize)*DYNAMICMEMORY_MINSIZE;
	*UsedMemorySize = DynamicMemory.UsedSize;
}

DYNAMICMEMORY *GetDynamicMemoryManager(void) {
	return &DynamicMemory;
}
