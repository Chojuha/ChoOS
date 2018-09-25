#include "List.h"

void InitList(LIST *List) {
	List->ItemCount = 0;
	List->Header = NULL;
	List->Tail = NULL;
}

int GetListCount(const LIST *List) {
	return List->ItemCount;
}

void AddListToTail(LIST *List , void *Item) {
	LISTLINK *Link;
	Link = (LISTLINK*)Item;
	Link->Next = NULL;
	if(List->Header == NULL) {
		List->Header = Item;
		List->Tail = Item;
		List->ItemCount = 1;
		return;
	}
	Link = (LISTLINK*)List->Tail;
	Link->Next = Item;
	List->Tail = Item;
	List->ItemCount++;
}

void AddListToHeader(LIST *List , void *Item) {
	LISTLINK *Link;
	Link = (LISTLINK*)Item;
	Link->Next = List->Header;
	if(List->Header == NULL) {
		List->Header = Item;
		List->Tail = Item;
		List->ItemCount = 1;
		return;
	}
	List->Header = Item;
	List->ItemCount++;
}

void *RemoveList(LIST *List , QWORD ID) {
	LISTLINK *Link;
	LISTLINK *PrevLink;
	PrevLink = (LISTLINK*)List->Header;
	for(Link = PrevLink; Link != NULL; Link = Link->Next) {
		if(Link->ID == ID) {
			if((Link == List->Header) && (Link == List->Tail)) {
				List->Header = NULL;
				List->Tail = NULL;
			}
			else if(Link == List->Header) {
				List->Header = Link->Next;
			}
			else if(Link == List->Tail) {
				List->Tail = PrevLink;
			}
			else {
				PrevLink->Next = Link->Next;
			}
			List->ItemCount--;
			return Link;
		}
		PrevLink = Link;
	}
	return NULL;
}

void *RemoveListFromHeader(LIST *List) {
	LISTLINK *Link;
	if(List->ItemCount == 0) {
		return NULL;
	}
	Link = (LISTLINK*)List->Header;
	return RemoveList(List , Link->ID);
}

void *RemoveListFromTail(LIST *List) {
	LISTLINK *Link;
	if(List->ItemCount == 0) {
		return NULL;
	}
	Link = (LISTLINK*)List->Tail;
	return RemoveList(List , Link->ID);
}

void *FindList(const LIST *List , QWORD ID) {
	LISTLINK *Link;
	for(Link = (LISTLINK*)List->Header; Link != NULL; Link = Link->Next) {
		if(Link->ID == ID) {
			return Link;
		}
	}
	return NULL;
}

void *GetHeaderFromList(const LIST *List) {
	return List->Header;
}

void *GetTailFromList(const LIST *List) {
	return List->Tail;
}

void *GetNextFromList(const LIST *List , void *Current) {
	LISTLINK *Link;
	Link = (LISTLINK*)Current;
	return Link->Next;
}
