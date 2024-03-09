
#include <array>
#include <cstdlib>
#include <cstddef>

#include "benchmark/benchmark.h"

#include "libpmr/new.h"

namespace {

template <typename T, std::size_t N>
class cache {
public:
  template <typename U>
  void push(U &&u) noexcept {
    data_[idx_++] = std::forward<U>(u);
  }

  T &pop() noexcept {
    return data_[--idx_];
  }

  bool at_begin() const noexcept {
    return idx_ == 0;
  }

  bool at_end() const noexcept {
    return idx_ == N;
  }

private:
  std::array<T, N> data_{};
  std::size_t idx_{};
};

template <typename P, std::size_t CacheSize = 128>
class test_suit {
  void next(std::size_t &idx) noexcept {
    idx = (idx + 1) % 3;
  }

public:
  ~test_suit() noexcept {
    for (auto &pts : pts_) {
      while (!pts.at_begin()) {
        P::deallocate(pts.pop());
      }
    }
  }

  bool test_allocate() noexcept {
    auto &pts = pts_[idx_a_];
    pts.push(P::allocate());
    if (pts.at_end()) {
      next(idx_a_);
      idx_d_ = idx_a_;
    }
    return ++allocated_ < CacheSize;
  }

  bool test_deallocate() noexcept {
    auto &pts = pts_[idx_d_];
    if (pts.at_begin()) {
      next(idx_d_);
      if (allocated_ == CacheSize) {
        allocated_ = CacheSize / 2;
        return true;
      }
      return allocated_ > 0;
    }
    P::deallocate(pts.pop());
    --allocated_;
    return true;
  }

private:
  cache<void *, CacheSize / 2> pts_[3];
  std::size_t idx_a_{};
  std::size_t idx_d_{};
  std::size_t allocated_{};
};

template <std::size_t AllocSize>
struct policy_malloc {
  static void *allocate() noexcept {
    return std::malloc(AllocSize);
  }

  static void deallocate(void *p) noexcept {
    std::free(p);
  }
};

template <std::size_t AllocSize>
struct policy_cpp_new {
  static void *allocate() noexcept {
    return new char[AllocSize];
  }

  static void deallocate(void *p) noexcept {
    delete[] static_cast<char *>(p);
  }
};

template <std::size_t AllocSize>
struct policy_pmr_new {
  static void *allocate() noexcept {
    return pmr::new$<std::array<char, AllocSize>>();
  }

  static void deallocate(void *p) noexcept {
    pmr::delete$(static_cast<std::array<char, AllocSize> *>(p));
  }
};

template <template <std::size_t> class P, std::size_t AllocSize>
void pmr_allocate(benchmark::State &state) {
  test_suit<P<AllocSize>> suit;
  for (auto _ : state) {
    if (suit.test_allocate()) continue;
    state.PauseTiming();
    while (suit.test_deallocate()) ;
    state.ResumeTiming();
  }
}

template <template <std::size_t> class P, std::size_t AllocSize>
void pmr_deallocate(benchmark::State &state) {
  test_suit<P<AllocSize>> suit;
  for (auto _ : state) {
    if (suit.test_deallocate()) continue;
    state.PauseTiming();
    while (suit.test_allocate()) ;
    state.ResumeTiming();
  }
}

} // namespace

BENCHMARK(pmr_allocate<policy_malloc, 8>)->ThreadRange(1, 16);
BENCHMARK(pmr_allocate<policy_malloc, 32>)->ThreadRange(1, 16);
BENCHMARK(pmr_allocate<policy_malloc, 128>)->ThreadRange(1, 16);
BENCHMARK(pmr_allocate<policy_malloc, 1024>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_malloc, 8>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_malloc, 32>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_malloc, 128>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_malloc, 1024>)->ThreadRange(1, 16);

BENCHMARK(pmr_allocate<policy_cpp_new, 8>)->ThreadRange(1, 16);
BENCHMARK(pmr_allocate<policy_cpp_new, 32>)->ThreadRange(1, 16);
BENCHMARK(pmr_allocate<policy_cpp_new, 128>)->ThreadRange(1, 16);
BENCHMARK(pmr_allocate<policy_cpp_new, 1024>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_cpp_new, 8>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_cpp_new, 32>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_cpp_new, 128>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_cpp_new, 1024>)->ThreadRange(1, 16);

BENCHMARK(pmr_allocate<policy_pmr_new, 8>)->ThreadRange(1, 16);
BENCHMARK(pmr_allocate<policy_pmr_new, 32>)->ThreadRange(1, 16);
BENCHMARK(pmr_allocate<policy_pmr_new, 128>)->ThreadRange(1, 16);
BENCHMARK(pmr_allocate<policy_pmr_new, 1024>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_pmr_new, 8>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_pmr_new, 32>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_pmr_new, 128>)->ThreadRange(1, 16);
BENCHMARK(pmr_deallocate<policy_pmr_new, 1024>)->ThreadRange(1, 16);

