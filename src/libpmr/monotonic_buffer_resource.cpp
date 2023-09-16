
#include "libimp/log.h"

#include "libpmr/monotonic_buffer_resource.h"

LIBPMR_NAMESPACE_BEG_

monotonic_buffer_resource::monotonic_buffer_resource() noexcept {

}

monotonic_buffer_resource::monotonic_buffer_resource(allocator upstream) {

}

monotonic_buffer_resource::monotonic_buffer_resource(std::size_t initial_size) {

}

monotonic_buffer_resource::monotonic_buffer_resource(std::size_t initial_size, allocator upstream) {

}

monotonic_buffer_resource::monotonic_buffer_resource(::LIBIMP::span<::LIBIMP::byte> buffer) noexcept {

}

monotonic_buffer_resource::monotonic_buffer_resource(::LIBIMP::span<::LIBIMP::byte> buffer, allocator upstream) noexcept {

}

monotonic_buffer_resource::~monotonic_buffer_resource() {

}

allocator monotonic_buffer_resource::upstream_resource() const noexcept {

}

void monotonic_buffer_resource::release() {

}

void *monotonic_buffer_resource::allocate(std::size_t bytes, std::size_t alignment) noexcept {

}

void monotonic_buffer_resource::deallocate(void *p, std::size_t bytes, std::size_t alignment) noexcept {

}

LIBPMR_NAMESPACE_END_
