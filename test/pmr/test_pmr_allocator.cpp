
#include <vector>
#if defined(LIBIMP_CPP_17) && defined(__cpp_lib_memory_resource)
#include <memory_resource>
#endif

#include "gtest/gtest.h"

#include "libpmr/allocator.h"

TEST(allocator, detail) {
  EXPECT_FALSE(pmr::detail::has_allocate<void>::value);
  EXPECT_FALSE(pmr::detail::has_allocate<int>::value);
  EXPECT_FALSE(pmr::detail::has_allocate<std::vector<int>>::value);
  EXPECT_TRUE (pmr::detail::has_allocate<std::allocator<int>>::value);
#if defined(LIBIMP_CPP_17) && defined(__cpp_lib_memory_resource)
  EXPECT_TRUE (pmr::detail::has_allocate<std::pmr::memory_resource>::value);
  EXPECT_TRUE (pmr::detail::has_allocate<std::pmr::polymorphic_allocator<int>>::value);
#endif
}

TEST(allocator, construct) {
}