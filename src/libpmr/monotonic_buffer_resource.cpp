
#include <utility>
#include <memory>
#include <algorithm>

#include "libimp/log.h"
#include "libimp/aligned.h"

#include "libpmr/monotonic_buffer_resource.h"

LIBPMR_NAMESPACE_BEG_
namespace {

template <typename Node>
Node *make_node(allocator const &upstream, std::size_t initial_size, std::size_t alignment) {
  LIBIMP_LOG_();
  auto sz = ::LIBIMP::round_up(sizeof(Node), alignment) + initial_size;
  auto *node = static_cast<Node *>(upstream.allocate(sz));
  if (node == nullptr) {
    log.error("failed: allocate memory for `monotonic_buffer_resource`.");
    return nullptr;
  }
  node->next = nullptr;
  node->size = sz;
  return node;
}

std::size_t next_buffer_size(std::size_t size) noexcept {
  return size * 3 / 2;
}

} // namespace

monotonic_buffer_resource::monotonic_buffer_resource() noexcept
  : monotonic_buffer_resource(allocator{}) {}

monotonic_buffer_resource::monotonic_buffer_resource(allocator upstream) noexcept
  : monotonic_buffer_resource(0, std::move(upstream)) {}

monotonic_buffer_resource::monotonic_buffer_resource(std::size_t initial_size)
  : monotonic_buffer_resource(initial_size, allocator{}) {}

monotonic_buffer_resource::monotonic_buffer_resource(std::size_t initial_size, allocator upstream)
  : upstream_      (std::move(upstream))
  , free_list_     (nullptr)
  , head_          (nullptr)
  , tail_          (nullptr)
  , next_size_     (initial_size)
  , initial_buffer_(nullptr)
  , initial_size_  (initial_size) {}

monotonic_buffer_resource::monotonic_buffer_resource(::LIBIMP::span<::LIBIMP::byte> buffer) noexcept
  : monotonic_buffer_resource(buffer, allocator{}) {}

monotonic_buffer_resource::monotonic_buffer_resource(::LIBIMP::span<::LIBIMP::byte> buffer, allocator upstream) noexcept
  : upstream_      (std::move(upstream))
  , free_list_     (nullptr)
  , head_          (buffer.begin())
  , tail_          (buffer.end())
  , next_size_     (next_buffer_size(buffer.size()))
  , initial_buffer_(buffer.begin())
  , initial_size_  (buffer.size()) {}

monotonic_buffer_resource::~monotonic_buffer_resource() {
  release();
}

allocator monotonic_buffer_resource::upstream_resource() const noexcept {
  return upstream_;
}

void monotonic_buffer_resource::release() {
  while (free_list_ != nullptr) {
    auto *next = free_list_->next;
    upstream_.deallocate(free_list_, free_list_->size);
    free_list_ = next;
  }
  // reset to initial state at contruction
  if ((head_ = initial_buffer_) != nullptr) {
    tail_ = head_ + initial_size_;
    next_size_ = next_buffer_size(initial_size_);
  } else {
    tail_ = nullptr;
    next_size_ = initial_size_;
  }
}

void *monotonic_buffer_resource::allocate(std::size_t bytes, std::size_t alignment) noexcept {
  LIBIMP_LOG_();
  if (bytes == 0) {
    log.error("failed: allocate bytes = 0.");
    return nullptr;
  }
  if ((alignment & (alignment - 1)) != 0) {
    log.error("failed: allocate alignment is not a power of 2.");
    return nullptr;
  }
  void *p = head_;
  auto  s = static_cast<std::size_t>(tail_ - head_);
  if (std::align(alignment, bytes, p, s) == nullptr) {
    next_size_ = (std::max)(next_size_, bytes);
    auto *node = make_node<monotonic_buffer_resource::node>(upstream_, next_size_, alignment);
    if (node == nullptr) return nullptr;
    node->next = free_list_;
    free_list_ = node;
    next_size_ = next_buffer_size(next_size_);
    p = reinterpret_cast<::LIBIMP::byte *>(free_list_) + ::LIBIMP::round_up(sizeof(node), alignment);
  }
  head_ = static_cast<::LIBIMP::byte *>(p) + bytes;
  return p;
}

void monotonic_buffer_resource::deallocate(void *p, std::size_t bytes, std::size_t alignment) noexcept {
  static_cast<void>(p);
  static_cast<void>(bytes);
  static_cast<void>(alignment);
  // Do nothing.
}

LIBPMR_NAMESPACE_END_
