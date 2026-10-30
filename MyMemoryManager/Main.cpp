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
#include<cstdint>
#include<limits>
#include"HeapAllocator.h"

constexpr size_t sizeHeap = 1024 * 1024;	// 1 MB test heap
constexpr size_t alignment = 16;			// every pointer from Allocate must be a multiple of this

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

// Returns true if p is a non-null pointer on an aligned address
bool IsAligned(const void* p)
{
	return p != nullptr && reinterpret_cast<std::uintptr_t>(p) % alignment == 0;
}

// Makes up to count allocations of random sizes (0 to maxSize bytes) and appends
// the returned pointers to outAddresses. Stops early if the heap runs out of memory.
void AllocateRandomBlocks(HeapAllocator& heap, unsigned int count, size_t maxSize, std::vector<void*>& outAddresses, std::mt19937& rng)
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
unsigned int FreeAllInRandomOrder(HeapAllocator& heap, std::vector<void*>& allocatedAddresses, std::mt19937& rng)
{
	unsigned int freedCount = 0;
	if (!allocatedAddresses.empty())
	{
		// randomize the addresses
		std::shuffle(allocatedAddresses.begin(), allocatedAddresses.end(), rng);

		// return them back to the allocator
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

// [A used][B free][C free]: used to loop forever; B and C should merge
void TestCoalesceMergesAroundUsedBlock(void* memory)
{
	HeapAllocator heap(memory, sizeHeap);
	void* pA = heap.Allocate(100);
	void* pB = heap.Allocate(100);
	void* pC = heap.Allocate(100);
	heap.Free(pB);
	heap.Free(pC);
	heap.Coalesce();
	heap.PrintHeap();
	Check(heap.Allocate(150) == pB, "Free neighbours B and C are merged and reused, with A still in use");
}	// heap is destroyed here, automatically

// [A used] only: used to reset the heap and hand A's memory out again
void TestCoalesceKeepsSingleUsedBlock(void* memory)
{
	HeapAllocator heap(memory, sizeHeap);
	int* pValue = static_cast<int*>(heap.Allocate(sizeof(int)));
	*pValue = 1234;
	heap.Coalesce();
	void* pNext = heap.Allocate(sizeof(int));
	Check(pNext != pValue && *pValue == 1234, "A single block in use survives Coalesce");
}

// Using a brand-new heap: the singleton version read uninitialised memory here
void TestEmptyHeap(void* memory)
{
	HeapAllocator heap(memory, sizeHeap);
	Check(!heap.Free(memory), "Free on an empty heap returns false");
	Check(heap.Allocate(sizeHeap + 1) == nullptr, "An oversized first request returns nullptr");
}

// Different sizes, so a rounding mistake would push a later block off the alignment
void TestPointersAreAligned(void* memory)
{
	HeapAllocator heap(memory, sizeHeap);
	bool allAligned = true;
	for (size_t size : { 1, 7, 15, 16, 17, 100, 1000 })
	{
		allAligned = allAligned && IsAligned(heap.Allocate(size));
	}
	Check(allAligned, "Pointers are aligned for every request size");
}

// A buffer starting 3 bytes past an aligned address: the first bytes must be skipped
void TestMisalignedBuffer(void* memory)
{
	HeapAllocator heap(static_cast<char*>(memory) + 3, sizeHeap - 3);
	Check(IsAligned(heap.Allocate(100)), "A misaligned buffer still gives aligned pointers");
}

// 3 bytes, all of them before the first aligned address: nothing is usable
void TestTinyBuffer(void* memory)
{
	HeapAllocator heap(static_cast<char*>(memory) + 1, 3);
	Check(heap.Allocate(1) == nullptr, "A buffer too small to align has no usable space");
}

// Used to wrap around to a tiny block and return a pointer
void TestHugeRequest(void* memory)
{
	HeapAllocator heap(memory, sizeHeap);
	Check(heap.Allocate(std::numeric_limits<size_t>::max()) == nullptr, "The largest possible request returns nullptr");
}

// Zero bytes still gets its own block, like malloc(0) on most platforms
void TestZeroSizeRequest(void* memory)
{
	HeapAllocator heap(memory, sizeHeap);
	void* pZero = heap.Allocate(0);
	Check(IsAligned(pZero), "Allocate(0) returns an aligned pointer");
	Check(heap.Free(pZero), "The pointer from Allocate(0) can be freed");
}

int main()
{
	// Report memory leaks in the Output window when the program exits (Debug builds only)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// One random engine shared by every test. Replace the seed with a fixed number
	// (e.g. rng(42)) to repeat exactly the same run when debugging a failure.
	std::mt19937 rng(std::random_device{}());
	// allocate the memory the allocator will manage (freed with _aligned_free, not free)
	void * pHeapMemory = _aligned_malloc(sizeHeap, 16);
	assert(pHeapMemory);
	s_heapStart = static_cast<const char*>(pHeapMemory);

	// the heap used by tests 1 to 4
	HeapAllocator heap(pHeapMemory, sizeHeap);

	std::vector<void *> allocatedAddresses;

	////////////////////////////   TEST 1: FIRST HAND ALLOC AND FREE   ////////////////////////////
	// 4 allocations of up to 1023 bytes from fresh memory, then free them all in random order
	printf("=== Test 1: fresh allocations (4 x up to 1023 bytes) ===\n");
	AllocateRandomBlocks(heap, 4, 1023, allocatedAddresses, rng);
	Check(allocatedAddresses.size() == 4, "All 4 allocations succeed");
	heap.PrintHeap();
	Check(FreeAllInRandomOrder(heap, allocatedAddresses, rng) == 4, "All 4 blocks are freed");
	heap.PrintHeap();

	////////////////////////////   TEST 2: SECOND HAND ALLOC AND FREE   ////////////////////////////
	// 6 allocations of up to 4095 bytes; any that fit in a block freed above reuse it
	printf("\n=== Test 2: reuse freed blocks (6 x up to 4095 bytes) ===\n");
	AllocateRandomBlocks(heap, 6, 4095, allocatedAddresses, rng);
	Check(allocatedAddresses.size() == 6, "All 6 allocations succeed"); 
	heap.PrintHeap();
	Check(FreeAllInRandomOrder(heap, allocatedAddresses, rng) == 6, "All 6 blocks are freed");

	////////////////////////////   TEST 3: FREE ONLY FREES ALLOCATED MEMORY   ////////////////////////////
	printf("\n=== Test 3: invalid frees ===\n");
	char* pBlock = static_cast<char*>(heap.Allocate(16));
	Check(pBlock != nullptr, "Allocate a 16-byte block to test with");
	Check(!heap.Free(nullptr), "Free rejects nullptr");
	Check(!heap.Free(pBlock + 1), "Free rejects a pointer into the middle of a block");
	Check(!heap.Free(reinterpret_cast<void *>(static_cast<std::uintptr_t>(rng()))), "Free rejects a random address");
	Check(heap.Free(pBlock), "Free accepts the real block");
	Check(!heap.Free(pBlock), "Free rejects a double free");

	////////////////////////////   TEST 4: COALESCE   ////////////////////////////
	// everything is free, so Coalesce merges it back into one block and resets the heap
	printf("\n=== Test 4: coalesce after freeing everything ===\n");
	printf("  Before Coalesce:\n");
	heap.PrintHeap();
	heap.Coalesce();
	printf("  After Coalesce:\n");
	heap.PrintHeap();
	void * pBig = heap.Allocate(sizeHeap - 8192);
	Check(pBig != nullptr, "A near-full-heap allocation succeeds");
	Check(heap.Free(pBig), "The large block is freed");

	////////////////////////////   TEST 5: COALESCE WITH BLOCKS IN USE   ////////////////////////////
	// Regression tests for the two Coalesce bugs. Each function creates its own fresh heap
	// over the same buffer; the heap from tests 1 to 4 is not used again after this point.
	printf("\n=== Test 5: coalesce with blocks still in use ===\n");
	TestCoalesceMergesAroundUsedBlock(pHeapMemory);
	TestCoalesceKeepsSingleUsedBlock(pHeapMemory);

	////////////////////////////   TEST 6: EMPTY HEAP   ////////////////////////////
	printf("\n=== Test 6: empty heap ===\n");
	TestEmptyHeap(pHeapMemory);

	////////////////////////////   TEST 7: ALIGNMENT   ////////////////////////////
	printf("\n=== Test 7: alignment ===\n");
	TestPointersAreAligned(pHeapMemory);
	TestMisalignedBuffer(pHeapMemory);
	TestTinyBuffer(pHeapMemory);

	////////////////////////////   TEST 8: UNUSUAL REQUEST SIZES   ////////////////////////////
	printf("\n=== Test 8: unusual request sizes ===\n");
	TestHugeRequest(pHeapMemory);
	TestZeroSizeRequest(pHeapMemory);

	////////////////////////////   RESULTS   ////////////////////////////
	printf("\n----------------------------------------\n");
	printf("Result: %u passed, %u failed\n", s_checksPassed, s_checksFailed);

	_aligned_free(pHeapMemory);
	return s_checksFailed == 0 ? 0 : 1;	// non-zero exit code tells scripts / CI that a test failed
}
