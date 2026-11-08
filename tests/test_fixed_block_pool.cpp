#include "fast_alloc/fixed_block_pool.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

using fast_alloc::FixedBlockPool;

#define CHECK(condition)                                                        \
  do {                                                                          \
    if (!(condition)) {                                                         \
      std::cerr << "CHECK failed at " << __FILE__ << ':' << __LINE__ << ": "  \
                << #condition << '\n';                                        \
      std::terminate();                                                         \
    }                                                                           \
  } while (false)

void test_capacity_and_reuse() {
  FixedBlockPool pool(13, 2);
  void *first = pool.allocate();
  void *second = pool.allocate();
  CHECK(first != nullptr && second != nullptr && first != second);
  CHECK(pool.allocate() == nullptr);
  CHECK(pool.available() == 0);
  CHECK(pool.deallocate(first));
  CHECK(pool.available() == 1);
  CHECK(pool.allocate() == first);
  CHECK(pool.deallocate(first));
  CHECK(pool.deallocate(second));
  CHECK(pool.available() == pool.capacity());
}

void test_rejects_invalid_deallocations() {
  FixedBlockPool pool(32, 1);
  std::array<std::byte, 32> foreign{};
  void *block = pool.allocate();
  CHECK(!pool.deallocate(nullptr));
  CHECK(!pool.deallocate(foreign.data()));
  CHECK(!pool.deallocate(static_cast<std::byte *>(block) + 1));
  CHECK(pool.deallocate(block));
  CHECK(!pool.deallocate(block)); // double free
}

void test_alignment() {
  FixedBlockPool pool(3, 8, 64);
  for (int i = 0; i < 8; ++i) {
    void *block = pool.allocate();
    CHECK(block != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(block) % 64 == 0);
    CHECK(pool.deallocate(block));
  }
}

void test_constructor_validation() {
  bool caught = false;
  try { FixedBlockPool pool(0, 1); }
  catch (const std::invalid_argument &) { caught = true; }
  CHECK(caught);

  caught = false;
  try { FixedBlockPool pool(8, 0); }
  catch (const std::invalid_argument &) { caught = true; }
  CHECK(caught);

  caught = false;
  try { FixedBlockPool pool(8, 1, 3); }
  catch (const std::invalid_argument &) { caught = true; }
  CHECK(caught);
}

void test_concurrent_allocate_deallocate() {
  FixedBlockPool pool(64, 256);
  constexpr int thread_count = 8;
  constexpr int iterations = 5000;
  std::vector<std::thread> workers;
  workers.reserve(thread_count);

  for (int t = 0; t < thread_count; ++t) {
    workers.emplace_back([&pool]() {
      for (int i = 0; i < iterations; ++i) {
        void *block = nullptr;
        while ((block = pool.allocate()) == nullptr) {
          std::this_thread::yield();
        }
        static_cast<std::byte *>(block)[0] = std::byte{0x5A};
        CHECK(pool.deallocate(block));
      }
    });
  }
  for (auto &worker : workers) worker.join();
  CHECK(pool.available() == pool.capacity());
  CHECK(pool.in_use() == 0);
}

int main() {
  test_capacity_and_reuse();
  test_rejects_invalid_deallocations();
  test_alignment();
  test_constructor_validation();
  test_concurrent_allocate_deallocate();
  std::cout << "All FixedBlockPool checks passed.\n";
}
