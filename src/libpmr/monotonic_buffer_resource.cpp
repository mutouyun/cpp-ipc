
#include <utility>

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

} // namespace

monotonic_buffer_resource::monotonic_buffer_resource() noexcept
  : monotonic_buffer_resource(allocator{}) {}

monotonic_buffer_resource::monotonic_buffer_resource(allocator upstream) noexcept
  : upstream_ (std::move(upstream))
  , free_list_(nullptr)
  , head_     (nullptr)
  , tail_     (nullptr)
  , initial_size_(0) {}

monotonic_buffer_resource::monotonic_buffer_resource(std::size_t initial_size)
  : monotonic_buffer_resource(initial_size, allocator{}) {}

monotonic_buffer_resource::monotonic_buffer_resource(std::size_t initial_size, allocator upstream)
  : upstream_ (std::move(upstream))
  , free_list_(make_node<node>(upstream_, initial_size, alignof(std::max_align_t)))
  , head_     (free_list_ == nullptr ? reinterpret_cast<::LIBIMP::byte *>(free_list_) + sizeof(node) : nullptr)
  , tail_     (head_ + initial_size)
  , initial_size_(initial_size) {}

monotonic_buffer_resource::monotonic_buffer_resource(::LIBIMP::span<::LIBIMP::byte> buffer) noexcept
  : monotonic_buffer_resource(buffer, allocator{}) {}

monotonic_buffer_resource::monotonic_buffer_resource(::LIBIMP::span<::LIBIMP::byte> buffer, allocator upstream) noexcept
  : upstream_ (std::move(upstream))
  , free_list_(nullptr)
  , head_     (buffer.begin())
  , tail_     (buffer.end())
  , initial_buffer_(buffer)
  , initial_size_(initial_buffer_.size()) {}

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
  head_ = initial_buffer_.begin();
  tail_ = initial_buffer_.end();
}

void *monotonic_buffer_resource::allocate(std::size_t bytes, std::size_t alignment) noexcept {
  LIBIMP_LOG_();
  if (bytes == 0) {
    log.error("failed: allocate bytes = 0.");
    return nullptr;
  }
  if ((alignment & (alignment - 1)) != 0) {
    log.error("failed: allocate alignment is not power of 2.");
    return nullptr;
  }
  auto *p = reinterpret_cast<::LIBIMP::byte *>(::LIBIMP::round_up(reinterpret_cast<std::size_t>(head_), alignment));
  auto remain = static_cast<std::size_t>(tail_ - p);
  if (remain < bytes) {
    initial_size_ = (std::max)(initial_size_ * 2, bytes);
    auto *node = make_node<monotonic_buffer_resource::node>(upstream_, initial_size_, alignment);
    if (node == nullptr) return nullptr;
    node->next = free_list_;
    free_list_ = node;
    p = reinterpret_cast<::LIBIMP::byte *>(free_list_) + ::LIBIMP::round_up(sizeof(node), alignment);
  }
  return p;
}

void monotonic_buffer_resource::deallocate(void *p, std::size_t bytes, std::size_t alignment) noexcept {
  static_cast<void>(p);
  static_cast<void>(bytes);
  static_cast<void>(alignment);
  // Do nothing.
}

LIBPMR_NAMESPACE_END_
