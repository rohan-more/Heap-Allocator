#pragma once
#include <cstdlib>
#include<cstdint>
#include <cstddef>
// Rounds value up to a multiple of alignment (must be a power of 2)
constexpr std::size_t AlignUp(std::size_t value, std::size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

// A simple first-fit heap allocator that manages a caller-provided block of memory.
// Each allocation is preceded by a BlockHeader, and all blocks form a singly linked
// list in address order. Each object is an independent heap.
class HeapAllocator
{
public:
	// Manages size bytes starting at memory. The caller keeps ownership of the buffer,
	// which must outlive this object.
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

	static constexpr std::size_t kAlignment = 16;

	struct alignas(kAlignment) BlockHeader
	{
		std::size_t fullSize;	// header + user data, rounded up to the alignment
		std::size_t userSize;	// bytes requested by the caller
		BlockHeader* next;		// next block in the list (nullptr for the last block)
		bool isInUse;			// false once the block has been freed
	};
	static_assert(sizeof(BlockHeader) % kAlignment == 0, "User data must start on an aligned address");

	// User data starts right after the header
	static void* UserData(BlockHeader* block) { return block + 1; }
	// True if size bytes are left in the unused part of the heap
	bool CanFit(std::size_t size) const { return size <= static_cast<std::size_t>(end_ - unusedStart_); }

	BlockHeader* firstBlock_ = nullptr;	// first block in the list, or nullptr when the heap is empty
	std::byte* start_;						// first aligned byte of the buffer
	std::byte * unusedStart_;					// start of memory that has never been handed out
	std::byte * end_;							// one past the last byte of the buffer
};
