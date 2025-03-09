#include "libipc/pool_alloc.h"

#include "libipc/mem/resource.h"

namespace ipc {
namespace mem {

void* pool_alloc::alloc(std::size_t size) noexcept {
    return async_pool_alloc::alloc(size);
}

void pool_alloc::free(void* p, std::size_t size) noexcept {
    async_pool_alloc::free(p, size);
}

} // namespace mem
} // namespace ipc
