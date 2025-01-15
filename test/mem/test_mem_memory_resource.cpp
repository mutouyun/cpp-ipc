
#include "test.h"

#include <utility>

#include "libipc/mem/memory_resource.h"

namespace {

template <typename T>
void *test_mr(T &&mr, std::size_t bytes, std::size_t alignment) {
  auto p = std::forward<T>(mr).allocate(bytes, alignment);
  if (alignment == 0) {
    EXPECT_EQ(p, nullptr);
  } else if (p != nullptr) {
    EXPECT_EQ((std::size_t)p % alignment, 0);
  }
  std::forward<T>(mr).deallocate(p, bytes, alignment);
  return p;
}

} // namespace

TEST(memory_resource, new_delete_resource) {
  ipc::mem::new_delete_resource mem_res;

  EXPECT_EQ(test_mr(mem_res, 0, 0), nullptr);
  EXPECT_EQ(test_mr(mem_res, 0, 1), nullptr);
  EXPECT_EQ(test_mr(mem_res, 0, 2), nullptr);
  EXPECT_EQ(test_mr(mem_res, 0, 3), nullptr);
  EXPECT_EQ(test_mr(mem_res, 0, 8), nullptr);
  EXPECT_EQ(test_mr(mem_res, 0, 64), nullptr);

  EXPECT_EQ(test_mr(mem_res, 1, 0), nullptr);
  EXPECT_NE(test_mr(mem_res, 1, 1), nullptr);
  EXPECT_NE(test_mr(mem_res, 1, 2), nullptr);
  EXPECT_EQ(test_mr(mem_res, 1, 3), nullptr);
  EXPECT_NE(test_mr(mem_res, 1, 8), nullptr);
  EXPECT_NE(test_mr(mem_res, 1, 64), nullptr);
}

TEST(memory_resource, monotonic_buffer_resource_construct) {
  { ipc::mem::monotonic_buffer_resource tmp; }
  ipc::mem::monotonic_buffer_resource{};
  ipc::mem::monotonic_buffer_resource{ipc::mem::bytes_allocator{}};
  ipc::mem::monotonic_buffer_resource{0};
  ipc::mem::monotonic_buffer_resource{0, ipc::mem::bytes_allocator{}};
  ipc::mem::monotonic_buffer_resource{ipc::span<ipc::byte>{}};
  ipc::mem::monotonic_buffer_resource{ipc::span<ipc::byte>{}, ipc::mem::bytes_allocator{}};
  SUCCEED();
}

TEST(memory_resource, monotonic_buffer_resource_no_copy) {
  EXPECT_FALSE(std::is_copy_constructible<ipc::mem::monotonic_buffer_resource>::value);
  EXPECT_FALSE(std::is_copy_assignable<ipc::mem::monotonic_buffer_resource>::value);
  EXPECT_FALSE(std::is_move_constructible<ipc::mem::monotonic_buffer_resource>::value);
  EXPECT_FALSE(std::is_move_assignable<ipc::mem::monotonic_buffer_resource>::value);
}

TEST(memory_resource, monotonic_buffer_resource_upstream_resource) {
  struct dummy_allocator {
    bool allocated = false;
    void *allocate(std::size_t, std::size_t) noexcept { allocated = true; return nullptr; }
    void deallocate(void *, std::size_t, std::size_t) noexcept {}
  } dummy;
  ipc::mem::monotonic_buffer_resource tmp{&dummy};
  ASSERT_EQ(tmp.upstream_resource().allocate(1), nullptr);
  ASSERT_TRUE(dummy.allocated);
}

namespace {

struct dummy_allocator {
  std::size_t allocated = 0;
  void *allocate(std::size_t size, std::size_t) noexcept {
    allocated += size;
    return std::malloc(size);
  }
  void deallocate(void *p, std::size_t size, std::size_t) noexcept {
    allocated -= size;
    std::free(p);
  }
};

} // namespace

TEST(memory_resource, monotonic_buffer_resource_allocate) {
  dummy_allocator dummy;
  {
    ipc::mem::monotonic_buffer_resource tmp{&dummy};
    ASSERT_EQ(tmp.allocate(0), nullptr);
    ASSERT_EQ(dummy.allocated, 0);
  }
  ASSERT_EQ(dummy.allocated, 0);
  {
    ipc::mem::monotonic_buffer_resource tmp{&dummy};
    std::size_t sz = 0;
    for (std::size_t i = 1; i < 1024; ++i) {
      ASSERT_NE(tmp.allocate(i), nullptr);
      sz += i;
    }
    for (std::size_t i = 1; i < 1024; ++i) {
      ASSERT_NE(tmp.allocate(1024 - i), nullptr);
      sz += 1024 - i;
    }
    ASSERT_GE(dummy.allocated, sz);
  }
  ASSERT_EQ(dummy.allocated, 0);
}

TEST(memory_resource, monotonic_buffer_resource_allocate_by_buffer) {
  dummy_allocator dummy;
  std::array<ipc::byte, 4096> buffer;
  {
    ipc::mem::monotonic_buffer_resource tmp{buffer, &dummy};
    for (std::size_t i = 1; i < 64; ++i) {
      ASSERT_NE(tmp.allocate(i), nullptr);
    }
    ASSERT_EQ(dummy.allocated, 0);
    std::size_t sz = 0;
    for (std::size_t i = 1; i < 64; ++i) {
      ASSERT_NE(tmp.allocate(64 - i), nullptr);
      sz += 64 - i;
    }
    ASSERT_GT(dummy.allocated, sz);
  }
  ASSERT_EQ(dummy.allocated, 0);
}

TEST(memory_resource, monotonic_buffer_resource_release) {
  dummy_allocator dummy;
  {
    ipc::mem::monotonic_buffer_resource tmp{&dummy};
    tmp.release();
    ASSERT_EQ(dummy.allocated, 0);
    ASSERT_NE(tmp.allocate(1024), nullptr);
    ASSERT_GE(dummy.allocated, 1024u);
    ASSERT_LE(dummy.allocated, 1024u * 1.5);
    tmp.release();
    ASSERT_EQ(dummy.allocated, 0);
    ASSERT_NE(tmp.allocate(1024), nullptr);
    ASSERT_GE(dummy.allocated, 1024u);
    ASSERT_LE(dummy.allocated, 1024u * 1.5);
  }
  ASSERT_EQ(dummy.allocated, 0);
  std::array<ipc::byte, 4096> buffer;
  {
    ipc::mem::monotonic_buffer_resource tmp{buffer, &dummy};
    auto *p = tmp.allocate(1024);
    ASSERT_EQ(p, buffer.data());
    ASSERT_EQ(dummy.allocated, 0);
    p = tmp.allocate(10240);
    ASSERT_NE(p, buffer.data());
    ASSERT_LE(dummy.allocated, 10240u + 1024u);
    tmp.release();
    ASSERT_EQ(dummy.allocated, 0);
    p = tmp.allocate(1024);
    ASSERT_EQ(p, buffer.data());
    ASSERT_EQ(dummy.allocated, 0);
    p = tmp.allocate(10240);
    ASSERT_NE(p, buffer.data());
    ASSERT_LE(dummy.allocated, 10240u + 1024u);
  }
  ASSERT_EQ(dummy.allocated, 0);
}
