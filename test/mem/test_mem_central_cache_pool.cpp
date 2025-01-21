
#include "test.h"

#include <utility>

#include "libipc/mem/central_cache_pool.h"

TEST(central_cache_pool, ctor) {
  ASSERT_FALSE((std::is_default_constructible<ipc::mem::central_cache_pool<ipc::mem::block<1>, 1>>::value));
  ASSERT_FALSE((std::is_copy_constructible<ipc::mem::central_cache_pool<ipc::mem::block<1>, 1>>::value));
  ASSERT_FALSE((std::is_move_constructible<ipc::mem::central_cache_pool<ipc::mem::block<1>, 1>>::value));
  ASSERT_FALSE((std::is_copy_assignable<ipc::mem::central_cache_pool<ipc::mem::block<1>, 1>>::value));
  ASSERT_FALSE((std::is_move_assignable<ipc::mem::central_cache_pool<ipc::mem::block<1>, 1>>::value));
  {
    auto &pool = ipc::mem::central_cache_pool<ipc::mem::block<1>, 1>::instance();
    ipc::mem::block<1> *b1 = pool.aqueire();
    ASSERT_FALSE(nullptr == b1);
    ASSERT_TRUE (nullptr == b1->next);
    pool.release(b1);
    ipc::mem::block<1> *b2 = pool.aqueire();
    EXPECT_EQ(b1, b2);
    ipc::mem::block<1> *b3 = pool.aqueire();
    ASSERT_FALSE(nullptr == b3);
    ASSERT_TRUE (nullptr == b3->next);
    EXPECT_NE(b1, b3);
  }
  {
    auto &pool = ipc::mem::central_cache_pool<ipc::mem::block<1>, 2>::instance();
    ipc::mem::block<1> *b1 = pool.aqueire();
    ASSERT_FALSE(nullptr == b1);
    ASSERT_FALSE(nullptr == b1->next);
    ASSERT_TRUE (nullptr == b1->next->next);
    pool.release(b1);
    ipc::mem::block<1> *b2 = pool.aqueire();
    EXPECT_EQ(b1, b2);
    ipc::mem::block<1> *b3 = pool.aqueire();
    EXPECT_NE(b1, b3);
    ipc::mem::block<1> *b4 = pool.aqueire();
    ASSERT_FALSE(nullptr == b4);
    ASSERT_FALSE(nullptr == b4->next);
    ASSERT_TRUE (nullptr == b4->next->next);
    EXPECT_NE(b1, b4);
  }
}
