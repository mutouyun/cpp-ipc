
#include <limits>
#include <array>
#include <cstring>
#include <cstddef>
#include <thread>

#include "test.h"

#include "libipc/mem/new.h"

TEST(new, regular_sizeof) {
  ASSERT_EQ(ipc::mem::regular_sizeof<std::int8_t >(), ipc::mem::regular_head_size + alignof(std::max_align_t));
  ASSERT_EQ(ipc::mem::regular_sizeof<std::int16_t>(), ipc::mem::regular_head_size + alignof(std::max_align_t));
  ASSERT_EQ(ipc::mem::regular_sizeof<std::int32_t>(), ipc::mem::regular_head_size + alignof(std::max_align_t));
  ASSERT_EQ(ipc::mem::regular_sizeof<std::int64_t>(), ipc::mem::regular_head_size + alignof(std::max_align_t));

  ASSERT_EQ((ipc::mem::regular_sizeof<std::array<char, 10    >>()), ipc::round_up<std::size_t>(ipc::mem::regular_head_size + 10   , alignof(std::max_align_t)));
  ASSERT_EQ((ipc::mem::regular_sizeof<std::array<char, 100   >>()), ipc::round_up<std::size_t>(ipc::mem::regular_head_size + 100  , alignof(std::max_align_t)));
  ASSERT_EQ((ipc::mem::regular_sizeof<std::array<char, 1000  >>()), ipc::round_up<std::size_t>(ipc::mem::regular_head_size + 1000 , 128));
  ASSERT_EQ((ipc::mem::regular_sizeof<std::array<char, 10000 >>()), ipc::round_up<std::size_t>(ipc::mem::regular_head_size + 10000, 8192));
  ASSERT_EQ((ipc::mem::regular_sizeof<std::array<char, 100000>>()), (std::numeric_limits<std::size_t>::max)());
}

TEST(new, new) {
  auto p = ipc::mem::$new<int>();
  ASSERT_NE(p, nullptr);
  *p = -1;
  ASSERT_EQ(*p, -1);
  ipc::mem::$delete(p);
}

TEST(new, new_value) {
  auto p = ipc::mem::$new<int>((std::numeric_limits<int>::max)());
  ASSERT_NE(p, nullptr);
  ASSERT_EQ(*p, (std::numeric_limits<int>::max)());
  ipc::mem::$delete(p);
}

namespace {

template <std::size_t Pts, std::size_t N>
void test_new$array() {
  std::array<void *, Pts> pts;
  using T = std::array<char, N>;
  for (int i = 0; i < (int)pts.size(); ++i) {
    auto p = ipc::mem::$new<T>();
    pts[i] = p;
    std::memset(p, i, sizeof(T));
  }
  for (int i = 0; i < (int)pts.size(); ++i) {
    T tmp;
    std::memset(&tmp, i, sizeof(T));
    ASSERT_EQ(std::memcmp(pts[i], &tmp, sizeof(T)), 0);
    ipc::mem::$delete(static_cast<T *>(pts[i]));
  }
}

} // namespace

TEST(new, new_array) {
  test_new$array<1000, 10>();
  test_new$array<1000, 100>();
  test_new$array<1000, 1000>();
  test_new$array<1000, 10000>();
  test_new$array<1000, 100000>();
  // test_new$array<1000, 1000000>();
}

namespace {

int construct_count__ = 0;

class Base {
public:
  virtual ~Base() = default;
  virtual int get() const = 0;
};

class Derived : public Base {
public:
  Derived(int value) : value_(value) {
    construct_count__ = value_;
  }

  ~Derived() override {
    construct_count__ = 0;
  }

  int get() const override { return value_; }

private:
  int value_;
};

class Derived64K : public Derived {
public:
  using Derived::Derived;

private:
  std::array<char, 65536> padding_;
};

} // namespace

TEST(new, delete_poly) {
  Base *p = ipc::mem::$new<Derived>(-1);
  ASSERT_NE(p, nullptr);
  ASSERT_EQ(p->get(), -1);
  ASSERT_EQ(construct_count__, -1);
  ipc::mem::$delete(p);
  ASSERT_EQ(construct_count__, 0);

  ASSERT_EQ(p, ipc::mem::$new<Derived>((std::numeric_limits<int>::max)()));
  ASSERT_EQ(p->get(), (std::numeric_limits<int>::max)());
  ASSERT_EQ(construct_count__, (std::numeric_limits<int>::max)());
  ipc::mem::$delete(p);
  ASSERT_EQ(construct_count__, 0);
}

TEST(new, delete_poly64k) {
  Base *p = ipc::mem::$new<Derived64K>(-1);
  ASSERT_NE(p, nullptr);
  ASSERT_EQ(p->get(), -1);
  ASSERT_EQ(construct_count__, -1);
  ipc::mem::$delete(p);
  ASSERT_EQ(construct_count__, 0);

  Base *q = ipc::mem::$new<Derived64K>((std::numeric_limits<int>::max)());
  ASSERT_EQ(q->get(), (std::numeric_limits<int>::max)());
  ASSERT_EQ(construct_count__, (std::numeric_limits<int>::max)());
  ipc::mem::$delete(q);
  ASSERT_EQ(construct_count__, 0);
}

TEST(new, delete_null) {
  Base *p = nullptr;
  ipc::mem::$delete(p);
  SUCCEED();
}

TEST(new, multi_thread) {
  std::array<std::thread, 16> threads;
  for (auto &t : threads) {
    t = std::thread([] {
      for (int i = 0; i < 10000; ++i) {
        auto p = ipc::mem::$new<int>();
        *p = i;
        ipc::mem::$delete(p);
      }
      std::array<void *, 10000> pts;
      for (int i = 0; i < 10000; ++i) {
        auto p = ipc::mem::$new<std::array<char, 10>>();
        pts[i] = p;
        std::memset(p, i, sizeof(std::array<char, 10>));
      }
      for (int i = 0; i < 10000; ++i) {
        std::array<char, 10> tmp;
        std::memset(&tmp, i, sizeof(std::array<char, 10>));
        ASSERT_EQ(std::memcmp(pts[i], &tmp, sizeof(std::array<char, 10>)), 0);
        ipc::mem::$delete(static_cast<std::array<char, 10> *>(pts[i]));
      }
    });
  }
  for (auto &t : threads) {
    t.join();
  }
  SUCCEED();
}
