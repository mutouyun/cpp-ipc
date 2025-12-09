
#include <utility>
#include <memory>
#include <algorithm>

#include "libipc/imp/log.h"
#include "libipc/imp/aligned.h"
#include "libipc/imp/detect_plat.h"
#include "libipc/mem/memory_resource.h"

namespace ipc {
namespace mem {
namespace {

template <typename Node>
Node *make_node(bytes_allocator const &upstream, std::size_t initial_size, std::size_t alignment) noexcept {
  LIBIPC_LOG();
  auto sz = ipc::round_up(sizeof(Node), alignment) + initial_size;
  LIBIPC_TRY {
    auto *node = static_cast<Node *>(upstream.allocate(sz));
    if (node == nullptr) {
      log.error("failed: allocate memory for `monotonic_buffer_resource`'s node.", 
                " bytes = ", initial_size, ", alignment = ", alignment);
      return nullptr;
    }
    node->next = nullptr;
    node->size = sz;
    return node;
  } LIBIPC_CATCH(...) {
    log.error("failed: allocate memory for `monotonic_buffer_resource`'s node.", 
              " bytes = ", initial_size, ", alignment = ", alignment,
              "\n\texception: ", ipc::log::exception_string(std::current_exception()));
    return nullptr;
  }
}

std::size_t next_buffer_size(std::size_t size) noexcept {
  return size * 3 / 2;
}

} // namespace

monotonic_buffer_resource::monotonic_buffer_resource() noexcept
  : monotonic_buffer_resource(bytes_allocator{}) {}

monotonic_buffer_resource::monotonic_buffer_resource(bytes_allocator upstream) noexcept
  : monotonic_buffer_resource(0, std::move(upstream)) {}

monotonic_buffer_resource::monotonic_buffer_resource(std::size_t initial_size) noexcept
  : monotonic_buffer_resource(initial_size, bytes_allocator{}) {}

monotonic_buffer_resource::monotonic_buffer_resource(std::size_t initial_size, bytes_allocator upstream) noexcept
  : upstream_      (std::move(upstream))
  , free_list_     (nullptr)
  , head_          (nullptr)
  , tail_          (nullptr)
  , next_size_     (initial_size)
  , initial_buffer_(nullptr)
  , initial_size_  (initial_size) {}

monotonic_buffer_resource::monotonic_buffer_resource(ipc::span<ipc::byte> buffer) noexcept
  : monotonic_buffer_resource(buffer, bytes_allocator{}) {}

monotonic_buffer_resource::monotonic_buffer_resource(ipc::span<ipc::byte> buffer, bytes_allocator upstream) noexcept
  : upstream_      (std::move(upstream))
  , free_list_     (nullptr)
  , head_          (buffer.begin())
  , tail_          (buffer.end())
  , next_size_     (next_buffer_size(buffer.size()))
  , initial_buffer_(buffer.begin())
  , initial_size_  (buffer.size()) {}

monotonic_buffer_resource::~monotonic_buffer_resource() noexcept {
  release();
}

bytes_allocator monotonic_buffer_resource::upstream_resource() const noexcept {
  return upstream_;
}

void monotonic_buffer_resource::release() noexcept {
  LIBIPC_LOG();
  LIBIPC_TRY {
    while (free_list_ != nullptr) {
      auto *next = free_list_->next;
      upstream_.deallocate(free_list_, free_list_->size);
      free_list_ = next;
    }
  } LIBIPC_CATCH(...) {
    log.error("failed: deallocate memory for `monotonic_buffer_resource`.",
              "\n\texception: ", ipc::log::exception_string(std::current_exception()));
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
  LIBIPC_LOG();
  if (bytes == 0) {
    log.error("failed: allocate bytes = 0.");
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
    // try again
    s = node->size - sizeof(monotonic_buffer_resource::node);
    p = std::align(alignment, bytes, (p = node + 1), s);
    if (p == nullptr) {
      log.error("failed: allocate memory for `monotonic_buffer_resource`.", 
                " bytes = ", bytes, ", alignment = ", alignment);
      return nullptr;
    }
    tail_ = static_cast<ipc::byte *>(p) + s;
  }
  head_ = static_cast<ipc::byte *>(p) + bytes;
  return p;
}

void monotonic_buffer_resource::deallocate(void *p, std::size_t bytes, std::size_t alignment) noexcept {
  static_cast<void>(p);
  static_cast<void>(bytes);
  static_cast<void>(alignment);
  // Do nothing.
}

} // namespace mem
} // namespace ipc
