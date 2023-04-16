
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
  struct header {
    concur::index_t circ_size;

    bool valid() const noexcept {
      return (circ_size > 1) && ((circ_size & (circ_size - 1)) == 0);
    }
  };

  /// \brief circ-size = 0
  EXPECT_EQ(concur::trunc_index(header{0}, 0), 0);
  EXPECT_EQ(concur::trunc_index(header{0}, 1), 0);
  EXPECT_EQ(concur::trunc_index(header{0}, 2), 0);
  EXPECT_EQ(concur::trunc_index(header{0}, 16), 0);
  EXPECT_EQ(concur::trunc_index(header{0}, 111), 0);
  EXPECT_EQ(concur::trunc_index(header{0}, -1), 0);

  /// \brief circ-size = 1
  EXPECT_EQ(concur::trunc_index(header{1}, 0), 0);
  EXPECT_EQ(concur::trunc_index(header{1}, 1), 0);
  EXPECT_EQ(concur::trunc_index(header{1}, 2), 0);
  EXPECT_EQ(concur::trunc_index(header{1}, 16), 0);
  EXPECT_EQ(concur::trunc_index(header{1}, 111), 0);
  EXPECT_EQ(concur::trunc_index(header{1}, -1), 0);

  /// \brief circ-size = 2
  EXPECT_EQ(concur::trunc_index(header{2}, 0), 0);
  EXPECT_EQ(concur::trunc_index(header{2}, 1), 1);
  EXPECT_EQ(concur::trunc_index(header{2}, 2), 0);
  EXPECT_EQ(concur::trunc_index(header{2}, 16), 0);
  EXPECT_EQ(concur::trunc_index(header{2}, 111), 1);
  EXPECT_EQ(concur::trunc_index(header{2}, -1), 1);

  /// \brief circ-size = 10
  EXPECT_EQ(concur::trunc_index(header{10}, 0), 0);
  EXPECT_EQ(concur::trunc_index(header{10}, 1), 0);
  EXPECT_EQ(concur::trunc_index(header{10}, 2), 0);
  EXPECT_EQ(concur::trunc_index(header{10}, 16), 0);
  EXPECT_EQ(concur::trunc_index(header{10}, 111), 0);
  EXPECT_EQ(concur::trunc_index(header{10}, -1), 0);

  /// \brief circ-size = 16
  EXPECT_EQ(concur::trunc_index(header{16}, 0), 0);
  EXPECT_EQ(concur::trunc_index(header{16}, 1), 1);
  EXPECT_EQ(concur::trunc_index(header{16}, 2), 2);
  EXPECT_EQ(concur::trunc_index(header{16}, 16), 0);
  EXPECT_EQ(concur::trunc_index(header{16}, 111), 15);
  EXPECT_EQ(concur::trunc_index(header{16}, -1), 15);

  /// \brief circ-size = (index_t)-1
  EXPECT_EQ(concur::trunc_index(header{(concur::index_t)-1}, 0), 0);
  EXPECT_EQ(concur::trunc_index(header{(concur::index_t)-1}, 1), 0);
  EXPECT_EQ(concur::trunc_index(header{(concur::index_t)-1}, 2), 0);
  EXPECT_EQ(concur::trunc_index(header{(concur::index_t)-1}, 16), 0);
  EXPECT_EQ(concur::trunc_index(header{(concur::index_t)-1}, 111), 0);
  EXPECT_EQ(concur::trunc_index(header{(concur::index_t)-1}, -1), 0);

  /// \brief circ-size = 2147483648 (2^31)
  EXPECT_EQ(concur::trunc_index(header{2147483648u}, 0), 0);
  EXPECT_EQ(concur::trunc_index(header{2147483648u}, 1), 1);
  EXPECT_EQ(concur::trunc_index(header{2147483648u}, 2), 2);
  EXPECT_EQ(concur::trunc_index(header{2147483648u}, 16), 16);
  EXPECT_EQ(concur::trunc_index(header{2147483648u}, 111), 111);
  EXPECT_EQ(concur::trunc_index(header{2147483648u}, -1), 2147483647);
}

