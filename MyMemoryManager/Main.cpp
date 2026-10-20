#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC	// must come before any standard header so leak reports show file and line
#endif
#include<cstdlib>
#include<crtdbg.h>
#include<cassert>
#include<ctime>
#include<vector>
#include<algorithm>
#include"ConsolePrint.h"
#include"HeapManager.h"

constexpr size_t sizeHeap = 1024 * 1024;	// 1 MB test heap

// Makes up to count allocations of random sizes (0 to maxSize bytes) and appends
// the returned pointers to outAddresses. Stops early if the heap runs out of memory.
void AllocateRandomBlocks(HeapManager& heap, unsigned int count, size_t maxSize, std::vector<void*>& outAddresses)
{
	unsigned int totalAllocations = 0;
	size_t sizeAllocations = 0;
	for (unsigned int i = 0; i < count; i++)
	{
		size_t sizeAlloc = static_cast<size_t>(rand()) % (maxSize + 1);
		void* pPtr = heap.Allocate(sizeAlloc);
		DEBUG_PRINT("The memory allocated is: %p , %zu \n", pPtr, sizeAlloc);
		if (pPtr == nullptr)
		{
			break;	// heap is full
		}
		outAddresses.push_back(pPtr);
		sizeAllocations += sizeAlloc;
		totalAllocations++;
	}
	DEBUG_PRINT("Made %u allocations totaling %zu bytes from heap of %zu\n", totalAllocations, sizeAllocations, sizeHeap);
}

// Frees every pointer in allocatedAddresses in a random order and empties the vector.
// Random order leaves gaps in the heap, like a real program would.
void FreeAllInRandomOrder(HeapManager& heap, std::vector<void*>& allocatedAddresses)
{
	if (!allocatedAddresses.empty())
	{
		// randomize the addresses
		std::random_shuffle(allocatedAddresses.begin(), allocatedAddresses.end());

		// return them back to the heap manager
		while (!allocatedAddresses.empty())
		{
			void* pPtr = allocatedAddresses.back();
			if (heap.Free(pPtr))
			{
				DEBUG_PRINT("The memory at %p has been freed\n", pPtr);
			}
			allocatedAddresses.pop_back();
		}
	}
}

int main()
{
	// Report memory leaks in the Output window when the program exits (Debug builds only)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	srand(time(NULL));
	// allocate the memory the heap manager will manage (freed with _aligned_free, not free)
	void * pHeapMemory = _aligned_malloc(sizeHeap, 16);
	assert(pHeapMemory);

	// create a HeapManager instance
	HeapManager * pHeapManager = HeapManager::Create(pHeapMemory, sizeHeap);
	assert(pHeapManager);

	std::vector<void *> allocatedAddresses;

	////////////////////////////   UNIT TEST FOR FIRST HAND ALLOC AND FREE   ////////////////////////////
	// 4 allocations of up to 1023 bytes from fresh memory, then free them all in random order
	AllocateRandomBlocks(*pHeapManager, 4, 1023, allocatedAddresses);
	FreeAllInRandomOrder(*pHeapManager, allocatedAddresses);

	////////////////////////////   UNIT TEST FOR SECOND HAND ALLOC AND FREE   ////////////////////////////
	// 6 allocations of up to 4095 bytes; any that fit in a block freed above reuse it
	AllocateRandomBlocks(*pHeapManager, 6, 4095, allocatedAddresses);
	FreeAllInRandomOrder(*pHeapManager, allocatedAddresses);

	////////////////////////////   UNIT TEST TO CHECK FREE ONLY FREES ALLOCATED MEMORY   ////////////////////////////
	// a small random number is never a valid block address, so Free should reject it
	if (!pHeapManager->Free(reinterpret_cast<void *> (rand())))
	{
		DEBUG_PRINT("The memory you tried to free is not valid\n");
	}

	////////////////////////////   UNIT TEST FOR COALESCE   ////////////////////////////
	// everything is free, so Coalesce merges it back into one block and a near-full-heap request fits
	pHeapManager->Coalesce();
	void * pPtr = pHeapManager->Allocate(1024 * 1024 - 8192);
	DEBUG_PRINT("The memory allocated is: %p  \n", pPtr);
	_aligned_free(pHeapMemory);
	return 0;
}
