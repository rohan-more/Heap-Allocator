# Heap Allocator

A first-fit heap allocator in C++. You give it one block of memory, and it hands out
pieces of it through a `malloc`/`free`-style API, with a test program that prints a
heap map and runs pass/fail checks.

It started as a university project in 2016 and was cleaned up, debugged and tested in
2026. The write-up is here:
[Memory Block Allocator, ten years later](https://iamrohanmore.wordpress.com/2026/10/21/memory-block-allocator-ten-years-later/).

## How it works

The allocator manages a buffer that the caller provides. Every allocation is preceded
by a small header, and all blocks form a singly linked list in address order:

```
 [ header | user data ][ header | user data ][ header | user data ][ unused ......... ]
     │                     ▲    │                ▲
     └──── next ───────────┘    └──── next ──────┘
```

The header (`MemBlockDescriptor`, 20 bytes in a 32-bit build) stores the block's full
size, the size the caller asked for, the pointer handed to the caller, the next block,
and whether the block is in use. Block sizes are rounded up to a multiple of 4 bytes.

- **Allocate** is first fit: it reuses the first free block that is big enough,
  otherwise it carves a new block from the unused end of the heap.
- **Free** finds the block whose user data starts at the given pointer and marks it
  free. Invalid pointers and double frees are rejected.
- **Coalesce** merges neighbouring free blocks so that large allocations fit again. If
  everything merges into one free block, the heap is reset to empty.

## API

```cpp
HeapManager* heap = HeapManager::Create(memory, sizeInBytes);   // manage a caller-provided buffer

void* p = heap->Allocate(64);   // nullptr if there is not enough space
heap->Free(p);                  // false for invalid pointers or double frees
heap->Coalesce();               // merge neighbouring free blocks
heap->PrintHeap();              // print a heap map to the console

HeapManager::Destroy();         // before freeing the buffer
```

## Building and running

Requires Windows and Visual Studio 2026 (C++ desktop workload, v145 toolset).

1. Open `MyMemoryManager.sln`.
2. Select **Debug | Win32**.
3. Run with **Ctrl+F5**, or **F5** to also get the CRT leak report in the Output window.

From a Developer Command Prompt:

```
msbuild MyMemoryManager.sln /p:Configuration=Debug /p:Platform=Win32
Debug\MyMemoryManager.exe
```

The program exits with code 0 if every check passes and 1 if any check fails.

## Tests

`Main.cpp` runs five groups of tests on a 1 MB heap:

1. Fresh allocations, then freeing them in random order
2. Allocations that can reuse the freed blocks
3. Invalid frees: `nullptr`, a pointer into the middle of a block, a random address, a double free
4. `Coalesce` after freeing everything, followed by a near-full-heap allocation
5. Regression tests for two `Coalesce` bugs, each on a fresh heap

Sample output:

```
=== Test 3: invalid frees ===
  [PASS] Allocate a 16-byte block to test with
  [PASS] Free rejects nullptr
  [PASS] Free rejects a pointer into the middle of a block
  [PASS] Free rejects a random address
  [PASS] Free accepts the real block
  [PASS] Free rejects a double free
...
=== Test 5: coalesce with blocks still in use ===
  Heap map:
    offset        0   size     120   used
    offset      120   size     240   free
    unused at end: 1048216 bytes
  [PASS] Free neighbours B and C are merged and reused, with A still in use
  [PASS] A single block in use survives Coalesce

----------------------------------------
Result: 14 passed, 0 failed
```

Allocation sizes in tests 1 and 2 are random, so their heap maps differ from run to run.

## History

| Version | What |
|---|---|
| [`v0-2016-original`](https://github.com/rohan-more/Heap-Allocator/tree/v0-2016-original) | The original 2016 university code, unchanged |
| `main` | The 2026 clean-up: clearer names and comments, two `Coalesce` bugs fixed (an infinite loop, and a heap reset while a block was still in use), two memory leaks fixed, C++17, and a test program with pass/fail checks |

The clean-up was done in small commits, one change each, so `git log` reads as a
before-and-after.

## Known limitations

These are planned for the rewrite on the `rewrite/modern-allocator` branch:

- A singleton with static state, so only one heap can exist at a time
- 32-bit build only, and blocks are aligned to 4 bytes rather than 16
- Free blocks are reused whole instead of being split, which wastes space
- `Free` walks the whole list, so it gets slower as the heap fills up
- Free blocks are only merged when `Coalesce` is called explicitly

## Project layout

```
MyMemoryManager.sln
MyMemoryManager/
  HeapManager.h / .cpp        the allocator
  Main.cpp                    test program
  Assert.*, ConsolePrint.*,   engine helper files provided as part of the
  Target*.h                   university course (see License)
```

## License

MIT, see [LICENSE](LICENSE). The engine helper files (`Assert.h`, `Assert.Win32.cpp`,
`ConsolePrint.h`, `ConsolePrint.Win32.cpp`, `Target.h`, `Target.Win32.h`) were provided
as part of the university course and are not covered by this license.
