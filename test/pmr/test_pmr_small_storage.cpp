
#include <type_traits>
#include <utility>

#include "gtest/gtest.h"

#include "libpmr/small_storage.h"

TEST(small_storage, holder_construct) {
  pmr::holder_null();
  pmr::holder<int, true>();
  pmr::holder<int, false>();
  pmr::holder<void, true>();
  pmr::holder<void, false>();
  SUCCEED();
}

TEST(small_storage, holder_copy_move_construct) {
  EXPECT_FALSE(std::is_copy_constructible<pmr::holder_null>::value);
  EXPECT_FALSE((std::is_copy_constructible<pmr::holder<int, true>>::value));
  EXPECT_FALSE((std::is_copy_constructible<pmr::holder<int, false>>::value));
  EXPECT_FALSE((std::is_copy_constructible<pmr::holder<void, true>>::value));
  EXPECT_FALSE((std::is_copy_constructible<pmr::holder<void, false>>::value));

  EXPECT_FALSE(std::is_copy_assignable<pmr::holder_null>::value);
  EXPECT_FALSE((std::is_copy_assignable<pmr::holder<int, true>>::value));
  EXPECT_FALSE((std::is_copy_assignable<pmr::holder<int, false>>::value));
  EXPECT_FALSE((std::is_copy_assignable<pmr::holder<void, true>>::value));
  EXPECT_FALSE((std::is_copy_assignable<pmr::holder<void, false>>::value));

  EXPECT_FALSE(std::is_move_constructible<pmr::holder_null>::value);
  EXPECT_FALSE((std::is_move_constructible<pmr::holder<int, true>>::value));
  EXPECT_FALSE((std::is_move_constructible<pmr::holder<int, false>>::value));
  EXPECT_FALSE((std::is_move_constructible<pmr::holder<void, true>>::value));
  EXPECT_FALSE((std::is_move_constructible<pmr::holder<void, false>>::value));

  EXPECT_FALSE(std::is_move_assignable<pmr::holder_null>::value);
  EXPECT_FALSE((std::is_move_assignable<pmr::holder<int, true>>::value));
  EXPECT_FALSE((std::is_move_assignable<pmr::holder<int, false>>::value));
  EXPECT_FALSE((std::is_move_assignable<pmr::holder<void, true>>::value));
  EXPECT_FALSE((std::is_move_assignable<pmr::holder<void, false>>::value));
}

TEST(small_storage, holder_copy_move) {
  struct foo {
    int i;
    foo(int i) : i(i) {}
    foo(foo const &rhs) : i(rhs.i) {}
    foo(foo &&rhs) : i(std::exchange(rhs.i, 0)) {}
  };

  pmr::allocator alc;
  pmr::holder<foo, true> h1(alc, 1);
  pmr::holder<foo, true> h2, h3;
  h1.copy_to(alc, &h2);
  EXPECT_EQ(static_cast<foo *>(h1.get())->i, 1);
  EXPECT_EQ(static_cast<foo *>(h2.get())->i, 1);
  h1.move_to(alc, &h3);
  EXPECT_EQ(static_cast<foo *>(h1.get())->i, 0);
  EXPECT_EQ(static_cast<foo *>(h3.get())->i, 1);
  h1.destroy(alc);
  h2.destroy(alc);
  h3.destroy(alc);

  pmr::holder<foo, false> h4(alc, 1);
  pmr::holder<foo, false> h5, h6;
  h4.copy_to(alc, &h5);
  EXPECT_EQ(static_cast<foo *>(h4.get())->i, 1);
  EXPECT_EQ(static_cast<foo *>(h5.get())->i, 1);
  h4.move_to(alc, &h6);
  EXPECT_EQ(static_cast<foo *>(h4.get())->i, 0);
  EXPECT_EQ(static_cast<foo *>(h6.get())->i, 1);
  h4.destroy(alc);
  h5.destroy(alc);
  h6.destroy(alc);

  void *ph1 = std::malloc(pmr::holder<void, true>::full_sizeof<int>(10));
  void *ph2 = std::malloc(pmr::holder<void, true>::full_sizeof<int>(10));
  void *ph3 = std::malloc(pmr::holder<void, true>::full_sizeof<int>(10));
  auto *h7 = ::new (ph1) pmr::holder<void, true>(alc, ::LIBIMP::types<int>{}, 10);
  auto *h8 = ::new (ph2) pmr::holder<void, true>;
  auto *h9 = ::new (ph2) pmr::holder<void, true>;
  h7->copy_to(alc, h8);
  EXPECT_EQ(h7->count(), 10);
  EXPECT_EQ(h8->count(), 10);
  h7->move_to(alc, h9);
  EXPECT_EQ(h7->count(), 0);
  EXPECT_EQ(h9->count(), 10);
  h7->destroy(alc);
  h8->destroy(alc);
  h9->destroy(alc);
  std::free(ph1);
  std::free(ph2);
  std::free(ph3);

  pmr::holder<void, false> h10(alc, ::LIBIMP::types<int>{}, 10);
  pmr::holder<void, false> h11, h12;
  h10.copy_to(alc, &h11);
  EXPECT_EQ(h10.count(), 10);
  EXPECT_EQ(h11.count(), 10);
  h10.move_to(alc, &h12);
  EXPECT_EQ(h10.count(), 0);
  EXPECT_EQ(h12.count(), 10);
  h10.destroy(alc);
  h11.destroy(alc);
  h12.destroy(alc);
}

