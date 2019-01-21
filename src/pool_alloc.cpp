#include "pool_alloc.h"

#include "memory/resource.hpp"

namespace ipc {
namespace mem {

void pool_alloc::clear() {
    sync_pool_alloc::clear();
}

void* pool_alloc::alloc(std::size_t size) {
    return sync_pool_alloc::alloc(size);
}

void pool_alloc::free(void* p, std::size_t size) {
    sync_pool_alloc::free(p, size);
}

} // namespace mem
} // namespace ipc
