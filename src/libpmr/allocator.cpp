
#include <algorithm>  // std::swap

#include "libpmr/allocator.h"

LIBPMR_NAMESPACE_BEG_

allocator::holder_mr_base &allocator::get_holder() noexcept {
  return *reinterpret_cast<holder_mr_base *>(holder_.data());
}

allocator::holder_mr_base const &allocator::get_holder() const noexcept {
  return *reinterpret_cast<holder_mr_base const *>(holder_.data());
}

allocator::allocator() noexcept
  : allocator(new_delete_resource::get()) {}

allocator::~allocator() noexcept {
  ::LIBIMP::destroy(&get_holder());
}

void allocator::swap(allocator &other) noexcept {
  std::swap(this->holder_, other.holder_);
}

void *allocator::allocate(std::size_t s, std::size_t a) const {
  return get_holder().alloc(s, a);
}

void allocator::deallocate(void *p, std::size_t s, std::size_t a) const {
  get_holder().dealloc(p, s, a);
}

LIBPMR_NAMESPACE_END_
