
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <array>

#include "gtest/gtest.h"

#include "libconcur/queue.h"
#include "libimp/log.h"
#include "libimp/nameof.h"

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
  log.info("\n\tStart with: [", imp::nameof<PR>(), " - ", imp::nameof<CR>(), "]\n\t\t", np, " producers, ", nc, " consumers...");

  struct Data {
    std::uint64_t n;
    std::uint64_t i;
  };
  queue<Data, PR, CR> que;

  constexpr static std::uint32_t loop_size = 10'0000;

  std::atomic<std::uint64_t> sum {0};
  std::atomic<std::size_t> running {np};

  auto prod_call = [&](std::size_t n) {
    for (std::uint32_t i = 1; i <= loop_size; ++i) {
      std::this_thread::yield();
      for (std::uint32_t k = 1; !que.push(Data{n, i}); ++k) {
        std::this_thread::yield();
        if (k % (loop_size / 10) == 0) {
          log.info("[", n, "] put count: ", i, ", retry: ", k);
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      }
      if (i % (loop_size / 10) == 0) {
        log.info("[", n, "] put count: ", i);
      }
    }
    --running;
  };
  auto cons_call = [&] {
    for (;;) {
      std::this_thread::yield();
      Data v;
      while (!que.pop(v)) {
        if (running == 0) return;
        std::this_thread::yield();
      }
      sum += v.i;
    }
  };

  std::vector<std::thread> prods(np);
  for (std::size_t n = 0; n < np; ++n) prods[n] = std::thread(prod_call, n);
  std::vector<std::thread> conss(nc);
  for (auto &c : conss) c = std::thread(cons_call);

  for (auto &p : prods) p.join();
  for (auto &c : conss) c.join();

  EXPECT_EQ(sum, np * (loop_size * std::uint64_t(loop_size + 1)) / 2);
}

} // namespace

TEST(queue, multi_thread) {
  using namespace concur;

  /// \brief 1-1
  test_queue<relation::single, relation::single>(1, 1);
  test_queue<relation::single, relation::multi >(1, 1);
  test_queue<relation::multi , relation::single>(1, 1);
  test_queue<relation::multi , relation::multi >(1, 1);

  /// \brief 8-1
  test_queue<relation::multi , relation::single>(8, 1);
  test_queue<relation::multi , relation::multi >(8, 1);

  /// \brief 1-8
  test_queue<relation::single, relation::multi >(1, 8);
  test_queue<relation::multi , relation::multi >(1, 8);

  /// \brief 8-8
  test_queue<relation::multi , relation::multi >(8, 8);
}
