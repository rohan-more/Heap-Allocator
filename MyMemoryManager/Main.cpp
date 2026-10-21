#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC	// must come before any standard header so leak reports show file and line
#endif
#include <random>
#include<cstdlib>
#include<cstdio>
#include<crtdbg.h>
#include<cassert>
#include<vector>
#include<algorithm>
#include"HeapManager.h"

constexpr size_t sizeHeap = 1024 * 1024;	// 1 MB test heap

// Test results, updated by Check
static unsigned int s_checksPassed = 0;
static unsigned int s_checksFailed = 0;

// Start of the test heap, so pointers can be printed as offsets instead of raw addresses
static const char* s_heapStart = nullptr;

// Records one test result: prints [PASS] or [FAIL] with the description and updates
// s_checksPassed / s_checksFailed.
void Check(bool condition, const char* description)
{
	if (condition)
	{
		printf("  [PASS] %s\n", description);
		s_checksPassed++;
	}
	else
	{
		printf("  [FAIL] %s\n", description);
		s_checksFailed++;
	}
}

// Returns how far p is from the start of the test heap, in bytes
size_t OffsetInHeap(const void* p)
{
	return static_cast<size_t>(static_cast<const char*>(p) - s_heapStart);
}

// Makes up to count allocations of random sizes (0 to maxSize bytes) and appends
// the returned pointers to outAddresses. Stops early if the heap runs out of memory.
void AllocateRandomBlocks(HeapManager& heap, unsigned int count, size_t maxSize, std::vector<void*>& outAddresses, std::mt19937& rng)
{
	for (unsigned int i = 0; i < count; i++)
	{
		size_t sizeAlloc = static_cast<size_t>(rng()) % (maxSize + 1);
		void* pPtr = heap.Allocate(sizeAlloc);
		if (pPtr == nullptr)
		{
			printf("  Allocate(%4zu) -> FAILED, heap is full\n", sizeAlloc);
			break;
		}
		printf("  Allocate(%4zu) -> data at offset %7zu\n", sizeAlloc, OffsetInHeap(pPtr));
		outAddresses.push_back(pPtr);
	}
}

// Frees every pointer in allocatedAddresses in a random order and empties the vector.
// Random order leaves gaps in the heap, like a real program would.
// Returns how many of the frees succeeded.
unsigned int FreeAllInRandomOrder(HeapManager& heap, std::vector<void*>& allocatedAddresses, std::mt19937& rng)
{
	unsigned int freedCount = 0;
	if (!allocatedAddresses.empty())
	{
		// randomize the addresses
		std::shuffle(allocatedAddresses.begin(), allocatedAddresses.end(), rng);

		// return them back to the heap manager
		while (!allocatedAddresses.empty())
		{
			if (heap.Free(allocatedAddresses.back()))
			{
				freedCount++;
			}
			allocatedAddresses.pop_back();
		}
	}
	printf("  Freed %u blocks in random order\n", freedCount);
	return freedCount;
}

int main()
{
	// Report memory leaks in the Output window when the program exits (Debug builds only)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// One random engine shared by every test. Replace the seed with a fixed number
	// (e.g. rng(42)) to repeat exactly the same run when debugging a failure.
	std::mt19937 rng(std::random_device{}());
	// allocate the memory the heap manager will manage (freed with _aligned_free, not free)
	void * pHeapMemory = _aligned_malloc(sizeHeap, 16);
	assert(pHeapMemory);
	s_heapStart = static_cast<const char*>(pHeapMemory);

	// create a HeapManager instance
	HeapManager * pHeapManager = HeapManager::Create(pHeapMemory, sizeHeap);
	assert(pHeapManager);

	std::vector<void *> allocatedAddresses;

	////////////////////////////   TEST 1: FIRST HAND ALLOC AND FREE   ////////////////////////////
	// 4 allocations of up to 1023 bytes from fresh memory, then free them all in random order
	printf("=== Test 1: fresh allocations (4 x up to 1023 bytes) ===\n");
	AllocateRandomBlocks(*pHeapManager, 4, 1023, allocatedAddresses, rng);
	Check(allocatedAddresses.size() == 4, "All 4 allocations succeed");
	pHeapManager->PrintHeap();
	Check(FreeAllInRandomOrder(*pHeapManager, allocatedAddresses, rng) == 4, "All 4 blocks are freed");
	pHeapManager->PrintHeap();

	////////////////////////////   TEST 2: SECOND HAND ALLOC AND FREE   ////////////////////////////
	// 6 allocations of up to 4095 bytes; any that fit in a block freed above reuse it
	printf("\n=== Test 2: reuse freed blocks (6 x up to 4095 bytes) ===\n");
	AllocateRandomBlocks(*pHeapManager, 6, 4095, allocatedAddresses, rng);
	Check(allocatedAddresses.size() == 6, "All 6 allocations succeed");
	pHeapManager->PrintHeap();
	Check(FreeAllInRandomOrder(*pHeapManager, allocatedAddresses, rng) == 6, "All 6 blocks are freed");

	////////////////////////////   TEST 3: FREE ONLY FREES ALLOCATED MEMORY   ////////////////////////////
	printf("\n=== Test 3: invalid frees ===\n");
	char* pBlock = static_cast<char*>(pHeapManager->Allocate(16));
	Check(pBlock != nullptr, "Allocate a 16-byte block to test with");
	Check(!pHeapManager->Free(nullptr), "Free rejects nullptr");
	Check(!pHeapManager->Free(pBlock + 1), "Free rejects a pointer into the middle of a block");
	Check(!pHeapManager->Free(reinterpret_cast<void *>(rng())), "Free rejects a random address");
	Check(pHeapManager->Free(pBlock), "Free accepts the real block");
	Check(!pHeapManager->Free(pBlock), "Free rejects a double free");

	////////////////////////////   TEST 4: COALESCE   ////////////////////////////
	// everything is free, so Coalesce merges it back into one block and resets the heap
	printf("\n=== Test 4: coalesce after freeing everything ===\n");
	printf("  Before Coalesce:\n");
	pHeapManager->PrintHeap();
	pHeapManager->Coalesce();
	printf("  After Coalesce:\n");
	pHeapManager->PrintHeap();
	void * pBig = pHeapManager->Allocate(sizeHeap - 8192);
	Check(pBig != nullptr, "A near-full-heap allocation succeeds");
	Check(pHeapManager->Free(pBig), "The large block is freed");

	////////////////////////////   RESULTS   ////////////////////////////
	printf("\n----------------------------------------\n");
	printf("Result: %u passed, %u failed\n", s_checksPassed, s_checksFailed);

	// destroy the manager before freeing the memory it manages
	HeapManager::Destroy();
	_aligned_free(pHeapMemory);
	return s_checksFailed == 0 ? 0 : 1;	// non-zero exit code tells scripts / CI that a test failed
}
