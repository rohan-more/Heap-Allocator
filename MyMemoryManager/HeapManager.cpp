#include"HeapManager.h"

HeapManager * HeapManager::heapManager = NULL;
HeapManager::MemBlockDesc * HeapManager::memoryList = NULL;
HeapManager::MemBlockDesc * HeapManager::head = reinterpret_cast<MemBlockDesc *>(malloc(sizeof(HeapManager::MemBlockDesc)));

HeapManager * HeapManager::Create(void * heapMemory, size_t sizeHeap)
{
	if (heapManager == NULL)
	{
		heapManager = new HeapManager();
		heapManager->startOfHeap = reinterpret_cast<char *>(heapMemory);
		heapManager->currentPosOfHeap = heapManager->startOfHeap;
		heapManager->endOfHeap = reinterpret_cast<char *>(heapMemory)+sizeHeap;
		head->next = reinterpret_cast<MemBlockDesc *>(heapMemory);
		return heapManager;
	}
	else
	{
		return heapManager;
	}
}

void *  HeapManager::myAlloc(size_t chunkSize)
{
	MemBlockDesc * traverseTemp = head->next;
	MemBlockDesc * tempTraverseTemp = head->next;
	if (memoryList == NULL && (heapManager->endOfHeap - heapManager->currentPosOfHeap) >= ROUND_UP(sizeof(MemBlockDesc) + chunkSize, 4))
	{
		memoryList = head->next;
		memoryList->userSize = chunkSize;
		memoryList->fullSize = ROUND_UP(sizeof(MemBlockDesc) + traverseTemp->userSize, 4);
		memoryList->isInUse = true;
		memoryList->next = NULL;
		memoryList->userStart = reinterpret_cast<char *>(memoryList)+sizeof(MemBlockDesc);
		heapManager->currentPosOfHeap += memoryList->fullSize;
		return memoryList->userStart;
	}
	else
	{
		//second hand memory	
		do
		{
			traverseTemp = tempTraverseTemp;
			if (traverseTemp->fullSize >= ROUND_UP(sizeof(MemBlockDesc) + chunkSize, 4) && traverseTemp->isInUse == false)
			{
				traverseTemp->userSize = chunkSize;
				traverseTemp->isInUse = true;
				traverseTemp->userStart = reinterpret_cast<char *>(traverseTemp)+sizeof(MemBlockDesc);
				return traverseTemp->userStart;
			}
			tempTraverseTemp = traverseTemp->next;
		} while (traverseTemp->next != NULL);
		//first hand memory
		if ((heapManager->endOfHeap - heapManager->currentPosOfHeap) >= ROUND_UP(sizeof(MemBlockDesc) + chunkSize, 4))
		{
			tempTraverseTemp = reinterpret_cast<MemBlockDesc *>(heapManager->currentPosOfHeap);
			traverseTemp->next = tempTraverseTemp;
			tempTraverseTemp->userSize = chunkSize;
			tempTraverseTemp->fullSize = ROUND_UP(sizeof(MemBlockDesc) + tempTraverseTemp->userSize, 4);
			tempTraverseTemp->isInUse = true;
			tempTraverseTemp->userStart = heapManager->currentPosOfHeap + sizeof(MemBlockDesc);
			tempTraverseTemp->next = NULL;
			heapManager->currentPosOfHeap += tempTraverseTemp->fullSize;
			return tempTraverseTemp->userStart;
		}
		return NULL;
	}
}


bool HeapManager::myFree(void * pPtr)
{
	MemBlockDesc * traverseTemp = head->next;
	MemBlockDesc * tempTraverseTemp = head->next;
	do
	{
		traverseTemp = tempTraverseTemp;
		if ((reinterpret_cast<MemBlockDesc *>(reinterpret_cast<char *>(pPtr)-sizeof(MemBlockDesc)) == traverseTemp) && traverseTemp->isInUse == true)
		{
			traverseTemp->isInUse = false;
			return true;
		}
		tempTraverseTemp = traverseTemp->next;
	} while (traverseTemp->next != NULL);
	return false;
}

void  HeapManager::myCollect()
{
	MemBlockDesc * traverseTemp = head->next;
	MemBlockDesc * tempTraverseTemp = head->next;
	MemBlockDesc * tempTraverseTemp2;
	while (traverseTemp->next != NULL)
	{	
		do
		{
			traverseTemp = tempTraverseTemp;
			tempTraverseTemp2 = reinterpret_cast<MemBlockDesc *>(reinterpret_cast<char *>(traverseTemp)+traverseTemp->fullSize);
			if (tempTraverseTemp2 == traverseTemp->next && traverseTemp->isInUse == false && traverseTemp->next->isInUse == false)
			{
				traverseTemp->fullSize += traverseTemp->next->fullSize;
				traverseTemp->next = traverseTemp->next->next;
			}
			tempTraverseTemp = traverseTemp->next;
		} while (traverseTemp->next != NULL);
		traverseTemp = head->next;
		tempTraverseTemp = head->next;
	}
	traverseTemp = head->next;
	if (traverseTemp->next == NULL)
	{
		memoryList = NULL;
		currentPosOfHeap = startOfHeap;
	}
}