
#include <vector>
#include <utility>

#include "gtest/gtest.h"

#include "libpmr/allocator.h"

TEST(pmr_allocator, construct) {
  pmr::allocator alc;
  SUCCEED();
}

TEST(pmr_allocator, construct_value_initialization) {
  pmr::allocator alc{};
  auto p = alc.allocate(128);
  EXPECT_NE(p, nullptr);
  EXPECT_NO_THROW(alc.deallocate(p, 128));
}

namespace {

class dummy_resource {
public:
  void *allocate(std::size_t, std::size_t = 0) noexcept {
    return nullptr;
  }
  void deallocate(void *, std::size_t, std::size_t = 0) noexcept {
    return;
  }
};

} // namespace

TEST(pmr_allocator, construct_copy_move) {
  pmr::new_delete_resource mem_res;
  dummy_resource dummy_res;
  pmr::allocator alc1{&mem_res}, alc2{&dummy_res};
  auto p = alc1.allocate(128);
  ASSERT_NE(p, nullptr);
  ASSERT_NO_THROW(alc1.deallocate(p, 128));
  ASSERT_EQ(alc2.allocate(128), nullptr);

  pmr::allocator alc3{alc1}, alc4{alc2}, alc5{std::move(alc1)};

  p = alc3.allocate(128);
  ASSERT_NE(p, nullptr);
  ASSERT_NO_THROW(alc3.deallocate(p, 128));

  ASSERT_EQ(alc4.allocate(128), nullptr);

  p = alc5.allocate(128);
  ASSERT_NE(p, nullptr);
  ASSERT_NO_THROW(alc5.deallocate(p, 128));
}

TEST(pmr_allocator, swap) {
  pmr::new_delete_resource mem_res;
  dummy_resource dummy_res;
  pmr::allocator alc1{&mem_res}, alc2{&dummy_res};
  alc1.swap(alc2);
  auto p = alc2.allocate(128);
  ASSERT_NE(p, nullptr);
  ASSERT_NO_THROW(alc2.deallocate(p, 128));
  ASSERT_EQ(alc1.allocate(128), nullptr);
}

TEST(pmr_allocator, invalid_alloc_free) {
  pmr::allocator alc1;
  EXPECT_EQ(alc1.allocate(0), nullptr);
  EXPECT_NO_THROW(alc1.deallocate(nullptr, 128));
  EXPECT_NO_THROW(alc1.deallocate(nullptr, 0));
  EXPECT_NO_THROW(alc1.deallocate(&alc1, 0));
}

TEST(pmr_allocator, sizeof) {
  EXPECT_EQ(sizeof(pmr::allocator), sizeof(void *) * 2);
}
