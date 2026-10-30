#pragma once
#include <cstdlib>
#include<cstdint>
// Round size up / address down to a multiple of align (align must be a power of 2)
#define ROUND_UP(size,align) ((size + (align - 1)) & (~(align - 1)))
#define ROUND_DOWN(addr,align) (reinterpret_cast<uintptr_t>(addr) & (~(align - 1)))

// A simple first-fit heap allocator that manages a caller-provided block of memory.
// Each allocation is preceded by a BlockHeader, and all blocks form a singly linked
// list in address order. Each object is an independent heap.
class HeapAllocator
{
public:
	// Manages size bytes starting at memory. The caller keeps ownership of the buffer,
	// which must outlive this object and stay aligned to at least 4 bytes.
	HeapAllocator(void* memory, std::size_t size);
	// Does nothing to the buffer, because it belongs to the caller.
	~HeapAllocator() = default;

	// Not copyable: two objects managing one buffer would hand out the same memory twice.
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
	// Stored in the heap directly before each block of user memory
	struct BlockHeader
	{
		unsigned int fullSize;	// header + user data, rounded up to the alignment
		unsigned int userSize;	// bytes requested by the caller
		void* userStart;		// pointer returned to the caller
		BlockHeader* next;		// next block in the list (nullptr for the last block)
		bool isInUse;			// false once the block has been freed
	};

	BlockHeader* firstBlock_ = nullptr;	// first block in the list, or nullptr when the heap is empty
	char* start_;						// first byte of the buffer
	char* unusedStart_;					// start of memory that has never been handed out
	char* end_;							// one past the last byte of the buffer
};
