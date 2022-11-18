
#include <utility>
#if defined(LIBIMP_CPP_17) && defined(__cpp_lib_memory_resource)
#include <memory_resource>
#endif

#include "gtest/gtest.h"

#include "libpmr/memory_resource.h"

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

TEST(memory_resource, traits) {
  EXPECT_FALSE(pmr::has_allocate<void>::value);
  EXPECT_FALSE(pmr::has_allocate<int>::value);
  EXPECT_FALSE(pmr::has_allocate<std::vector<int>>::value);
  EXPECT_TRUE (pmr::has_allocate<std::allocator<int>>::value);
#if defined(LIBIMP_CPP_17) && defined(__cpp_lib_memory_resource)
  EXPECT_TRUE (pmr::has_allocate<std::pmr::memory_resource>::value);
  EXPECT_TRUE (pmr::has_allocate<std::pmr::polymorphic_allocator<int>>::value);
#endif

  EXPECT_FALSE(pmr::has_deallocate<void>::value);
  EXPECT_FALSE(pmr::has_deallocate<int>::value);
  EXPECT_FALSE(pmr::has_deallocate<std::vector<int>>::value);
  EXPECT_FALSE(pmr::has_deallocate<std::allocator<int>>::value);
#if defined(LIBIMP_CPP_17) && defined(__cpp_lib_memory_resource)
  EXPECT_TRUE (pmr::has_deallocate<std::pmr::memory_resource>::value);
  EXPECT_FALSE(pmr::has_deallocate<std::pmr::polymorphic_allocator<int>>::value);
#endif
}

TEST(memory_resource, new_delete_resource) {
  pmr::new_delete_resource mem_res;

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