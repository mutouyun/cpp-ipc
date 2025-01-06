
#include "test.h"

#include "libipc/imp/expected.h"

namespace {

class test_val {
public:
  int val = 0;
  int dc_ = 0;
  int cc_ = 0;
  int mv_ = 0;

  test_val() {
    dc_ += 1;
    std::printf("test_val construct.\n");
  }
  test_val(test_val const &o)
    : val(o.val) {
    cc_ = o.cc_ + 1;
    std::printf("test_val copy construct.\n");
  }
  test_val(test_val &&o) noexcept
    : val(std::exchange(o.val, 0)) {
    mv_ = o.mv_ + 1;
    std::printf("test_val move construct.\n");
  }
  ~test_val() {
    std::printf("test_val destruct.\n");
  }

  test_val &operator=(test_val &&o) noexcept {
    mv_ = o.mv_ + 1;
    val = std::exchange(o.val, 0);
    return *this;
  }

  test_val(int v)
    : val(v) {
    std::printf("test_val value initialization.\n");
  }

  bool operator==(test_val const &rhs) const {
    return val == rhs.val;
  }
};

class test_err {
public:
  int dc_ = 0;
  int cc_ = 0;
  int mv_ = 0;
  std::int64_t val = 0;

  test_err() {
    dc_ += 1;
    std::printf("test_err construct.\n");
  }
  test_err(test_err const &o)
    : val(o.val) {
    cc_ = o.cc_ + 1;
    std::printf("test_err copy construct.\n");
  }
  test_err(test_err &&o) noexcept
    : val(std::exchange(o.val, 0)) {
    mv_ = o.mv_ + 1;
    std::printf("test_err move construct.\n");
  }
  ~test_err() {
    std::printf("test_err destruct.\n");
  }

  test_err &operator=(test_err &&o) noexcept {
    mv_ = o.mv_ + 1;
    val = std::exchange(o.val, 0);
    return *this;
  }

  test_err(int v)
    : val(v) {
    std::printf("test_err value initialization.\n");
  }

  bool operator==(test_err const &rhs) const {
    return val == rhs.val;
  }
};

} // namespace

TEST(expected, in_place) {
  ipc::expected<test_val, test_err> e1;
  EXPECT_TRUE(e1);
  EXPECT_EQ(e1.value().dc_, 1);
  EXPECT_EQ(e1.value().val, 0);

  ipc::expected<test_val, test_err> e2 {ipc::in_place, 123};
  EXPECT_TRUE(e2);
  EXPECT_EQ(e2.value().dc_, 0);
  EXPECT_EQ(e2.value().val, 123);
}

TEST(expected, unexpected) {
  ipc::expected<test_val, test_err> e1 {ipc::unexpected};
  EXPECT_FALSE(e1);
  EXPECT_EQ(e1.error().dc_, 1);
  EXPECT_EQ(e1.error().val, 0);

  ipc::expected<test_val, test_err> e2 {ipc::unexpected, 321};
  EXPECT_FALSE(e2);
  EXPECT_EQ(e2.error().dc_, 0);
  EXPECT_EQ(e2.error().val, 321);
}

TEST(expected, copy_and_move) {
  ipc::expected<test_val, test_err> e1 {ipc::in_place, 123};
  ipc::expected<test_val, test_err> e2 {e1};
  EXPECT_TRUE(e1);
  EXPECT_TRUE(e2);
  EXPECT_EQ(e1, e2);
  EXPECT_EQ(e2.value().cc_, 1);
  EXPECT_EQ(e2.value().val, 123);

  ipc::expected<test_val, test_err> e3 {ipc::unexpected, 333};
  ipc::expected<test_val, test_err> e4 {e3};
  EXPECT_FALSE(e3);
  EXPECT_FALSE(e4);
  EXPECT_EQ(e3, e4);
  EXPECT_EQ(e4.error().cc_, 1);
  EXPECT_EQ(e4.error().val, 333);

  ipc::expected<test_val, test_err> e5;
  e5 = e1;
  EXPECT_EQ(e1, e5);
  EXPECT_EQ(e5.value().val, 123);

  ipc::expected<test_val, test_err> e6;
  e6 = std::move(e5);
  EXPECT_EQ(e1, e6);
  EXPECT_EQ(e5.value().val, 0);
}
