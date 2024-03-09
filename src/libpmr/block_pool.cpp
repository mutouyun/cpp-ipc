
#include <mutex>

#include "libimp/detect_plat.h"

#include "libpmr/block_pool.h"
#include "libpmr/monotonic_buffer_resource.h"

LIBPMR_NAMESPACE_BEG_

class thread_safe_resource : public monotonic_buffer_resource {
public:
  thread_safe_resource(::LIBIMP::span<::LIBIMP::byte> buffer) noexcept
      : monotonic_buffer_resource(buffer) {}

  ~thread_safe_resource() noexcept {
    LIBIMP_UNUSED std::lock_guard<std::mutex> lock(mutex_);
    monotonic_buffer_resource::release();
  }

  void *allocate(std::size_t bytes, std::size_t alignment) noexcept {
    LIBIMP_UNUSED std::lock_guard<std::mutex> lock(mutex_);
    return monotonic_buffer_resource::allocate(bytes, alignment);
  }

  void deallocate(void *p, std::size_t bytes, std::size_t alignment) noexcept {
    LIBIMP_UNUSED std::lock_guard<std::mutex> lock(mutex_);
    monotonic_buffer_resource::deallocate(p, bytes, alignment);
  }

private:
  std::mutex mutex_;
};

allocator &central_cache_allocator() noexcept {
  static std::array<::LIBIMP::byte, central_cache_default_size> buf;
  static thread_safe_resource res(buf);
  static allocator a(&res);
  return a;
}

LIBPMR_NAMESPACE_END_
