#include"HeapAllocator.h"
#include<cstdio>

HeapAllocator::HeapAllocator(void* memory, std::size_t size)
	: start_(static_cast<char*>(memory)),
	  unusedStart_(start_),
	  end_(start_ + size)
{
}

// Returns a pointer to chunkSize usable bytes, or nullptr if the heap is full.
// Strategy: reuse the first free block that is big enough (first fit),
// otherwise carve a new block from the unused memory at the end of the heap.
void* HeapAllocator::Allocate(size_t chunkSize)
{
	BlockHeader* currentBlock = firstBlock_;
	BlockHeader* nextBlock = firstBlock_;
	// Header + user data, rounded up to a multiple of 4 bytes
	size_t neededSize = ROUND_UP(sizeof(BlockHeader) + chunkSize, 4);

	// Case 1: the heap is empty, so this becomes the first block in the list
	if (firstBlock_ == nullptr && (end_ - unusedStart_) >= neededSize)
	{
		firstBlock_ = reinterpret_cast<BlockHeader*>(unusedStart_);
		firstBlock_->userSize = chunkSize;
		firstBlock_->fullSize = neededSize;
		firstBlock_->isInUse = true;
		firstBlock_->next = nullptr;
		firstBlock_->userStart = reinterpret_cast<char*>(firstBlock_) + sizeof(BlockHeader);	// user data starts right after the header
		unusedStart_ += firstBlock_->fullSize;
		return firstBlock_->userStart;
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
				currentBlock->userStart = reinterpret_cast<char*>(currentBlock) + sizeof(BlockHeader);
				return currentBlock->userStart;
			}
			nextBlock = currentBlock->next;
		} while (currentBlock->next != nullptr);

		// Case 3: first hand memory - carve a new block from the unused end of the heap
		// and link it after the last block (currentBlock is now the last block in the list).
		if ((end_ - unusedStart_) >= neededSize)
		{
			BlockHeader* newBlock = reinterpret_cast<BlockHeader*>(unusedStart_);
			currentBlock->next = newBlock;
			newBlock->userSize = chunkSize;
			newBlock->fullSize = neededSize;
			newBlock->isInUse = true;
			newBlock->userStart = unusedStart_ + sizeof(BlockHeader);
			newBlock->next = nullptr;
			unusedStart_ += newBlock->fullSize;
			return newBlock->userStart;
		}

		// Out of memory
		return nullptr;
	}
}


// Marks the block owning pPtr as free. Returns false if pPtr is not an allocated block
// (invalid pointer or double free). Free blocks are not merged here; call Coalesce for that.
bool HeapAllocator::Free(void* pPtr)
{
	BlockHeader* currentBlock = firstBlock_;
	BlockHeader* nextBlock = firstBlock_;
	do
	{
		currentBlock = nextBlock;
		// A block's header sits just before its user data, so step back by one header and compare.
		// Only addresses are compared; pPtr itself is never read, so a bad pointer is safe.
		if ((reinterpret_cast<BlockHeader*>(reinterpret_cast<char*>(pPtr) - sizeof(BlockHeader)) == currentBlock) && currentBlock->isInUse)
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
void  HeapAllocator::Coalesce()
{
	BlockHeader* currentBlock = firstBlock_;
	BlockHeader* nextBlock = firstBlock_;
	BlockHeader* adjacentBlock;
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
			adjacentBlock = reinterpret_cast<BlockHeader*>(reinterpret_cast<char*>(currentBlock) + currentBlock->fullSize);
			if (adjacentBlock == currentBlock->next && !currentBlock->isInUse && !currentBlock->next->isInUse)
			{
				// Absorb the neighbour's space and unlink it from the list
				currentBlock->fullSize += currentBlock->next->fullSize;
				currentBlock->next = currentBlock->next->next;
				mergedAny = true;
			}
			nextBlock = currentBlock->next;
		} while (currentBlock->next != nullptr);
		currentBlock = firstBlock_;
		nextBlock = firstBlock_;
	}

	// Only one block left: treat the heap as empty again
	currentBlock = firstBlock_;
	if (currentBlock->next == nullptr && !currentBlock->isInUse)
	{
		firstBlock_ = nullptr;
		unusedStart_ = start_;
	}
}

// Prints the block list as a heap map. Offsets are measured from the start of the heap,
// which is easier to read than raw addresses. Walks the list with a single pointer.
void HeapAllocator::PrintHeap() const
{
	printf("  Heap map:\n");
	if (firstBlock_ == nullptr)
	{
		printf("    (no blocks)\n");
	}
	else
	{
		for (const BlockHeader* block = firstBlock_; block != nullptr; block = block->next)
		{
			size_t offset = static_cast<size_t>(reinterpret_cast<const char*>(block) - start_);
			printf("    offset %8zu   size %7u   %s\n", offset, block->fullSize, block->isInUse ? "used" : "free");
		}
	}
	printf("    unused at end: %zu bytes\n", static_cast<size_t>(end_ - unusedStart_));
}
