
#include <algorithm>  // std::swap

#include "libipc/imp/log.h"
#include "libipc/mem/allocator.h"

namespace ipc {
namespace mem {

allocator::holder_mr_base &allocator::get_holder() noexcept {
  return *reinterpret_cast<holder_mr_base *>(holder_.data());
}

allocator::holder_mr_base const &allocator::get_holder() const noexcept {
  return *reinterpret_cast<holder_mr_base const *>(holder_.data());
}

allocator::allocator() noexcept
  : allocator(new_delete_resource::get()) {}

allocator::~allocator() noexcept {
  ipc::destroy(&get_holder());
}

void allocator::swap(allocator &other) noexcept {
  std::swap(this->holder_, other.holder_);
}

void *allocator::allocate(std::size_t s, std::size_t a) const {
  LIBIPC_LOG();
  if ((a & (a - 1)) != 0) {
    log.error("failed: allocate alignment is not a power of 2.");
    return nullptr;
  }
  return get_holder().alloc(s, a);
}

void allocator::deallocate(void *p, std::size_t s, std::size_t a) const {
  LIBIPC_LOG();
  if ((a & (a - 1)) != 0) {
    log.error("failed: allocate alignment is not a power of 2.");
    return;
  }
  get_holder().dealloc(p, s, a);
}

} // namespace mem
} // namespace ipc
