#include "pool_alloc.h"

#include "memory/resource.hpp"

namespace ipc {
namespace mem {

void pool_alloc::clear() {
    detail::pool_alloc::clear();
}

void* pool_alloc::alloc(std::size_t size) {
    return detail::pool_alloc::alloc(size);
}

void pool_alloc::free(void* p, std::size_t size) {
    detail::pool_alloc::free(p, size);
}

} // namespace mem
} // namespace ipc
