
#include <vector>
#include <utility>

#include "gtest/gtest.h"

#include "libpmr/allocator.h"

TEST(allocator, construct) {
  pmr::allocator alc;
  EXPECT_FALSE(alc.valid());
  EXPECT_FALSE(alc);
}

TEST(allocator, construct_with_memory_resource) {
  pmr::new_delete_resource mem_res;
  pmr::allocator alc {&mem_res};
  EXPECT_TRUE(alc.valid());
  EXPECT_TRUE(alc);

  auto p = alc.alloc(128);
  EXPECT_NE(p, nullptr);
  EXPECT_NO_THROW(alc.free(p, 128));
}

TEST(allocator, construct_copy_move) {
  pmr::new_delete_resource mem_res;

  pmr::allocator alc1 {&mem_res}, alc2;
  EXPECT_TRUE (alc1.valid());
  EXPECT_TRUE (alc1);
  EXPECT_FALSE(alc2.valid());
  EXPECT_FALSE(alc2);

  pmr::allocator alc3 {alc1}, alc4{alc2}, alc5 {std::move(alc1)};
  EXPECT_TRUE (alc3.valid());
  EXPECT_TRUE (alc3);
  EXPECT_FALSE(alc4.valid());
  EXPECT_FALSE(alc4);
  EXPECT_TRUE (alc5.valid());
  EXPECT_TRUE (alc5);
  EXPECT_FALSE(alc1.valid());
  EXPECT_FALSE(alc1);
}

TEST(allocator, swap) {
  pmr::new_delete_resource mem_res;

  pmr::allocator alc1 {&mem_res}, alc2;
  EXPECT_TRUE (alc1.valid());
  EXPECT_TRUE (alc1);
  EXPECT_FALSE(alc2.valid());
  EXPECT_FALSE(alc2);

  alc1.swap(alc2);
  EXPECT_FALSE(alc1.valid());
  EXPECT_FALSE(alc1);
  EXPECT_TRUE (alc2.valid());
  EXPECT_TRUE (alc2);
}

TEST(allocator, invalid_alloc_free) {
  pmr::new_delete_resource mem_res;

  pmr::allocator alc1 {&mem_res}, alc2;
  EXPECT_EQ(alc1.alloc(0), nullptr);
  EXPECT_NO_THROW(alc1.free(nullptr, 128));
  EXPECT_NO_THROW(alc1.free(nullptr, 0));
  EXPECT_NO_THROW(alc1.free(&alc1, 0));

  EXPECT_EQ(alc2.alloc(0), nullptr);
  EXPECT_NO_THROW(alc2.free(nullptr, 128));
  EXPECT_NO_THROW(alc2.free(nullptr, 0));
  EXPECT_NO_THROW(alc2.free(&alc1, 0));

  EXPECT_EQ(alc2.alloc(1024), nullptr);
  EXPECT_NO_THROW(alc2.free(&alc1, sizeof(alc1)));
}