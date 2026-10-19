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

void* HeapManager::Allocate(size_t chunkSize)
{
	MemBlockDesc* currentBlock = head->next;
	MemBlockDesc* nextBlock = head->next;
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
			currentBlock = nextBlock;
			if (currentBlock->fullSize >= neededSize && !currentBlock->isInUse)
			{
				currentBlock->userSize = chunkSize;
				currentBlock->isInUse = true;
				currentBlock->userStart = reinterpret_cast<char*>(currentBlock) + sizeof(MemBlockDesc);
				return currentBlock->userStart;
			}
			nextBlock = currentBlock->next;
		} while (currentBlock->next != nullptr);
		//first hand memory (currentBlock is now the last block in the list)
		if ((heapManager->endOfHeap - heapManager->currentPosOfHeap) >= neededSize)
		{
			MemBlockDesc* newBlock = reinterpret_cast<MemBlockDesc*>(heapManager->currentPosOfHeap);
			currentBlock->next = newBlock;
			newBlock->userSize = chunkSize;
			newBlock->fullSize = neededSize;
			newBlock->isInUse = true;
			newBlock->userStart = heapManager->currentPosOfHeap + sizeof(MemBlockDesc);
			newBlock->next = nullptr;
			heapManager->currentPosOfHeap += newBlock->fullSize;
			return newBlock->userStart;
		}
		return nullptr;
	}
}


bool HeapManager::Free(void* pPtr)
{
	MemBlockDesc* currentBlock = head->next;
	MemBlockDesc* nextBlock = head->next;
	do
	{
		currentBlock = nextBlock;
		if ((reinterpret_cast<MemBlockDesc*>(reinterpret_cast<char*>(pPtr) - sizeof(MemBlockDesc)) == currentBlock) && currentBlock->isInUse == true)
		{
			currentBlock->isInUse = false;
			return true;
		}
		nextBlock = currentBlock->next;
	} while (currentBlock->next != nullptr);
	return false;
}

void  HeapManager::Coalesce()
{
	MemBlockDesc* currentBlock = head->next;
	MemBlockDesc* nextBlock = head->next;
	MemBlockDesc* adjacentBlock;
	while (currentBlock->next != nullptr)
	{
		do
		{
			currentBlock = nextBlock;
			adjacentBlock = reinterpret_cast<MemBlockDesc*>(reinterpret_cast<char*>(currentBlock) + currentBlock->fullSize);
			if (adjacentBlock == currentBlock->next && currentBlock->isInUse == false && currentBlock->next->isInUse == false)
			{
				currentBlock->fullSize += currentBlock->next->fullSize;
				currentBlock->next = currentBlock->next->next;
			}
			nextBlock = currentBlock->next;
		} while (currentBlock->next != nullptr);
		currentBlock = head->next;
		nextBlock = head->next;
	}
	currentBlock = head->next;
	if (currentBlock->next == nullptr)
	{
		memoryList = nullptr;
		currentPosOfHeap = startOfHeap;
	}
}
