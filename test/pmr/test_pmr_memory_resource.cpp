
#include <utility>

#include "gtest/gtest.h"

#include "libpmr/memory_resource.h"

namespace {

template <typename T>
void *test_mr(T &&mr, std::size_t bytes, std::size_t alignment) {
  auto p = std::forward<T>(mr).do_allocate(bytes, alignment);
  if (alignment == 0) {
    EXPECT_EQ(p, nullptr);
  } else if (p != nullptr) {
    EXPECT_EQ((std::size_t)p % alignment, 0);
  }
  std::forward<T>(mr).do_deallocate(p, bytes, alignment);
  return p;
}

} // namespace

TEST(memory_resource, new_delete_resource) {
  pmr::new_delete_resource m_res;

  EXPECT_EQ(test_mr(m_res, 0, 0), nullptr);
  EXPECT_EQ(test_mr(m_res, 0, 1), nullptr);
  EXPECT_EQ(test_mr(m_res, 0, 2), nullptr);
  EXPECT_EQ(test_mr(m_res, 0, 3), nullptr);
  EXPECT_EQ(test_mr(m_res, 0, 8), nullptr);
  EXPECT_EQ(test_mr(m_res, 0, 64), nullptr);

  EXPECT_EQ(test_mr(m_res, 1, 0), nullptr);
  EXPECT_NE(test_mr(m_res, 1, 1), nullptr);
  EXPECT_NE(test_mr(m_res, 1, 2), nullptr);
  EXPECT_EQ(test_mr(m_res, 1, 3), nullptr);
  EXPECT_NE(test_mr(m_res, 1, 8), nullptr);
  EXPECT_NE(test_mr(m_res, 1, 64), nullptr);
}