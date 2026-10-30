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

The header (`BlockHeader`, 32 bytes in a 64-bit build, 16 in 32-bit) stores the block's full
size, the size the caller asked for, the next block, and whether the block is in use.
Block sizes are rounded up to a multiple of 16 bytes, so user data is always aligned to
16 bytes.

- **Allocate** is first fit: it reuses the first free block that is big enough,
  otherwise it carves a new block from the unused end of the heap.
- **Free** finds the block whose user data starts at the given pointer and marks it
  free. Invalid pointers and double frees are rejected.
- **Coalesce** merges neighbouring free blocks so that large allocations fit again. If
  everything merges into one free block, the heap is reset to empty.

## API

```cpp
HeapAllocator heap(memory, sizeInBytes);   // manage a caller-provided buffer

void* p = heap.Allocate(64);   // nullptr if there is not enough space
heap.Free(p);                  // false for invalid pointers or double frees
heap.Coalesce();               // merge neighbouring free blocks
heap.PrintHeap();              // print a heap map to the console
```

Each object is an independent heap and can't be copied. The buffer still belongs to the
caller and must outlive the object.

## Building and running

Requires Windows and Visual Studio 2026 (C++ desktop workload, v145 toolset).

1. Open `MyMemoryManager.sln`.
2. Select **Debug | x64** (Win32 also works).
3. Run with **Ctrl+F5**, or **F5** to also get the CRT leak report in the Output window.

From a Developer Command Prompt:

```
msbuild MyMemoryManager.sln /p:Configuration=Debug /p:Platform=x64
x64\Debug\MyMemoryManager.exe
```

The program exits with code 0 if every check passes and 1 if any check fails.

## Tests

`Main.cpp` runs eight groups of tests on a 1 MB heap:

1. Fresh allocations, then freeing them in random order
2. Allocations that can reuse the freed blocks
3. Invalid frees: `nullptr`, a pointer into the middle of a block, a random address, a double free
4. `Coalesce` after freeing everything, followed by a near-full-heap allocation
5. `Coalesce` with blocks still in use: regression tests for the two `Coalesce` bugs
6. An empty heap: `Free` returns false, and an oversized first request returns `nullptr`
7. Alignment: every request size, a misaligned buffer, and a buffer too small to align
8. Unusual request sizes: the largest possible `size_t`, and zero bytes

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
    offset        0   size     144   used
    offset      144   size     288   free
    unused at end: 1048144 bytes
  [PASS] Free neighbours B and C are merged and reused, with A still in use
  [PASS] A single block in use survives Coalesce

=== Test 6: empty heap ===
  [PASS] Free on an empty heap returns false
  [PASS] An oversized first request returns nullptr

=== Test 7: alignment ===
  [PASS] Pointers are aligned for every request size
  [PASS] A misaligned buffer still gives aligned pointers
  [PASS] A buffer too small to align has no usable space

=== Test 8: unusual request sizes ===
  [PASS] The largest possible request returns nullptr
  [PASS] Allocate(0) returns an aligned pointer
  [PASS] The pointer from Allocate(0) can be freed

----------------------------------------
Result: 22 passed, 0 failed
```

Allocation sizes in tests 1 and 2 are random, so their heap maps differ from run to run.

## History

| Version | What |
|---|---|
| [`v0-2016-original`](https://github.com/rohan-more/Heap-Allocator/tree/v0-2016-original) | The original 2016 university code, unchanged |
| `main` | The 2026 clean-up: clearer names and comments, two `Coalesce` bugs fixed (an infinite loop, and a heap reset while a block was still in use), two memory leaks fixed, C++17, and a test program with pass/fail checks. Then the rewrite: a regular class instead of a singleton, 16-byte alignment, overflow checks, and x64 builds |

The clean-up was done in small commits, one change each, so `git log` reads as a
before-and-after.

## Known limitations

These are planned next:

- Free blocks are reused whole instead of being split, which wastes space
- `Free` walks the whole list, so it gets slower as the heap fills up
- Free blocks are only merged when `Coalesce` is called explicitly

## Project layout

```
MyMemoryManager.sln
MyMemoryManager/
  HeapAllocator.h / .cpp      the allocator
  Main.cpp                    test program
```

## License

MIT, see [LICENSE](LICENSE). Older commits, including `v0-2016-original`, also contain
engine helper files (`Assert.*`, `ConsolePrint.*`, `Target*.h`) that were provided as
part of the university course. Those files are not covered by this license.
