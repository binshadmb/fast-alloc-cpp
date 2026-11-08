#include "fast_alloc/fixed_block_pool.hpp"

#include <limits>
#include <new>
#include <stdexcept>

namespace fast_alloc {
namespace {
constexpr std::size_t kNoBlock = std::numeric_limits<std::size_t>::max();

bool is_power_of_two(std::size_t value) noexcept {
  return value != 0 && (value & (value - 1)) == 0;
}
} // namespace

std::size_t FixedBlockPool::checked_stride(std::size_t block_size,
                                          std::size_t alignment) {
  if (block_size == 0) {
    throw std::invalid_argument("block_size must be greater than zero");
  }
  if (alignment < alignof(void *) || !is_power_of_two(alignment)) {
    throw std::invalid_argument(
        "alignment must be a power of two and at least alignof(void*)");
  }
  if (block_size > std::numeric_limits<std::size_t>::max() - (alignment - 1)) {
    throw std::length_error("block size and alignment overflow");
  }
  return (block_size + alignment - 1) & ~(alignment - 1);
}

FixedBlockPool::FixedBlockPool(std::size_t block_size, std::size_t capacity,
                               std::size_t alignment)
    : block_size_(block_size), capacity_(capacity), alignment_(alignment),
      stride_(checked_stride(block_size, alignment)), free_head_(0),
      available_(capacity), next_free_(capacity), in_use_(capacity, 0) {
  if (capacity == 0) {
    throw std::invalid_argument("capacity must be greater than zero");
  }
  if (capacity > std::numeric_limits<std::size_t>::max() / stride_) {
    throw std::length_error("pool capacity is too large");
  }

  storage_ = static_cast<std::byte *>(
      ::operator new(stride_ * capacity_, std::align_val_t(alignment_)));
  for (std::size_t i = 0; i < capacity_; ++i) {
    next_free_[i] = (i + 1 < capacity_) ? i + 1 : kNoBlock;
  }
}

FixedBlockPool::~FixedBlockPool() {
  if (storage_ != nullptr) {
    ::operator delete(storage_, std::align_val_t(alignment_));
  }
}

void *FixedBlockPool::allocate() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  if (free_head_ == kNoBlock) {
    return nullptr;
  }

  const std::size_t index = free_head_;
  free_head_ = next_free_[index];
  next_free_[index] = kNoBlock;
  in_use_[index] = 1;
  --available_;
  return storage_ + index * stride_;
}

bool FixedBlockPool::deallocate(void *pointer) noexcept {
  if (pointer == nullptr) {
    return false;
  }

  const auto address = reinterpret_cast<std::uintptr_t>(pointer);
  const auto base = reinterpret_cast<std::uintptr_t>(storage_);
  const std::size_t total_bytes = stride_ * capacity_;
  if (address < base || address - base >= total_bytes) {
    return false;
  }

  const std::size_t offset = static_cast<std::size_t>(address - base);
  if (offset % stride_ != 0) {
    return false;
  }
  const std::size_t index = offset / stride_;

  std::lock_guard<std::mutex> lock(mutex_);
  if (index >= capacity_ || in_use_[index] == 0) {
    return false;
  }

  in_use_[index] = 0;
  next_free_[index] = free_head_;
  free_head_ = index;
  ++available_;
  return true;
}

std::size_t FixedBlockPool::available() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return available_;
}

std::size_t FixedBlockPool::in_use() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return capacity_ - available_;
}

} // namespace fast_alloc
