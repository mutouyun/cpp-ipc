
#include <type_traits>
#include <utility>

#include "gtest/gtest.h"

#include "libpmr/small_storage.h"

TEST(small_storage, holder_construct) {
  pmr::holder_null();
  pmr::holder<int, true>();
  pmr::holder<int, false>();
  pmr::holder<void, true>();
  SUCCEED();
}

TEST(small_storage, holder_copy_move_construct) {
  EXPECT_FALSE(std::is_copy_constructible<pmr::holder_null>::value);
  EXPECT_FALSE((std::is_copy_constructible<pmr::holder<int, true>>::value));
  EXPECT_FALSE((std::is_copy_constructible<pmr::holder<int, false>>::value));
  EXPECT_FALSE((std::is_copy_constructible<pmr::holder<void, true>>::value));

  EXPECT_FALSE(std::is_copy_assignable<pmr::holder_null>::value);
  EXPECT_FALSE((std::is_copy_assignable<pmr::holder<int, true>>::value));
  EXPECT_FALSE((std::is_copy_assignable<pmr::holder<int, false>>::value));
  EXPECT_FALSE((std::is_copy_assignable<pmr::holder<void, true>>::value));

  EXPECT_FALSE(std::is_move_constructible<pmr::holder_null>::value);
  EXPECT_FALSE((std::is_move_constructible<pmr::holder<int, true>>::value));
  EXPECT_FALSE((std::is_move_constructible<pmr::holder<int, false>>::value));
  EXPECT_FALSE((std::is_move_constructible<pmr::holder<void, true>>::value));

  EXPECT_FALSE(std::is_move_assignable<pmr::holder_null>::value);
  EXPECT_FALSE((std::is_move_assignable<pmr::holder<int, true>>::value));
  EXPECT_FALSE((std::is_move_assignable<pmr::holder<int, false>>::value));
  EXPECT_FALSE((std::is_move_assignable<pmr::holder<void, true>>::value));
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
  pmr::holder<foo, true> h2, h3; // uninitialized
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
  pmr::holder<foo, false> h5, h6; // uninitialized
  h4.copy_to(alc, &h5);
  EXPECT_EQ(static_cast<foo *>(h4.get())->i, 1);
  EXPECT_EQ(static_cast<foo *>(h5.get())->i, 1);
  h4.move_to(alc, &h6);
  EXPECT_EQ(static_cast<foo *>(h4.get())->i, 0);
  EXPECT_EQ(static_cast<foo *>(h6.get())->i, 1);
  h4.destroy(alc);
  h5.destroy(alc);
  h6.destroy(alc);

  pmr::holder<void, true> h7(alc, ::LIBIMP::types<int>{}, 10);
  pmr::holder<void, true> h8, h9; // uninitialized
  h7.copy_to(alc, &h8);
  EXPECT_EQ(h7.count(), 10);
  EXPECT_EQ(h8.count(), 10);
  h7.move_to(alc, &h9);
  EXPECT_EQ(h7.count(), 0);
  EXPECT_EQ(h9.count(), 10);
  h7.destroy(alc);
  h8.destroy(alc);
  h9.destroy(alc);
}

TEST(small_storage, construct) {
}
