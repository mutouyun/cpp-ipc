
#include <iostream>
#include <cstddef>

#include "gtest/gtest.h"

#include "libconcur/concurrent.h"

TEST(concurrent, cache_line_size) {
  std::cout << concur::cache_line_size << "\n";
  EXPECT_TRUE(concur::cache_line_size >= alignof(std::max_align_t));
}

TEST(concurrent, index_and_flag) {
  EXPECT_TRUE(sizeof(concur::index_t) < sizeof(concur::state::flag_t));
}

TEST(concurrent, trunc_index) {
  struct context {
    concur::index_t circ_size;

    bool valid() const noexcept {
      return (circ_size > 1) && ((circ_size & (circ_size - 1)) == 0);
    }
  };

  /// @brief circ-size = 0
  EXPECT_EQ(concur::trunc_index(context{0}, 0), 0);
  EXPECT_EQ(concur::trunc_index(context{0}, 1), 0);
  EXPECT_EQ(concur::trunc_index(context{0}, 2), 0);
  EXPECT_EQ(concur::trunc_index(context{0}, 16), 0);
  EXPECT_EQ(concur::trunc_index(context{0}, 111), 0);
  EXPECT_EQ(concur::trunc_index(context{0}, -1), 0);

  /// @brief circ-size = 1
  EXPECT_EQ(concur::trunc_index(context{1}, 0), 0);
  EXPECT_EQ(concur::trunc_index(context{1}, 1), 0);
  EXPECT_EQ(concur::trunc_index(context{1}, 2), 0);
  EXPECT_EQ(concur::trunc_index(context{1}, 16), 0);
  EXPECT_EQ(concur::trunc_index(context{1}, 111), 0);
  EXPECT_EQ(concur::trunc_index(context{1}, -1), 0);

  /// @brief circ-size = 2
  EXPECT_EQ(concur::trunc_index(context{2}, 0), 0);
  EXPECT_EQ(concur::trunc_index(context{2}, 1), 1);
  EXPECT_EQ(concur::trunc_index(context{2}, 2), 0);
  EXPECT_EQ(concur::trunc_index(context{2}, 16), 0);
  EXPECT_EQ(concur::trunc_index(context{2}, 111), 1);
  EXPECT_EQ(concur::trunc_index(context{2}, -1), 1);

  /// @brief circ-size = 10
  EXPECT_EQ(concur::trunc_index(context{10}, 0), 0);
  EXPECT_EQ(concur::trunc_index(context{10}, 1), 0);
  EXPECT_EQ(concur::trunc_index(context{10}, 2), 0);
  EXPECT_EQ(concur::trunc_index(context{10}, 16), 0);
  EXPECT_EQ(concur::trunc_index(context{10}, 111), 0);
  EXPECT_EQ(concur::trunc_index(context{10}, -1), 0);

  /// @brief circ-size = 16
  EXPECT_EQ(concur::trunc_index(context{16}, 0), 0);
  EXPECT_EQ(concur::trunc_index(context{16}, 1), 1);
  EXPECT_EQ(concur::trunc_index(context{16}, 2), 2);
  EXPECT_EQ(concur::trunc_index(context{16}, 16), 0);
  EXPECT_EQ(concur::trunc_index(context{16}, 111), 15);
  EXPECT_EQ(concur::trunc_index(context{16}, -1), 15);

  /// @brief circ-size = (index_t)-1
  EXPECT_EQ(concur::trunc_index(context{(concur::index_t)-1}, 0), 0);
  EXPECT_EQ(concur::trunc_index(context{(concur::index_t)-1}, 1), 0);
  EXPECT_EQ(concur::trunc_index(context{(concur::index_t)-1}, 2), 0);
  EXPECT_EQ(concur::trunc_index(context{(concur::index_t)-1}, 16), 0);
  EXPECT_EQ(concur::trunc_index(context{(concur::index_t)-1}, 111), 0);
  EXPECT_EQ(concur::trunc_index(context{(concur::index_t)-1}, -1), 0);

  /// @brief circ-size = 2147483648 (2^31)
  EXPECT_EQ(concur::trunc_index(context{2147483648u}, 0), 0);
  EXPECT_EQ(concur::trunc_index(context{2147483648u}, 1), 1);
  EXPECT_EQ(concur::trunc_index(context{2147483648u}, 2), 2);
  EXPECT_EQ(concur::trunc_index(context{2147483648u}, 16), 16);
  EXPECT_EQ(concur::trunc_index(context{2147483648u}, 111), 111);
  EXPECT_EQ(concur::trunc_index(context{2147483648u}, -1), 2147483647);
}