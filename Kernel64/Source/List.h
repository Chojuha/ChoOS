#ifndef _LIST_H_
#define _LIST_H_

#include "Types.h"

#pragma pack(push , 1)

typedef struct ListLinkStruct {
	void *Next;
	QWORD ID;
}LISTLINK;

typedef struct ListManagerStruct {
	int ItemCount;
	void *Header;
	void *Tail;
}LIST;

#pragma pack(pop)

void InitList(LIST *List);
int GetListCount(const LIST *List);
void AddListToTail(LIST *List , void *Item);
void AddListToHeader(LIST *List , void *Item);
void *RemoveList(LIST *List , QWORD ID);
void *RemoveListFromHeader(LIST *List);
void *RemoveListFromTail(LIST *List);
void *FindList(const LIST *List , QWORD ID);
void *GetHeaderFromList(const LIST *List);
void *GetTailFromList(const LIST *List);
void *GetNextFromList(const LIST *Lost , void *Current);

#endif
