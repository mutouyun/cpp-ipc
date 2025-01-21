
#include <mutex>
#include <array>
#include <cstddef>

#include "libipc/def.h"
#include "libipc/imp/detect_plat.h"
#include "libipc/imp/byte.h"
#include "libipc/mem/polymorphic_allocator.h"
#include "libipc/mem/memory_resource.h"

namespace ipc {
namespace mem {

class thread_safe_resource : public monotonic_buffer_resource {
public:
  thread_safe_resource(span<byte> buffer) noexcept
      : monotonic_buffer_resource(buffer) {}

  ~thread_safe_resource() noexcept {
    LIBIPC_UNUSED std::lock_guard<std::mutex> lock(mutex_);
    monotonic_buffer_resource::release();
  }

  void *allocate(std::size_t bytes, std::size_t alignment) noexcept {
    LIBIPC_UNUSED std::lock_guard<std::mutex> lock(mutex_);
    return monotonic_buffer_resource::allocate(bytes, alignment);
  }

  void deallocate(void *p, std::size_t bytes, std::size_t alignment) noexcept {
    LIBIPC_UNUSED std::lock_guard<std::mutex> lock(mutex_);
    monotonic_buffer_resource::deallocate(p, bytes, alignment);
  }

private:
  std::mutex mutex_;
};

bytes_allocator &central_cache_allocator() noexcept {
  static std::array<byte, central_cache_default_size> buf;
  static thread_safe_resource res(buf);
  static bytes_allocator a(&res);
  return a;
}

} // namespace mem
} // namespace ipc
