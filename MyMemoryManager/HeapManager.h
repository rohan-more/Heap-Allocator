#include <cstdlib>
#include<cstdint>
// Round size up / address down to a multiple of align (align must be a power of 2)
#define ROUND_UP(size,align) ((size + (align - 1)) & (~(align - 1)))
#define ROUND_DOWN(addr,align) (reinterpret_cast<uintptr_t>(addr) & (~(align - 1)))

// A simple first-fit heap allocator that manages a caller-provided block of memory.
// Each allocation is preceded by a MemBlockDescriptor header, and all blocks form
// a singly linked list in address order.
class HeapManager
{
public:
	// Creates the heap manager over pHeapMemory (sizeHeap bytes). Singleton: later calls return the same instance.
	static HeapManager * Create(void * pHeapMemory, size_t sizeHeap);
	// Returns a pointer to chunkSize bytes of memory, or nullptr if there is not enough space.
	void * Allocate(size_t chunkSize);
	// Releases memory returned by Allocate. Returns false for invalid pointers or double frees.
	bool Free(void * pPtr);
	// Merges neighbouring free blocks so larger allocations can fit again.
	void Coalesce();

private:
	// Header stored in the heap directly before each block of user memory
	typedef struct MemBlockDescriptor
	{

		unsigned int fullSize;		// header + user data, rounded up to the alignment
		unsigned int userSize;		// bytes requested by the caller
		void * userStart;			// pointer returned to the caller
		//MemBlockDescriptor * current;
		//MemBlockDescriptor * previous;
		MemBlockDescriptor * next;	// next block in the list (nullptr for the last block)
		bool isInUse;				// false once the block has been freed
	}MemBlockDesc;

	char * startOfHeap;
	char * currentPosOfHeap;		// start of memory that has never been handed out
	char * endOfHeap;				// one past the last byte of the heap

	//MemBlockDesc * temp;
	static MemBlockDesc * memoryList;	// first block, or nullptr when the heap is empty
	static MemBlockDesc  head;			// dummy node; head->next points at the first block
	static HeapManager * heapManager;	// the single instance
};
