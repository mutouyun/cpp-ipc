
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