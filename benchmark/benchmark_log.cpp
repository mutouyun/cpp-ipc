
#include "benchmark/benchmark.h"

#include "libimp/log.h"

namespace {

void BM_imp_log_gripper(benchmark::State& state) {
  imp::log::gripper log {{}, __func__};
  for (auto _ : state) {
    log.info("hello log.");
  }
}

} // namespace

BENCHMARK(BM_imp_log_gripper);