
#include <thread>
#include <future>
#include <atomic>
#include <vector>

#include "benchmark/benchmark.h"

#include "libconcur/circular_queue.h"

namespace {

void concur_queue_rtt(benchmark::State &state) {
  using namespace concur;
  circular_queue<std::int64_t, relation::single, relation::single> que [2];
  std::atomic_bool stop = false;
  auto producer = std::async(std::launch::async, [&stop, &que] {
    for (std::int64_t i = 0; !stop.load(std::memory_order_relaxed); ++i) {
      std::int64_t n {};
      while (!que[0].pop(n)) ;
      (void)que[1].push(i);
    }
  });

  for (auto _ : state) {
    (void)que[0].push(0);
    std::int64_t n {};
    while (!que[1].pop(n)) ;
  }

  stop = true;
  (void)que[0].push(0);
  producer.wait();
}

void concur_queue_1v1(benchmark::State &state) {
  using namespace concur;
  circular_queue<std::int64_t, relation::single, relation::single> que;
  std::atomic_bool stop = false;
  auto producer = std::async(std::launch::async, [&stop, &que] {
    for (std::int64_t i = 0; !stop.load(std::memory_order_relaxed); ++i) {
      (void)que.push(i);
      std::this_thread::yield();
    }
  });

  for (auto _ : state) {
    std::int64_t i;
    while (!que.pop(i)) std::this_thread::yield();
  }

  stop = true;
  producer.wait();
}

void concur_queue_NvN(benchmark::State &state) {
  using namespace concur;

  static circular_queue<std::int64_t, relation::multi, relation::multi> que;
  static std::atomic_int run = 0;
  static std::vector<std::thread> prods;

  if (state.thread_index() == 0) {
    run = state.threads();
    prods.resize(state.threads());
    auto producer = [] {
      for (std::int64_t i = 0; run.load(std::memory_order_relaxed) > 0; ++i) {
        (void)que.push(i);
        std::this_thread::yield();
      }
    };
    for (auto &p : prods) p = std::thread(producer);
  }

  for (auto _ : state) {
    std::int64_t i;
    while (!que.pop(i)) std::this_thread::yield();
  }
  run -= 1;

  if (state.thread_index() == 0) {
    for (auto &p : prods) p.join();
  }
}

} // namespace

BENCHMARK(concur_queue_rtt);
BENCHMARK(concur_queue_1v1);
BENCHMARK(concur_queue_NvN)->ThreadRange(1, 16);
