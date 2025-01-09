
#include "test.h"

#define private public
#include "libipc/concur/intrusive_stack.h"

using namespace ipc;

TEST(intrusive_stack, construct) {
  concur::intrusive_stack<int> s;
  EXPECT_TRUE(s.empty());
}

TEST(intrusive_stack, construct_node) {
  concur::intrusive_stack<int>::node n{};
  EXPECT_TRUE(n.next.load(std::memory_order_relaxed) == nullptr);
}

TEST(intrusive_stack, copyable) {
  EXPECT_FALSE(std::is_copy_constructible<concur::intrusive_stack<int>>::value);
  EXPECT_FALSE(std::is_copy_assignable<concur::intrusive_stack<int>>::value);
}

TEST(intrusive_stack, moveable) {
  EXPECT_FALSE(std::is_move_constructible<concur::intrusive_stack<int>>::value);
  EXPECT_FALSE(std::is_move_assignable<concur::intrusive_stack<int>>::value);
}

TEST(intrusive_stack, push_one) {
  concur::intrusive_stack<int> s;
  concur::intrusive_stack<int>::node n{123};
  s.push(&n);
  EXPECT_FALSE(s.empty());
  EXPECT_TRUE(s.top_.load(std::memory_order_relaxed) == &n);
  EXPECT_TRUE(n.next.load(std::memory_order_relaxed) == nullptr);
  EXPECT_EQ(n.value, 123);
}

TEST(intrusive_stack, push_many) {
  concur::intrusive_stack<int> s;
  concur::intrusive_stack<int>::node n1{111111};
  concur::intrusive_stack<int>::node n2{222222};
  concur::intrusive_stack<int>::node n3{333333};
  s.push(&n1);
  s.push(&n2);
  s.push(&n3);
  EXPECT_FALSE(s.empty());
  EXPECT_TRUE(s.top_.load(std::memory_order_relaxed) == &n3);
  EXPECT_TRUE(n3.next.load(std::memory_order_relaxed) == &n2);
  EXPECT_TRUE(n2.next.load(std::memory_order_relaxed) == &n1);
  EXPECT_TRUE(n1.next.load(std::memory_order_relaxed) == nullptr);
  EXPECT_EQ(n1.value, 111111);
  EXPECT_EQ(n2.value, 222222);
  EXPECT_EQ(n3.value, 333333);
}

TEST(intrusive_stack, push_same) {
  concur::intrusive_stack<int> s;
  concur::intrusive_stack<int>::node n{321};
  s.push(&n);
  s.push(&n);
  EXPECT_FALSE(s.empty());
  EXPECT_TRUE(s.top_.load(std::memory_order_relaxed) == &n);
  EXPECT_TRUE(n.next.load(std::memory_order_relaxed) == &n);
  EXPECT_EQ(n.value, 321);
}

TEST(intrusive_stack, pop_empty) {
  concur::intrusive_stack<int> s;
  EXPECT_TRUE(s.pop() == nullptr);
}

TEST(intrusive_stack, pop_one) {
  concur::intrusive_stack<int> s;
  concur::intrusive_stack<int>::node n{112233};
  s.push(&n);
  EXPECT_TRUE(s.pop() == &n);
  EXPECT_TRUE(s.empty());
  EXPECT_TRUE(s.top_.load(std::memory_order_relaxed) == nullptr);
  EXPECT_TRUE(n.next.load(std::memory_order_relaxed) == nullptr);
  EXPECT_EQ(n.value, 112233);
}

TEST(intrusive_stack, pop_many) {
  concur::intrusive_stack<int> s;
  concur::intrusive_stack<int>::node n1{111111};
  concur::intrusive_stack<int>::node n2{222222};
  concur::intrusive_stack<int>::node n3{333333};
  s.push(&n1);
  s.push(&n2);
  s.push(&n3);
  EXPECT_TRUE(s.pop() == &n3);
  EXPECT_TRUE(s.pop() == &n2);
  EXPECT_TRUE(s.pop() == &n1);
  EXPECT_TRUE(s.empty());
  EXPECT_TRUE(s.top_.load(std::memory_order_relaxed) == nullptr);
  EXPECT_TRUE(n3.next.load(std::memory_order_relaxed) == &n2);
  EXPECT_TRUE(n2.next.load(std::memory_order_relaxed) == &n1);
  EXPECT_TRUE(n1.next.load(std::memory_order_relaxed) == nullptr);
  EXPECT_EQ(n1.value, 111111);
  EXPECT_EQ(n2.value, 222222);
  EXPECT_EQ(n3.value, 333333);
}
