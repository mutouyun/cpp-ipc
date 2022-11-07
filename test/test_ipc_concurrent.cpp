
#include <iostream>
#include <cstddef>

#include "gtest/gtest.h"

#include "libipc/concurrent.h"

TEST(concurrent, cache_line_size) {
  std::cout << ipc::cache_line_size << "\n";
  EXPECT_TRUE(ipc::cache_line_size >= alignof(std::max_align_t));
}

TEST(concurrent, index_and_flag) {
  EXPECT_TRUE(sizeof(ipc::concurrent::index_t) < sizeof(ipc::concurrent::state::flag_t));
}