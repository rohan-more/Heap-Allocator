#include"HeapManager.h"


HeapManager* HeapManager::heapManager = nullptr;
HeapManager::MemBlockDesc* HeapManager::memoryList = nullptr;
HeapManager::MemBlockDesc* HeapManager::head = reinterpret_cast<MemBlockDesc*>(malloc(sizeof(HeapManager::MemBlockDesc)));

HeapManager* HeapManager::Create(void* heapMemory, size_t sizeHeap)
{
	if (heapManager == nullptr)
	{
		heapManager = new HeapManager();
		heapManager->startOfHeap = reinterpret_cast<char*>(heapMemory);
		heapManager->currentPosOfHeap = heapManager->startOfHeap;
		heapManager->endOfHeap = reinterpret_cast<char*>(heapMemory) + sizeHeap;
		head->next = reinterpret_cast<MemBlockDesc*>(heapMemory);
		return heapManager;
	}
	else
	{
		return heapManager;
	}
}

void* HeapManager::myAlloc(size_t chunkSize)
{
	MemBlockDesc* traverseTemp = head->next;
	MemBlockDesc* tempTraverseTemp = head->next;
	size_t neededSize = ROUND_UP(sizeof(MemBlockDesc) + chunkSize, 4);
	if (memoryList == nullptr && (heapManager->endOfHeap - heapManager->currentPosOfHeap) >= neededSize)
	{
		memoryList = head->next;
		memoryList->userSize = chunkSize;
		memoryList->fullSize = neededSize;
		memoryList->isInUse = true;
		memoryList->next = nullptr;
		memoryList->userStart = reinterpret_cast<char*>(memoryList) + sizeof(MemBlockDesc);
		heapManager->currentPosOfHeap += memoryList->fullSize;
		return memoryList->userStart;
	}
	else
	{
		//second hand memory	
		do
		{
			traverseTemp = tempTraverseTemp;
			if (traverseTemp->fullSize >= neededSize && !traverseTemp->isInUse)
			{
				traverseTemp->userSize = chunkSize;
				traverseTemp->isInUse = true;
				traverseTemp->userStart = reinterpret_cast<char*>(traverseTemp) + sizeof(MemBlockDesc);
				return traverseTemp->userStart;
			}
			tempTraverseTemp = traverseTemp->next;
		} while (traverseTemp->next != nullptr);
		//first hand memory
		if ((heapManager->endOfHeap - heapManager->currentPosOfHeap) >= neededSize)
		{
			tempTraverseTemp = reinterpret_cast<MemBlockDesc*>(heapManager->currentPosOfHeap);
			traverseTemp->next = tempTraverseTemp;
			tempTraverseTemp->userSize = chunkSize;
			tempTraverseTemp->fullSize = neededSize;
			tempTraverseTemp->isInUse = true;
			tempTraverseTemp->userStart = heapManager->currentPosOfHeap + sizeof(MemBlockDesc);
			tempTraverseTemp->next = nullptr;
			heapManager->currentPosOfHeap += tempTraverseTemp->fullSize;
			return tempTraverseTemp->userStart;
		}
		return nullptr;
	}
}


bool HeapManager::myFree(void* pPtr)
{
	MemBlockDesc* traverseTemp = head->next;
	MemBlockDesc* tempTraverseTemp = head->next;
	do
	{
		traverseTemp = tempTraverseTemp;
		if ((reinterpret_cast<MemBlockDesc*>(reinterpret_cast<char*>(pPtr) - sizeof(MemBlockDesc)) == traverseTemp) && traverseTemp->isInUse == true)
		{
			traverseTemp->isInUse = false;
			return true;
		}
		tempTraverseTemp = traverseTemp->next;
	} while (traverseTemp->next != nullptr);
	return false;
}

void  HeapManager::myCollect()
{
	MemBlockDesc* traverseTemp = head->next;
	MemBlockDesc* tempTraverseTemp = head->next;
	MemBlockDesc* tempTraverseTemp2;
	while (traverseTemp->next != nullptr)
	{
		do
		{
			traverseTemp = tempTraverseTemp;
			tempTraverseTemp2 = reinterpret_cast<MemBlockDesc*>(reinterpret_cast<char*>(traverseTemp) + traverseTemp->fullSize);
			if (tempTraverseTemp2 == traverseTemp->next && traverseTemp->isInUse == false && traverseTemp->next->isInUse == false)
			{
				traverseTemp->fullSize += traverseTemp->next->fullSize;
				traverseTemp->next = traverseTemp->next->next;
			}
			tempTraverseTemp = traverseTemp->next;
		} while (traverseTemp->next != nullptr);
		traverseTemp = head->next;
		tempTraverseTemp = head->next;
	}
	traverseTemp = head->next;
	if (traverseTemp->next == nullptr)
	{
		memoryList = nullptr;
		currentPosOfHeap = startOfHeap;
	}
}
