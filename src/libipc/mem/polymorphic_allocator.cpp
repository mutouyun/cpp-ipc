
#include <algorithm>  // std::swap

#include "libipc/imp/log.h"
#include "libipc/mem/polymorphic_allocator.h"
#include "libipc/mem/memory_resource.h"

namespace ipc {
namespace mem {

bytes_allocator::holder_mr_base &bytes_allocator::get_holder() noexcept {
  return *reinterpret_cast<holder_mr_base *>(holder_.data());
}

bytes_allocator::holder_mr_base const &bytes_allocator::get_holder() const noexcept {
  return *reinterpret_cast<holder_mr_base const *>(holder_.data());
}

void bytes_allocator::init_default_resource() noexcept {
  std::ignore = ipc::construct<holder_mr<new_delete_resource>>(holder_.data(), new_delete_resource::get());
}

bytes_allocator::bytes_allocator() noexcept
  : bytes_allocator(new_delete_resource::get()) {}

bytes_allocator::~bytes_allocator() noexcept {
  ipc::destroy(&get_holder());
}

void bytes_allocator::swap(bytes_allocator &other) noexcept {
  std::swap(this->holder_, other.holder_);
}

void *bytes_allocator::allocate(std::size_t s, std::size_t a) const {
  LIBIPC_LOG();
  if ((a & (a - 1)) != 0) {
    log.error("failed: allocate alignment is not a power of 2.");
    return nullptr;
  }
  return get_holder().alloc(s, a);
}

void bytes_allocator::deallocate(void *p, std::size_t s, std::size_t a) const {
  LIBIPC_LOG();
  if ((a & (a - 1)) != 0) {
    log.error("failed: allocate alignment is not a power of 2.");
    return;
  }
  get_holder().dealloc(p, s, a);
}

} // namespace mem
} // namespace ipc
