#pragma once
#include <cstdlib>
#include<cstdint>
// Round size up / address down to a multiple of align (align must be a power of 2)
#define ROUND_UP(size,align) ((size + (align - 1)) & (~(align - 1)))
#define ROUND_DOWN(addr,align) (reinterpret_cast<uintptr_t>(addr) & (~(align - 1)))

class HeapAllocator
{
public:
	HeapAllocator(void* memory, std::size_t size);
	~HeapAllocator() = default;

	HeapAllocator(const HeapAllocator&) = delete;
	HeapAllocator& operator=(const HeapAllocator&) = delete;

	// Returns a pointer to chunkSize bytes of memory, or nullptr if there is not enough space.
	void* Allocate(size_t chunkSize);
	// Releases memory returned by Allocate. Returns false for invalid pointers or double frees.
	bool Free(void* pPtr);

	// Merges neighbouring free blocks so larger allocations can fit again.
	void Coalesce();

	// Prints every block (offset from the start of the heap, size, used/free) and the unused space at the end.
	void PrintHeap() const;

private:
	struct BlockHeader
	{
		unsigned int fullSize;
		unsigned int userSize;
		void* userStart;
		BlockHeader* next;
		bool isInUse;
	};

	BlockHeader* firstBlock_ = nullptr;
	char* start_;
	char* unusedStart_;
	char* end_;
};
