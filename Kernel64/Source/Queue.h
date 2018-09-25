#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "Types.h"

#pragma pack(push , 1)

typedef struct QueueManagerStruct {
	int DataSize;
	int MaxDataCount;
	void *QueueArray;
	int PutIndex;
	int GetIndex;
	BOOL LastOperationPut;
}QUEUE;

#pragma pack(pop)

void InitQueue(QUEUE *Queue , void *QueueBuffer , int MaxDataCount , int DataSize);
BOOL IsQueueFull(const QUEUE *Queue);
BOOL IsQueueEmpty(const QUEUE *Queue);
BOOL PutQueue(QUEUE *Queue , const void *Data);
BOOL GetQueue(QUEUE *Queue , void *Data);

#endif
