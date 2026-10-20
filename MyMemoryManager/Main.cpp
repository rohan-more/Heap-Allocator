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

int main()
{
	// Report memory leaks in the Output window when the program exits (Debug builds only)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	srand(time(NULL));
	// allocate some memory for this heap
	const size_t sizeHeap = 1024 * 1024;
	void * pHeapMemory = _aligned_malloc(sizeHeap, 4);
	assert(pHeapMemory);

	// create a HeapManager instance
	HeapManager * pHeapManager = HeapManager::Create(pHeapMemory, sizeHeap);
	assert(pHeapManager);

	std::vector<void *> allocatedAddresses;

	unsigned int numberOfAllocations = 4;

	bool done = false;

	unsigned int totalAllocations = 0;
	long sizeAllocations = 0;
	////////////////////////////   UNIT TEST FOR FIRST HAND ALLOC AND FREE   ////////////////////////////
	// allocate memory of random sizes up to 1024 bytes from the heap manager
	// until it runs out of memory
	do
	{
		size_t sizeAlloc = static_cast<size_t>(rand() & 1023);// max allocation = 1024;
		void * pPtr = pHeapManager->Allocate(sizeAlloc);
		DEBUG_PRINT("The memory allocated is: %p , %d \n", pPtr,sizeAlloc);
		numberOfAllocations--;
		if (pPtr)
		{
			allocatedAddresses.push_back(pPtr);
			sizeAllocations += sizeAlloc;
			totalAllocations++;
		}
		else
		{
			done = true;
		}
	} while (!done && numberOfAllocations);
	DEBUG_PRINT("Made %d allocations totaling %ld from heap of %d\n", totalAllocations, sizeAllocations, sizeHeap);

	if (!allocatedAddresses.empty())
	{
		// randomize the addresses
		std::random_shuffle(allocatedAddresses.begin(), allocatedAddresses.end());

		// return them back to the heap manager
		while (!allocatedAddresses.empty())
		{
			void * pPtr = allocatedAddresses.back();
			if (pHeapManager->Free(pPtr))
			{
				DEBUG_PRINT("The memory at %p has been freed\n", pPtr);
			}
			allocatedAddresses.pop_back();
		}
	}
	////////////////////////////   UNIT TEST FOR SECOND HAND ALLOC AND FREE   ////////////////////////////
	// allocate memory of random sizes up to 1024 bytes from the heap manager
	// until it runs out of memory
	numberOfAllocations = 6;
	sizeAllocations = 0;
	totalAllocations = 0;
	do
	{
		size_t sizeAlloc = static_cast<size_t>(rand() & 4095);// max allocation = 4096;
		void * pPtr = pHeapManager->Allocate(sizeAlloc);
		DEBUG_PRINT("The memory allocated is: %p , %d \n", pPtr, sizeAlloc);
		numberOfAllocations--;
		if (pPtr)
		{
			allocatedAddresses.push_back(pPtr);
			sizeAllocations += sizeAlloc;
			totalAllocations++;
		}
		else
		{
			done = true;
		}
	} while (!done && numberOfAllocations);
	DEBUG_PRINT("Made %d allocations totaling %ld from heap of %d\n", totalAllocations, sizeAllocations, sizeHeap);

	if (!allocatedAddresses.empty())
	{
		// randomize the addresses
		std::random_shuffle(allocatedAddresses.begin(), allocatedAddresses.end());

		// return them back to the heap manager
		while (!allocatedAddresses.empty())
		{
			void * pPtr = allocatedAddresses.back();
			if (pHeapManager->Free(pPtr))
			{
				DEBUG_PRINT("The memory at %p has been freed\n", pPtr);
			}
			allocatedAddresses.pop_back();
		}
	}
	////////////////////////////   UNIT TEST TO CHECK FREE ONLY FREES ALLOCATED MEMORY   ////////////////////////////
	if (!pHeapManager->Free(reinterpret_cast<void *> (rand())))
	{
		DEBUG_PRINT("The memory you tried to free is not valid\n");
	}

	pHeapManager->Coalesce();
	void * pPtr = pHeapManager->Allocate(1024 * 1024 - 8192);
	DEBUG_PRINT("The memory allocated is: %p  \n", pPtr);
	_aligned_free(pHeapMemory);
	return 0;
}