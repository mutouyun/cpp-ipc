
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <typeinfo>
#include <cstddef>

#include "gtest/gtest.h"

#include "libconcur/concurrent.h"
#include "libimp/countof.h"
#include "libimp/log.h"
#include "libimp/nameof.h"

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

namespace {

template <typename PC>
void test_concur(std::size_t np, std::size_t nc, std::size_t k) {
  LIBIMP_LOG_();
  log.info("\n\tStart with: ", imp::nameof<PC>(), ", ", np, " producers, ", nc, " consumers...");

  constexpr static std::uint32_t loop_size = 100'0000;

  concur::element<std::uint64_t> circ[32] {};
  PC pc;
  typename PC::context ctx {imp::make_span(circ)};
  ASSERT_TRUE(ctx.valid());

  std::atomic<std::uint64_t> sum {0};
  std::atomic<std::size_t> running {np};

  auto prod_call = [&](std::size_t n) {
    for (std::uint32_t i = 1; i <= loop_size; ++i) {
      std::this_thread::yield();
      while (!pc.enqueue(imp::make_span(circ), ctx, i)) {
        std::this_thread::yield();
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
      std::uint64_t i;
      while (!pc.dequeue(imp::make_span(circ), ctx, i)) {
        if (running == 0) return;
        std::this_thread::yield();
      }
      sum += i;
    }
  };

  std::vector<std::thread> prods(np);
  for (std::size_t n = 0; n < np; ++n) prods[n] = std::thread(prod_call, n);
  std::vector<std::thread> conss(nc);
  for (auto &c : conss) c = std::thread(cons_call);

  for (auto &p : prods) p.join();
  for (auto &c : conss) c.join();

  EXPECT_EQ(sum, k * np * (loop_size * std::uint64_t(loop_size + 1)) / 2);
}

} // namespace

TEST(concurrent, prod_cons) {
  using namespace concur;

  /// @brief 1-1
  test_concur<prod_cons<trans::unicast, relation::single, relation::single>>(1, 1, 1);
  test_concur<prod_cons<trans::unicast, relation::single, relation::multi >>(1, 1, 1);
  test_concur<prod_cons<trans::unicast, relation::multi , relation::single>>(1, 1, 1);
  test_concur<prod_cons<trans::unicast, relation::multi , relation::multi >>(1, 1, 1);

  /// @brief 8-1
  test_concur<prod_cons<trans::unicast, relation::multi , relation::single>>(8, 1, 1);
  test_concur<prod_cons<trans::unicast, relation::multi , relation::multi >>(8, 1, 1);

  /// @brief 1-8
  test_concur<prod_cons<trans::unicast, relation::single, relation::multi >>(1, 8, 1);
  test_concur<prod_cons<trans::unicast, relation::multi , relation::multi >>(1, 8, 1);

  /// @brief 8-8
  test_concur<prod_cons<trans::unicast, relation::multi , relation::multi >>(8, 8, 1);
}