TEST(small_storage, sizeof) {
  EXPECT_EQ(sizeof(pmr::holder_null), sizeof(void *));
  EXPECT_EQ(sizeof(pmr::holder<int, true>), sizeof(void *) + imp::round_up(sizeof(int), alignof(void *)));
  EXPECT_EQ(sizeof(pmr::holder<int, false>), sizeof(void *) + sizeof(void *));
  EXPECT_EQ(sizeof(pmr::holder<void, true>), sizeof(void *) + sizeof(pmr::detail::holder_info));
  EXPECT_EQ(sizeof(pmr::holder<void, false>), sizeof(void *) + sizeof(void *));

  // pmr::small_storage<4> s1;
  EXPECT_EQ(sizeof(pmr::small_storage<16>)  , 16);
  EXPECT_EQ(sizeof(pmr::small_storage<64>)  , 64);
  EXPECT_EQ(sizeof(pmr::small_storage<512>) , 512);
  EXPECT_EQ(sizeof(pmr::small_storage<4096>), 4096);
}

TEST(small_storage, construct) {
  pmr::small_storage<64> ss;
  SUCCEED();
}

TEST(small_storage, acquire) {
  pmr::small_storage<128> ss;
  pmr::allocator alc;
  ASSERT_FALSE(ss.valid());
  int *p = ss.acquire<int>(alc, 3);
  ASSERT_TRUE(ss.valid());
  ASSERT_NE(p, nullptr);
  ASSERT_EQ(*p, 3);
  ASSERT_EQ(p, ss.as<int>());
  ASSERT_EQ(ss.count(), 1);
  ASSERT_EQ(ss.sizeof_heap(), 0);
  ASSERT_EQ(ss.sizeof_type(), sizeof(int));

  p = ss.acquire<int[]>(alc, 3);
  ASSERT_TRUE(ss.valid());
  ASSERT_NE(p, nullptr);
  ASSERT_EQ(p, ss.as<int>());
  ASSERT_EQ(ss.count(), 3);
  ASSERT_EQ(ss.sizeof_heap(), 0);
  ASSERT_EQ(ss.sizeof_type(), sizeof(int));

  p = ss.acquire<int[]>(alc, 30);
  ASSERT_TRUE(ss.valid());
  ASSERT_NE(p, nullptr);
  ASSERT_EQ(p, ss.as<int>());
  ASSERT_EQ(ss.count(), 30);
  ASSERT_EQ(ss.sizeof_heap(), sizeof(int) * 30 + sizeof(pmr::detail::holder_info));
  ASSERT_EQ(ss.sizeof_type(), sizeof(int));

  ss.release(alc);
  SUCCEED();
}
