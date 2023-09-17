
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