/*
Run on (16 X 2313.68 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1280 KiB (x8)
  L3 Unified 24576 KiB (x1)
------------------------------------------------------------------------------------------
Benchmark                                                Time             CPU   Iterations
------------------------------------------------------------------------------------------
pmr_allocate<policy_malloc, 8>/threads:1              28.9 ns         29.8 ns     29866667
pmr_allocate<policy_malloc, 8>/threads:2              28.9 ns         53.7 ns     12800000
pmr_allocate<policy_malloc, 8>/threads:4              13.4 ns         43.9 ns     14933332
pmr_allocate<policy_malloc, 8>/threads:8              11.3 ns         68.4 ns      8000000
pmr_allocate<policy_malloc, 8>/threads:16             5.07 ns         56.7 ns     14336000

pmr_allocate<policy_malloc, 32>/threads:1             44.0 ns         42.1 ns     19298462
pmr_allocate<policy_malloc, 32>/threads:2             27.4 ns         54.9 ns     12800000
pmr_allocate<policy_malloc, 32>/threads:4             11.7 ns         47.1 ns     17920000
pmr_allocate<policy_malloc, 32>/threads:8             5.96 ns         43.7 ns     21082352
pmr_allocate<policy_malloc, 32>/threads:16            4.09 ns         56.7 ns     17920000

pmr_allocate<policy_malloc, 128>/threads:1            45.8 ns         39.6 ns     16592593
pmr_allocate<policy_malloc, 128>/threads:2            36.9 ns         75.3 ns      9955556
pmr_allocate<policy_malloc, 128>/threads:4            16.3 ns         66.7 ns     11946668
pmr_allocate<policy_malloc, 128>/threads:8            10.7 ns         77.1 ns     13784616
pmr_allocate<policy_malloc, 128>/threads:16           7.87 ns         94.8 ns     14336000

pmr_allocate<policy_malloc, 1024>/threads:1           75.5 ns         78.8 ns      8726261
pmr_allocate<policy_malloc, 1024>/threads:2           49.6 ns         46.9 ns     20000000
pmr_allocate<policy_malloc, 1024>/threads:4           18.4 ns         40.8 ns     29866668
pmr_allocate<policy_malloc, 1024>/threads:8           6.56 ns         25.6 ns     29866664
pmr_allocate<policy_malloc, 1024>/threads:16          6.25 ns         56.6 ns     16000000
------------------------------------------------------------------------------------------
pmr_deallocate<policy_malloc, 8>/threads:1            18.6 ns         19.9 ns     47786667
pmr_deallocate<policy_malloc, 8>/threads:2            8.52 ns         16.2 ns     47157894
pmr_deallocate<policy_malloc, 8>/threads:4            4.75 ns         18.8 ns     40000000
pmr_deallocate<policy_malloc, 8>/threads:8            3.18 ns         24.7 ns     51200000
pmr_deallocate<policy_malloc, 8>/threads:16           2.94 ns         38.1 ns     16000000

pmr_deallocate<policy_malloc, 32>/threads:1           18.0 ns         16.7 ns     45875200
pmr_deallocate<policy_malloc, 32>/threads:2           8.76 ns         19.9 ns     47157894
pmr_deallocate<policy_malloc, 32>/threads:4           4.50 ns         14.6 ns     44800000
pmr_deallocate<policy_malloc, 32>/threads:8           2.70 ns         16.7 ns     59733336
pmr_deallocate<policy_malloc, 32>/threads:16          2.30 ns         36.6 ns     23893328

pmr_deallocate<policy_malloc, 128>/threads:1          17.9 ns         16.3 ns     34461538
pmr_deallocate<policy_malloc, 128>/threads:2          8.29 ns         21.0 ns     66901334
pmr_deallocate<policy_malloc, 128>/threads:4          4.40 ns         14.5 ns     40000000
pmr_deallocate<policy_malloc, 128>/threads:8          2.65 ns         20.1 ns     80000000
pmr_deallocate<policy_malloc, 128>/threads:16         2.95 ns         40.5 ns     24717248

pmr_deallocate<policy_malloc, 1024>/threads:1         19.0 ns         20.4 ns     34461538
pmr_deallocate<policy_malloc, 1024>/threads:2         8.62 ns         19.7 ns     38956522
pmr_deallocate<policy_malloc, 1024>/threads:4         4.70 ns         11.7 ns     64000000
pmr_deallocate<policy_malloc, 1024>/threads:8         2.80 ns         16.0 ns     80000000
pmr_deallocate<policy_malloc, 1024>/threads:16        2.45 ns         26.1 ns     27569232
------------------------------------------------------------------------------------------
Benchmark                                                Time             CPU   Iterations
------------------------------------------------------------------------------------------
pmr_allocate<policy_cpp_new, 8>/threads:1             69.5 ns         78.5 ns      8960000
pmr_allocate<policy_cpp_new, 8>/threads:2             24.0 ns         40.1 ns     17920000
pmr_allocate<policy_cpp_new, 8>/threads:4             10.4 ns         37.1 ns     29866668
pmr_allocate<policy_cpp_new, 8>/threads:8             5.39 ns         42.2 ns     21082352
pmr_allocate<policy_cpp_new, 8>/threads:16            3.40 ns         45.9 ns     16000000

pmr_allocate<policy_cpp_new, 32>/threads:1            29.4 ns         29.5 ns     34461538
pmr_allocate<policy_cpp_new, 32>/threads:2            24.8 ns         43.9 ns     14933334
pmr_allocate<policy_cpp_new, 32>/threads:4            11.5 ns         45.1 ns     16290908
pmr_allocate<policy_cpp_new, 32>/threads:8            5.26 ns         34.8 ns     25600000
pmr_allocate<policy_cpp_new, 32>/threads:16           5.82 ns         90.7 ns      5513840

pmr_allocate<policy_cpp_new, 128>/threads:1           74.0 ns         66.3 ns      8960000
pmr_allocate<policy_cpp_new, 128>/threads:2           37.7 ns         80.6 ns     14933334
pmr_allocate<policy_cpp_new, 128>/threads:4           15.1 ns         52.1 ns     13784616
pmr_allocate<policy_cpp_new, 128>/threads:8           9.84 ns         63.2 ns     12358624
pmr_allocate<policy_cpp_new, 128>/threads:16          8.05 ns          110 ns     11946672

pmr_allocate<policy_cpp_new, 1024>/threads:1          79.5 ns         82.8 ns     10000000
pmr_allocate<policy_cpp_new, 1024>/threads:2          27.2 ns         59.6 ns     14933334
pmr_allocate<policy_cpp_new, 1024>/threads:4          12.3 ns         37.5 ns     40000000
pmr_allocate<policy_cpp_new, 1024>/threads:8          10.3 ns         29.8 ns     39822224
pmr_allocate<policy_cpp_new, 1024>/threads:16         6.17 ns         39.1 ns     16000000
------------------------------------------------------------------------------------------
pmr_deallocate<policy_cpp_new, 8>/threads:1           17.9 ns         18.9 ns     50301754
pmr_deallocate<policy_cpp_new, 8>/threads:2           8.73 ns         15.4 ns     51794580
pmr_deallocate<policy_cpp_new, 8>/threads:4           4.26 ns         15.2 ns     40000000
pmr_deallocate<policy_cpp_new, 8>/threads:8           2.46 ns         17.9 ns     71680000
pmr_deallocate<policy_cpp_new, 8>/threads:16          2.20 ns         34.0 ns     27569232

pmr_deallocate<policy_cpp_new, 32>/threads:1          16.1 ns         16.5 ns     56000000
pmr_deallocate<policy_cpp_new, 32>/threads:2          8.84 ns         16.1 ns     68923076
pmr_deallocate<policy_cpp_new, 32>/threads:4          4.48 ns         19.5 ns     44800000
pmr_deallocate<policy_cpp_new, 32>/threads:8          2.65 ns         15.3 ns     35840000
pmr_deallocate<policy_cpp_new, 32>/threads:16         2.20 ns         35.3 ns     23893328

pmr_deallocate<policy_cpp_new, 128>/threads:1         18.3 ns         21.1 ns     40727273
pmr_deallocate<policy_cpp_new, 128>/threads:2         8.83 ns         15.6 ns     38956522
pmr_deallocate<policy_cpp_new, 128>/threads:4         4.51 ns         17.6 ns     40000000
pmr_deallocate<policy_cpp_new, 128>/threads:8         2.93 ns         18.4 ns     39841984
pmr_deallocate<policy_cpp_new, 128>/threads:16        2.85 ns         36.1 ns     16000000

pmr_deallocate<policy_cpp_new, 1024>/threads:1        19.6 ns         16.0 ns     49777778
pmr_deallocate<policy_cpp_new, 1024>/threads:2        8.90 ns         17.9 ns     43631304
pmr_deallocate<policy_cpp_new, 1024>/threads:4        4.93 ns         13.7 ns     40000000
pmr_deallocate<policy_cpp_new, 1024>/threads:8        2.72 ns         19.9 ns     80000000
pmr_deallocate<policy_cpp_new, 1024>/threads:16       2.23 ns         29.0 ns     39822224
------------------------------------------------------------------------------------------
Benchmark                                                Time             CPU   Iterations
------------------------------------------------------------------------------------------
pmr_allocate<policy_pmr_new, 8>/threads:1             17.6 ns         18.8 ns     49777778
pmr_allocate<policy_pmr_new, 8>/threads:2             9.16 ns         14.5 ns     34461538
pmr_allocate<policy_pmr_new, 8>/threads:4             4.82 ns         18.8 ns     40000000
pmr_allocate<policy_pmr_new, 8>/threads:8             2.73 ns         17.4 ns     71680000
pmr_allocate<policy_pmr_new, 8>/threads:16            2.10 ns         34.1 ns     24717248

pmr_allocate<policy_pmr_new, 32>/threads:1            17.5 ns         14.6 ns     44800000
pmr_allocate<policy_pmr_new, 32>/threads:2            8.88 ns         16.0 ns     38956522
pmr_allocate<policy_pmr_new, 32>/threads:4            4.71 ns         17.8 ns     44800000
pmr_allocate<policy_pmr_new, 32>/threads:8            2.57 ns         16.7 ns     44800000
pmr_allocate<policy_pmr_new, 32>/threads:16           2.08 ns         41.0 ns     16000000

pmr_allocate<policy_pmr_new, 128>/threads:1           18.5 ns         14.4 ns     44600889
pmr_allocate<policy_pmr_new, 128>/threads:2           10.7 ns         22.5 ns     38956522
pmr_allocate<policy_pmr_new, 128>/threads:4           5.09 ns         21.3 ns     44800000
pmr_allocate<policy_pmr_new, 128>/threads:8           2.78 ns         19.6 ns     59733336
pmr_allocate<policy_pmr_new, 128>/threads:16          2.33 ns         28.8 ns     26548144

pmr_allocate<policy_pmr_new, 1024>/threads:1          24.8 ns         27.9 ns     22400000
pmr_allocate<policy_pmr_new, 1024>/threads:2          12.8 ns         24.0 ns     35840000
pmr_allocate<policy_pmr_new, 1024>/threads:4          6.67 ns         26.5 ns     44800000
pmr_allocate<policy_pmr_new, 1024>/threads:8          4.28 ns         28.3 ns     29866664
pmr_allocate<policy_pmr_new, 1024>/threads:16         3.21 ns         41.0 ns     16000000
------------------------------------------------------------------------------------------
pmr_deallocate<policy_pmr_new, 8>/threads:1           17.1 ns         16.3 ns     47786667
pmr_deallocate<policy_pmr_new, 8>/threads:2           8.89 ns         18.1 ns     34461538
pmr_deallocate<policy_pmr_new, 8>/threads:4           4.50 ns         16.1 ns     35840000
pmr_deallocate<policy_pmr_new, 8>/threads:8           2.77 ns         17.3 ns     59733336
pmr_deallocate<policy_pmr_new, 8>/threads:16          1.98 ns         34.2 ns     16000000

pmr_deallocate<policy_pmr_new, 32>/threads:1          17.1 ns         14.8 ns     49777778
pmr_deallocate<policy_pmr_new, 32>/threads:2          8.70 ns         18.7 ns     56000000
pmr_deallocate<policy_pmr_new, 32>/threads:4          4.58 ns         14.3 ns     44800000
pmr_deallocate<policy_pmr_new, 32>/threads:8          2.72 ns         17.3 ns     89600000
pmr_deallocate<policy_pmr_new, 32>/threads:16         2.04 ns         30.5 ns     25600000

pmr_deallocate<policy_pmr_new, 128>/threads:1         17.2 ns         15.3 ns     44800000
pmr_deallocate<policy_pmr_new, 128>/threads:2         8.67 ns         15.7 ns     52705882
pmr_deallocate<policy_pmr_new, 128>/threads:4         4.67 ns         18.7 ns     35840000
pmr_deallocate<policy_pmr_new, 128>/threads:8         2.76 ns         18.0 ns     39822224
pmr_deallocate<policy_pmr_new, 128>/threads:16        2.04 ns         28.3 ns     27569232

pmr_deallocate<policy_pmr_new, 1024>/threads:1        17.2 ns         21.5 ns     32000000
pmr_deallocate<policy_pmr_new, 1024>/threads:2        8.74 ns         16.0 ns     38956522
pmr_deallocate<policy_pmr_new, 1024>/threads:4        4.84 ns         16.7 ns     44800000
pmr_deallocate<policy_pmr_new, 1024>/threads:8        2.73 ns         16.0 ns     44800000
pmr_deallocate<policy_pmr_new, 1024>/threads:16       2.04 ns         37.1 ns     16000000
*/