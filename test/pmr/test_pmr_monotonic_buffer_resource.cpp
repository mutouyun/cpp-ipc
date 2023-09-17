
#include <type_traits>
#include <array>
#include <cstdlib>

#include "gtest/gtest.h"

#include "libpmr/monotonic_buffer_resource.h"

TEST(monotonic_buffer_resource, construct) {
  { pmr::monotonic_buffer_resource tmp; }
  pmr::monotonic_buffer_resource{};
  pmr::monotonic_buffer_resource{pmr::allocator{}};
  pmr::monotonic_buffer_resource{0};
  pmr::monotonic_buffer_resource{0, pmr::allocator{}};
  pmr::monotonic_buffer_resource{imp::span<imp::byte>{}};
  pmr::monotonic_buffer_resource{imp::span<imp::byte>{}, pmr::allocator{}};
  SUCCEED();
}

TEST(monotonic_buffer_resource, no_copy) {
  EXPECT_FALSE(std::is_copy_constructible<pmr::monotonic_buffer_resource>::value);
  EXPECT_FALSE(std::is_copy_assignable<pmr::monotonic_buffer_resource>::value);
  EXPECT_FALSE(std::is_move_constructible<pmr::monotonic_buffer_resource>::value);
  EXPECT_FALSE(std::is_move_assignable<pmr::monotonic_buffer_resource>::value);
}

TEST(monotonic_buffer_resource, upstream_resource) {
  struct dummy_allocator {
    bool allocated = false;
    void *allocate(std::size_t, std::size_t) noexcept { allocated = true; return nullptr; }
    void deallocate(void *, std::size_t, std::size_t) noexcept {}
  } dummy;
  pmr::monotonic_buffer_resource tmp{&dummy};
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

TEST(monotonic_buffer_resource, allocate) {
  dummy_allocator dummy;
  {
    pmr::monotonic_buffer_resource tmp{&dummy};
    ASSERT_EQ(tmp.allocate(0), nullptr);
    ASSERT_EQ(dummy.allocated, 0);
  }
  ASSERT_EQ(dummy.allocated, 0);
  {
    pmr::monotonic_buffer_resource tmp{&dummy};
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

TEST(monotonic_buffer_resource, allocate_by_buffer) {
  dummy_allocator dummy;
  std::array<imp::byte, 4096> buffer;
  {
    pmr::monotonic_buffer_resource tmp{buffer, &dummy};
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

TEST(monotonic_buffer_resource, release) {
  dummy_allocator dummy;
  {
    pmr::monotonic_buffer_resource tmp{&dummy};
    tmp.release();
    ASSERT_EQ(dummy.allocated, 0);
    ASSERT_NE(tmp.allocate(1024), nullptr);
    ASSERT_GE(dummy.allocated, 1024);
    ASSERT_LE(dummy.allocated, 1024 * 1.5);
    tmp.release();
    ASSERT_EQ(dummy.allocated, 0);
    ASSERT_NE(tmp.allocate(1024), nullptr);
    ASSERT_GE(dummy.allocated, 1024);
    ASSERT_LE(dummy.allocated, 1024 * 1.5);
  }
  ASSERT_EQ(dummy.allocated, 0);
  std::array<imp::byte, 4096> buffer;
  {
    pmr::monotonic_buffer_resource tmp{buffer, &dummy};
    auto *p = tmp.allocate(1024);
    ASSERT_EQ(p, buffer.data());
    ASSERT_EQ(dummy.allocated, 0);
    p = tmp.allocate(10240);
    ASSERT_NE(p, buffer.data());
    ASSERT_LE(dummy.allocated, 10240 + 1024);
    tmp.release();
    ASSERT_EQ(dummy.allocated, 0);
    p = tmp.allocate(1024);
    ASSERT_EQ(p, buffer.data());
    ASSERT_EQ(dummy.allocated, 0);
    p = tmp.allocate(10240);
    ASSERT_NE(p, buffer.data());
    ASSERT_LE(dummy.allocated, 10240 + 1024);
  }
  ASSERT_EQ(dummy.allocated, 0);
}
