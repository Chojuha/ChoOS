#include "Queue.h"

void InitQueue(QUEUE *Queue , void *QueueBuffer , int MaxDataCount , int DataSize) {
	Queue->MaxDataCount = MaxDataCount;
	Queue->DataSize = DataSize;
	Queue->QueueArray = QueueBuffer;
	Queue->PutIndex = 0;
	Queue->GetIndex = 0;
	Queue->LastOperationPut = FALSE;
}

BOOL IsQueueFull(const QUEUE *Queue) {
	if((Queue->GetIndex == Queue->PutIndex) && (Queue->LastOperationPut == TRUE)) {
		return TRUE;
	}
	return FALSE;
}

BOOL IsQueueEmpty(const QUEUE *Queue) {
	if((Queue->GetIndex == Queue->PutIndex) && (Queue->LastOperationPut == FALSE)) {
		return TRUE;
	}
	return FALSE;
}

BOOL PutQueue(QUEUE *Queue , const void *Data) {
	if(IsQueueFull(Queue) == TRUE) {
		return FALSE;
	}
	MemCpy((char*)Queue->QueueArray+(Queue->DataSize*Queue->PutIndex) , Data , Queue->DataSize);
	Queue->PutIndex = (Queue->PutIndex+1)%Queue->MaxDataCount;
	Queue->LastOperationPut = TRUE;
	return TRUE;
}

BOOL GetQueue(QUEUE *Queue , void *Data) {
	if(IsQueueEmpty(Queue) == TRUE) {
		return FALSE;
	}
	MemCpy(Data , (char*)Queue->QueueArray+(Queue->DataSize*Queue->GetIndex) , Queue->DataSize);
	Queue->GetIndex = (Queue->GetIndex+1)%Queue->MaxDataCount;
	Queue->LastOperationPut = FALSE;
	return TRUE;
}