namespace {

template <typename PC>
void test_unicast(std::size_t np, std::size_t nc) {
  LIBIMP_LOG_();
  log.info("\n\tStart with: ", imp::nameof<PC>(), ", ", np, " producers, ", nc, " consumers...");

  constexpr static std::uint32_t loop_size = 100'0000;

  concur::element<std::uint64_t> circ[32] {};
  PC pc;
  typename concur::traits<PC>::header hdr {imp::make_span(circ)};
  ASSERT_TRUE(hdr.valid());

  std::atomic<std::uint64_t> sum {0};
  std::atomic<std::size_t> running {np};

  auto prod_call = [&](std::size_t n) {
    typename concur::traits<PC>::context ctx {};
    for (std::uint32_t i = 1; i <= loop_size; ++i) {
      std::this_thread::yield();
      while (!pc.enqueue(imp::make_span(circ), hdr, ctx, i)) {
        std::this_thread::yield();
      }
      if (i % (loop_size / 10) == 0) {
        log.info("[", n, "] put count: ", i);
      }
    }
    --running;
  };
  auto cons_call = [&] {
    typename concur::traits<PC>::context ctx {};
    for (;;) {
      std::this_thread::yield();
      std::uint64_t v;
      while (!pc.dequeue(imp::make_span(circ), hdr, ctx, v)) {
        if (running == 0) return;
        std::this_thread::yield();
      }
      sum += v;
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

TEST(concurrent, unicast) {
  using namespace concur;

  /// \brief 1-1
  test_unicast<prod_cons<trans::unicast, relation::single, relation::single>>(1, 1);
  test_unicast<prod_cons<trans::unicast, relation::single, relation::multi >>(1, 1);
  test_unicast<prod_cons<trans::unicast, relation::multi , relation::single>>(1, 1);
  test_unicast<prod_cons<trans::unicast, relation::multi , relation::multi >>(1, 1);

  /// \brief 8-1
  test_unicast<prod_cons<trans::unicast, relation::multi , relation::single>>(8, 1);
  test_unicast<prod_cons<trans::unicast, relation::multi , relation::multi >>(8, 1);

  /// \brief 1-8
  test_unicast<prod_cons<trans::unicast, relation::single, relation::multi >>(1, 8);
  test_unicast<prod_cons<trans::unicast, relation::multi , relation::multi >>(1, 8);

  /// \brief 8-8
  test_unicast<prod_cons<trans::unicast, relation::multi , relation::multi >>(8, 8);
}

namespace {

template <typename PC>
void test_broadcast(std::size_t np, std::size_t nc) {
  LIBIMP_LOG_();
  {
    concur::element<std::uint64_t> circ[32] {};
    PC pc;
    typename concur::traits<PC>::header hdr {imp::make_span(circ)};
    ASSERT_TRUE(hdr.valid());

    auto push_one = [&, ctx = typename concur::traits<PC>::context{}](std::uint32_t i) mutable {
      return pc.enqueue(imp::make_span(circ), hdr, ctx, i);
    };
    auto pop_one = [&, ctx = typename concur::traits<PC>::context{}]() mutable {
      std::uint64_t i;
      if (pc.dequeue(imp::make_span(circ), hdr, ctx, i)) {
        return i;
      }
      return (std::uint64_t)-1;
    };
    auto pop_one_2 = pop_one;

    // empty queue pop
    ASSERT_EQ(pop_one(), (std::uint64_t)-1);

    // test one push & pop
    for (int i = 0; i < 32; ++i) {
      ASSERT_TRUE(push_one(i));
      ASSERT_EQ(pop_one(), i);
    }
    for (int i = 0; i < 100; ++i) {
      ASSERT_TRUE(push_one(i));
      ASSERT_EQ(pop_one(), i);
    }
    ASSERT_EQ(pop_one(), (std::uint64_t)-1);

    // test loop push & pop
    for (int i = 0; i < 10; ++i) ASSERT_TRUE(push_one(i));
    for (int i = 0; i < 10; ++i) ASSERT_EQ(pop_one(), i);
    ASSERT_EQ(pop_one(), (std::uint64_t)-1);

    // other loop pop
    for (int i = 0; i < 32; ++i) ASSERT_TRUE(push_one(i));
    for (int i = 0; i < 32; ++i) ASSERT_EQ(pop_one_2(), i);
    ASSERT_EQ(pop_one_2(), (std::uint64_t)-1);

    // overwrite
    ASSERT_TRUE(push_one(123));
    for (int i = 1; i < 32; ++i) {
      ASSERT_EQ(pop_one(), i);
    }
    ASSERT_EQ(pop_one(), 123);
    ASSERT_EQ(pop_one(), (std::uint64_t)-1);
  }

  log.info("\n\tStart with: ", imp::nameof<PC>(), ", ", np, " producers, ", nc, " consumers...");
  {
    struct Data {
      std::uint64_t n;
      std::uint64_t i;
    };
    concur::element<Data> circ[32] {};
    PC pc;
    typename concur::traits<PC>::header hdr {imp::make_span(circ)};
    ASSERT_TRUE(hdr.valid());

    constexpr static std::uint32_t loop_size = 10'0000;

    std::atomic<std::uint64_t> sum {0};
    std::atomic<std::size_t> running {np};
    std::array<std::atomic<std::size_t>, 64> counters {};

    auto prod_call = [&](std::size_t n) {
      typename concur::traits<PC>::context ctx {};
      for (std::uint32_t i = 1; i <= loop_size; ++i) {
        std::this_thread::yield();
        counters[n] = 0;
        for (std::uint32_t k = 1;; ++k) {
          ASSERT_TRUE(pc.enqueue(imp::make_span(circ), hdr, ctx, Data{n, i}));
          // We need to wait for the consumer to consume the data.
          if (counters[n] >= nc) {
            break;
          }
          std::this_thread::yield();
          if (k % (loop_size / 10) == 0) {
            log.info("[", n, "] put count: ", i, ", retry: ", k, ", counters: ", counters[n]);
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
      typename concur::traits<PC>::context ctx {};
      std::array<std::uint64_t, 64> last_i {};
      for (;;) {
        std::this_thread::yield();
        Data v;
        while (!pc.dequeue(imp::make_span(circ), hdr, ctx, v)) {
          if (running == 0) return;
          std::this_thread::yield();
        }
        // The v.i variable always increases.
        if (last_i[(std::size_t)v.n] >= v.i) {
          continue;
        }
        last_i[(std::size_t)v.n] = v.i;
        sum += v.i;
        ++counters[(std::size_t)v.n];
      }
    };

    std::vector<std::thread> prods(np);
    for (std::size_t n = 0; n < np; ++n) prods[n] = std::thread(prod_call, n);
    std::vector<std::thread> conss(nc);
    for (auto &c : conss) c = std::thread(cons_call);

    for (auto &p : prods) p.join();
    for (auto &c : conss) c.join();

    EXPECT_EQ(sum, np * nc * (loop_size * std::uint64_t(loop_size + 1)) / 2);
  }
}

} // namespace

TEST(concurrent, broadcast) {
  using namespace concur;

  /// \brief 1-1
  test_broadcast<prod_cons<trans::broadcast, relation::single, relation::multi>>(1, 1);
  test_broadcast<prod_cons<trans::broadcast, relation::multi , relation::multi>>(1, 1);

  /// \brief 8-1
  test_broadcast<prod_cons<trans::broadcast, relation::multi , relation::multi>>(8, 1);

  /// \brief 1-8
  test_broadcast<prod_cons<trans::broadcast, relation::single, relation::multi>>(1, 8);
  test_broadcast<prod_cons<trans::broadcast, relation::multi , relation::multi>>(1, 8);

  /// \brief 8-8
  test_broadcast<prod_cons<trans::broadcast, relation::multi , relation::multi>>(8, 8);
}