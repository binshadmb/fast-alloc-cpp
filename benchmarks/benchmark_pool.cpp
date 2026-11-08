#include "fast_alloc/fixed_block_pool.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

using Clock = std::chrono::steady_clock;

template <typename Operation>
double measure_ms(Operation operation) {
  const auto start = Clock::now();
  operation();
  const auto end = Clock::now();
  return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
  constexpr std::size_t iterations = 500000;
  constexpr std::size_t block_size = 64;
  fast_alloc::FixedBlockPool pool(block_size, iterations);

  const double pool_ms = measure_ms([&]() {
    std::vector<void *> blocks;
    blocks.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
      void *block = pool.allocate();
      if (block == nullptr) std::abort();
      blocks.push_back(block);
    }
    for (void *block : blocks) {
      if (!pool.deallocate(block)) std::abort();
    }
  });

  const double heap_ms = measure_ms([&]() {
    std::vector<void *> blocks;
    blocks.reserve(iterations);
    for (std::size_t i = 0; i < iterations; ++i) {
      blocks.push_back(::operator new(block_size));
    }
    for (void *block : blocks) ::operator delete(block);
  });

  std::cout << "iterations=" << iterations << '\n'
            << "fixed_pool_allocate_and_free_ms=" << pool_ms << '\n'
            << "operator_new_delete_ms=" << heap_ms << '\n'
            << "Note: timings depend on compiler, allocator, and machine.\n";
}
