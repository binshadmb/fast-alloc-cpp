# fast-alloc-cpp

A small C++17 fixed-block memory pool for bounded workloads. It preallocates aligned storage, reuses blocks through an index-based free list, and protects allocation metadata with a mutex. It uses only the C++ standard library.

## Design and complexity

- Pool construction: **O(capacity)** time and memory.
- `allocate()` and `deallocate()`: **O(1)** metadata work, plus mutex contention.
- Storage is bounded; `allocate()` returns `nullptr` when all blocks are in use.
- Invalid, foreign, misaligned, and double-free pointers are rejected.
- The pool manages raw storage. Callers construct and destroy objects themselves.

The mutex makes operations thread-safe, but this is not a lock-free allocator. Fixed-size blocks avoid external fragmentation within the pool; unused capacity and per-block padding still consume memory.

## Build and test

Requires CMake 3.16+ and a C++17 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Benchmark

```sh
./build/benchmark_pool
```

The benchmark compares a batch of pool allocations/frees with `operator new`/`operator delete`. It reports measurements from the machine where it runs; no performance result is claimed in advance. For meaningful comparisons, record compiler, flags, CPU, operating system, and repeated-run statistics.

## Scope

This is a learning and evaluation project, not a replacement for a production allocator. It does not call object constructors, implement lock-free allocation, or guarantee a latency bound under mutex contention.
