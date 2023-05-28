
#include "gtest/gtest.h"

#include "libconcur/queue.h"
#include "libimp/log.h"

using namespace concur;

TEST(queue, construct) {
  using queue_t = queue<int>;
  queue_t q1;
  EXPECT_TRUE(q1.valid());
  EXPECT_TRUE(q1.empty());
  EXPECT_EQ(q1.approx_size(), 0);
}

namespace {

template <typename PR, typename CR>
void test_queue_basic() {
  using queue_t = queue<int, PR, CR>;
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
    ASSERT_TRUE(q1.push(i));
    ASSERT_FALSE(q1.empty());
    ++count;
    ASSERT_EQ(q1.approx_size(), count);
  };
  auto pop = [&q1, &count](int i) {
    int value;
    ASSERT_TRUE(q1.pop(value));
    ASSERT_EQ(value, i);
    --count;
    ASSERT_EQ(q1.approx_size(), count);
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
  ASSERT_FALSE(q1.push(65536));
  for (int i = 0; i < default_circle_buffer_size; ++i) {
    pop(i);
  }
}

} // namespace

TEST(queue, push_pop) {
  test_queue_basic<relation::single, relation::single>();
  test_queue_basic<relation::single, relation::multi >();
  test_queue_basic<relation::multi , relation::multi >();
}

namespace {

template <typename PR, typename CR>
void test_queue(std::size_t np, std::size_t nc) {
  LIBIMP_LOG_();

}

} // namespace