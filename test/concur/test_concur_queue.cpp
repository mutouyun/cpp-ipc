
#include "gtest/gtest.h"

#include "libconcur/queue.h"

using namespace concur;

TEST(queue, construct) {
  using queue_t = queue<int>;
  queue_t q1;
  EXPECT_TRUE(q1.valid());
  EXPECT_TRUE(q1.empty());
  EXPECT_EQ(q1.approx_size(), 0);
}

TEST(queue, push_pop) {
  using queue_t = queue<int>;
  queue_t q1;
  EXPECT_TRUE(q1.valid());
  EXPECT_TRUE(q1.empty());
  EXPECT_EQ(q1.approx_size(), 0);

  EXPECT_TRUE(q1.push(1));
  EXPECT_FALSE(q1.empty());
  EXPECT_EQ(q1.approx_size(), 1);

  int value;
  EXPECT_TRUE(q1.pop(value));
  EXPECT_EQ(value, 1);
  EXPECT_TRUE(q1.empty());
  EXPECT_EQ(q1.approx_size(), 0);

  int count = 0;
  auto push = [&q1, &count](int i) {
    EXPECT_TRUE(q1.push(i));
    EXPECT_FALSE(q1.empty());
    ++count;
    EXPECT_EQ(q1.approx_size(), count);
  };
  auto pop = [&q1, &count](int i) {
    int value;
    EXPECT_TRUE(q1.pop(value));
    EXPECT_EQ(value, i);
    --count;
    EXPECT_EQ(q1.approx_size(), count);
  };

  for (int i = 0; i < 1000; ++i) {
    push(i);
  }
  for (int i = 0; i < 1000; ++i) {
    pop(i);
  }

  for (int i = 0; i < default_circle_buffer_size; ++i) {
    push(i);
  }
  EXPECT_FALSE(q1.push(65536));
  for (int i = 0; i < default_circle_buffer_size; ++i) {
    pop(i);
  }
}