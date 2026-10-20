#include"HeapManager.h"


HeapManager* HeapManager::heapManager = nullptr;
HeapManager::MemBlockDesc* HeapManager::memoryList = nullptr;
// Dummy node: only head->next is used, and it always points at the start of the heap buffer.
HeapManager::MemBlockDesc HeapManager::head = { 0, 0, nullptr, nullptr, false };

// Creates the single HeapManager instance on first call; later calls return the same instance.
HeapManager* HeapManager::Create(void* heapMemory, size_t sizeHeap)
{
	if (heapManager == nullptr)
	{
		heapManager = new HeapManager();
		heapManager->startOfHeap = reinterpret_cast<char*>(heapMemory);
		heapManager->currentPosOfHeap = heapManager->startOfHeap;
		heapManager->endOfHeap = reinterpret_cast<char*>(heapMemory) + sizeHeap;	// one past the end
		// The first block will be placed at the very start of the buffer (not written until the first Allocate).
		head.next = reinterpret_cast<MemBlockDesc*>(heapMemory);
		return heapManager;
	}
	else
	{
		return heapManager;
	}
}

// Returns a pointer to chunkSize usable bytes, or nullptr if the heap is full.
// Strategy: reuse the first free block that is big enough (first fit),
// otherwise carve a new block from the unused memory at the end of the heap.
void* HeapManager::Allocate(size_t chunkSize)
{
	MemBlockDesc* currentBlock = head.next;
	MemBlockDesc* nextBlock = head.next;
	// Header + user data, rounded up to a multiple of 4 bytes
	size_t neededSize = ROUND_UP(sizeof(MemBlockDesc) + chunkSize, 4);

	// Case 1: the heap is empty, so this becomes the first block in the list
	if (memoryList == nullptr && (heapManager->endOfHeap - heapManager->currentPosOfHeap) >= neededSize)
	{
		memoryList = head.next;
		memoryList->userSize = chunkSize;
		memoryList->fullSize = neededSize;
		memoryList->isInUse = true;
		memoryList->next = nullptr;
		memoryList->userStart = reinterpret_cast<char*>(memoryList) + sizeof(MemBlockDesc);	// user data starts right after the header
		heapManager->currentPosOfHeap += memoryList->fullSize;
		return memoryList->userStart;
	}
	else
	{
		// Case 2: second hand memory - reuse the first freed block that is big enough.
		// The whole block is reused; it is not split, so any extra space is wasted.
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

		// Case 3: first hand memory - carve a new block from the unused end of the heap
		// and link it after the last block (currentBlock is now the last block in the list).
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

		// Out of memory
		return nullptr;
	}
}


// Marks the block owning pPtr as free. Returns false if pPtr is not an allocated block
// (invalid pointer or double free). Free blocks are not merged here; call Coalesce for that.
bool HeapManager::Free(void* pPtr)
{
	MemBlockDesc* currentBlock = head.next;
	MemBlockDesc* nextBlock = head.next;
	do
	{
		currentBlock = nextBlock;
		// A block's header sits just before its user data, so step back by one header and compare.
		// Only addresses are compared; pPtr itself is never read, so a bad pointer is safe.
		if ((reinterpret_cast<MemBlockDesc*>(reinterpret_cast<char*>(pPtr) - sizeof(MemBlockDesc)) == currentBlock) && currentBlock->isInUse)
		{
			currentBlock->isInUse = false;
			return true;
		}
		nextBlock = currentBlock->next;
	} while (currentBlock->next != nullptr);
	return false;
}

// Merges neighbouring free blocks into larger ones so big allocations can fit again.
// If everything merges into a single block, the heap is reset to empty.
void  HeapManager::Coalesce()
{
	MemBlockDesc* currentBlock = head.next;
	MemBlockDesc* nextBlock = head.next;
	MemBlockDesc* adjacentBlock;
	bool mergedAny = true;
	// Repeat passes over the list, because one pass can miss merges
	while (mergedAny)
	{
		mergedAny = false;
		// One pass: merge each free block with the free block directly after it
		do
		{
			currentBlock = nextBlock;
			// Where the next block should start in memory: this block's address + its size
			adjacentBlock = reinterpret_cast<MemBlockDesc*>(reinterpret_cast<char*>(currentBlock) + currentBlock->fullSize);
			if (adjacentBlock == currentBlock->next && !currentBlock->isInUse && !currentBlock->next->isInUse)
			{
				// Absorb the neighbour's space and unlink it from the list
				currentBlock->fullSize += currentBlock->next->fullSize;
				currentBlock->next = currentBlock->next->next;
				mergedAny = true;
			}
			nextBlock = currentBlock->next;
		} while (currentBlock->next != nullptr);
		currentBlock = head.next;
		nextBlock = head.next;
	}

	// Only one block left: treat the heap as empty again
	currentBlock = head.next;
	if (currentBlock->next == nullptr && !currentBlock->isInUse)
	{
		memoryList = nullptr;
		currentPosOfHeap = startOfHeap;
	}
}
