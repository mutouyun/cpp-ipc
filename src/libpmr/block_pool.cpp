
#include "libpmr/block_pool.h"
#include "libpmr/monotonic_buffer_resource.h"

LIBPMR_NAMESPACE_BEG_

allocator &central_cache_allocator() noexcept {
  static std::array<::LIBIMP::byte, central_cache_default_size> buffer;
  static monotonic_buffer_resource mr(buffer);
  static allocator a(&mr);
  return a;
}

LIBPMR_NAMESPACE_END_
