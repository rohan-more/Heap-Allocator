#include <cstdlib>
#include<cstdint>
#define ROUND_UP(size,align) ((size + (align - 1)) & (~(align - 1)))
#define ROUND_DOWN(addr,align) (reinterpret_cast<uintptr_t>(addr) & (~(align - 1)))
class HeapManager
{
public:
	static HeapManager * Create(void * pHeapMemory, size_t sizeHeap);
	void * myAlloc(size_t chunkSize);
	bool myFree(void * pPtr);
	void myCollect();

private:
	typedef struct MemBlockDescriptor
	{
		
		unsigned int fullSize;
		unsigned int userSize;
		void * userStart;
		//MemBlockDescriptor * current;
		//MemBlockDescriptor * previous;
		MemBlockDescriptor * next;
		bool isInUse;
	}MemBlockDesc;

	char * startOfHeap;
	char * currentPosOfHeap;
	char * endOfHeap;

	//MemBlockDesc * temp;
	static MemBlockDesc * memoryList;
	static MemBlockDesc * head;
	static HeapManager * heapManager;
};