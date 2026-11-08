#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace fast_alloc {

// A bounded, thread-safe pool for fixed-size raw storage blocks.
// Object construction and destruction remain the caller's responsibility.
class FixedBlockPool final {
public:
  FixedBlockPool(std::size_t block_size, std::size_t capacity,
                 std::size_t alignment = alignof(std::max_align_t));
  ~FixedBlockPool();

  FixedBlockPool(const FixedBlockPool &) = delete;
  FixedBlockPool &operator=(const FixedBlockPool &) = delete;
  FixedBlockPool(FixedBlockPool &&) = delete;
  FixedBlockPool &operator=(FixedBlockPool &&) = delete;

  // Returns nullptr when the bounded pool is exhausted.
  void *allocate() noexcept;

  // Returns false for null, foreign, misaligned, or already released pointers.
  bool deallocate(void *pointer) noexcept;

  std::size_t capacity() const noexcept { return capacity_; }
  std::size_t block_size() const noexcept { return block_size_; }
  std::size_t stride() const noexcept { return stride_; }
  std::size_t available() const noexcept;
  std::size_t in_use() const noexcept;

private:
  static std::size_t checked_stride(std::size_t block_size,
                                    std::size_t alignment);

  std::byte *storage_ = nullptr;
  std::size_t block_size_;
  std::size_t capacity_;
  std::size_t alignment_;
  std::size_t stride_;
  std::size_t free_head_;
  std::size_t available_;
  std::vector<std::size_t> next_free_;
  std::vector<std::uint8_t> in_use_;
  mutable std::mutex mutex_;
};

} // namespace fast_alloc
