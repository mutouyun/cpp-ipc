
#include "test.h"

#include <utility>

#include "libipc/mem/central_cache_allocator.h"

TEST(central_cache_allocator, allocate) {
  auto &a = ipc::mem::central_cache_allocator();
  ASSERT_FALSE(nullptr == a.allocate(1));
  ASSERT_FALSE(nullptr == a.allocate(10));
  ASSERT_FALSE(nullptr == a.allocate(100));
  ASSERT_FALSE(nullptr == a.allocate(1000));
  ASSERT_FALSE(nullptr == a.allocate(10000));
}
