
#include <type_traits>
#include <algorithm>

#include "gtest/gtest.h"

#include "libpmr/block_pool.h"

TEST(pmr_block_pool, central_cache_allocator) {
  auto &a = pmr::central_cache_allocator();
  ASSERT_FALSE(nullptr == a.allocate(1));
  ASSERT_FALSE(nullptr == a.allocate(10));
  ASSERT_FALSE(nullptr == a.allocate(100));
  ASSERT_FALSE(nullptr == a.allocate(1000));
  ASSERT_FALSE(nullptr == a.allocate(10000));
}

TEST(pmr_block_pool, block) {
  pmr::block<1> b1;
  EXPECT_EQ(sizeof(b1), (std::max)(alignof(std::max_align_t), sizeof(void *)));
  pmr::block<sizeof(void *)> b2;
  EXPECT_EQ(sizeof(b2), (std::max)(alignof(std::max_align_t), sizeof(void *)));
  pmr::block<sizeof(void *) + 1> b3;
  EXPECT_EQ(sizeof(b3), (std::max)(alignof(std::max_align_t), sizeof(void *) * 2));
}

TEST(pmr_block_pool, central_cache_pool_ctor) {
  ASSERT_FALSE((std::is_default_constructible<pmr::central_cache_pool<pmr::block<1>, 1>>::value));
  ASSERT_FALSE((std::is_copy_constructible<pmr::central_cache_pool<pmr::block<1>, 1>>::value));
  ASSERT_FALSE((std::is_move_constructible<pmr::central_cache_pool<pmr::block<1>, 1>>::value));
  ASSERT_FALSE((std::is_copy_assignable<pmr::central_cache_pool<pmr::block<1>, 1>>::value));
  ASSERT_FALSE((std::is_move_assignable<pmr::central_cache_pool<pmr::block<1>, 1>>::value));
  {
    auto &pool = pmr::central_cache_pool<pmr::block<1>, 1>::instance();
    pmr::block<1> *b1 = pool.aqueire();
    ASSERT_FALSE(nullptr == b1);
    ASSERT_TRUE (nullptr == b1->next);
    pool.release(b1);
    pmr::block<1> *b2 = pool.aqueire();
    EXPECT_EQ(b1, b2);
    pmr::block<1> *b3 = pool.aqueire();
    ASSERT_FALSE(nullptr == b3);
    ASSERT_TRUE (nullptr == b3->next);
    EXPECT_NE(b1, b3);
  }
  {
    auto &pool = pmr::central_cache_pool<pmr::block<1>, 2>::instance();
    pmr::block<1> *b1 = pool.aqueire();
    ASSERT_FALSE(nullptr == b1);
    ASSERT_FALSE(nullptr == b1->next);
    ASSERT_TRUE (nullptr == b1->next->next);
    pool.release(b1);
    pmr::block<1> *b2 = pool.aqueire();
    EXPECT_EQ(b1, b2);
    pmr::block<1> *b3 = pool.aqueire();
    EXPECT_NE(b1, b3);
    pmr::block<1> *b4 = pool.aqueire();
    ASSERT_FALSE(nullptr == b4);
    ASSERT_FALSE(nullptr == b4->next);
    ASSERT_TRUE (nullptr == b4->next->next);
    EXPECT_NE(b1, b4);
  }
}

TEST(pmr_block_pool, ctor) {
  ASSERT_TRUE ((std::is_default_constructible<pmr::block_pool<1, 1>>::value));
  ASSERT_FALSE((std::is_copy_constructible<pmr::block_pool<1, 1>>::value));
  ASSERT_TRUE ((std::is_move_constructible<pmr::block_pool<1, 1>>::value));
  ASSERT_FALSE((std::is_copy_assignable<pmr::block_pool<1, 1>>::value));
  ASSERT_FALSE((std::is_move_assignable<pmr::block_pool<1, 1>>::value));
}

TEST(pmr_block_pool, allocate) {
  std::vector<void *> v;
  pmr::block_pool<1, 1> pool;
  for (int i = 0; i < 100; ++i) {
    v.push_back(pool.allocate());
  }
  for (void *p: v) {
    ASSERT_FALSE(nullptr == p);
    pool.deallocate(p);
  }
  for (int i = 0; i < 100; ++i) {
    ASSERT_EQ(v[v.size() - i - 1], pool.allocate());
  }
  for (void *p: v) {
    pool.deallocate(p);
  }
}
