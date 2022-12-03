
#include "benchmark/benchmark.h"

#include "libimp/log.h"

namespace {

void imp_log_no_output(benchmark::State &state) {
  imp::log::grip log {__func__, {}};
  for (auto _ : state) {
    log.debug("hello log.");
  }
}

void imp_log_gripper(benchmark::State &state) {
  imp::log::grip log {__func__, {}};
  for (auto _ : state) {
    log.info("hello log.");
  }
}

} // namespace

BENCHMARK(imp_log_no_output);
BENCHMARK(imp_log_gripper);