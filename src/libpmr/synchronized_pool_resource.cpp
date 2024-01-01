
#include "libimp/log.h"

#include "libpmr/synchronized_pool_resource.h"
#include "libpmr/block_pool.h"

#include "verify_args.h"

LIBPMR_NAMESPACE_BEG_
namespace {

} // namespace

synchronized_pool_resource *synchronized_pool_resource::get() noexcept {
  static synchronized_pool_resource mem_res;
  return &mem_res;
}

void *synchronized_pool_resource::allocate(std::size_t bytes, std::size_t alignment) noexcept {
  LIBIMP_LOG_();
  if (!verify_args(bytes, alignment) || (alignment > alignof(std::max_align_t))) {
    log.error("invalid bytes = ", bytes, ", alignment = ", alignment);
    return nullptr;
  }
  return nullptr;
}

void synchronized_pool_resource::deallocate(void *p, std::size_t bytes, std::size_t alignment) noexcept {
  LIBIMP_LOG_();
  if (p == nullptr) {
    return;
  }
  if (!verify_args(bytes, alignment) || (alignment > alignof(std::max_align_t))) {
    log.error("invalid bytes = ", bytes, ", alignment = ", alignment);
    return;
  }
}

LIBPMR_NAMESPACE_END_
