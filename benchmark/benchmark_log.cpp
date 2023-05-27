
#include "benchmark/benchmark.h"

#include "libimp/log.h"

namespace {

void imp_log_no_output(benchmark::State &state) {
  LIBIMP_LOG_([](auto &&) {});
  for (auto _ : state) {
    log.debug("hello log.");
  }
}

void imp_log_info(benchmark::State &state) {
  LIBIMP_LOG_([](auto &&) {});
  for (auto _ : state) {
    log.info("hello log.");
  }
}

} // namespace

BENCHMARK(imp_log_no_output);
BENCHMARK(imp_log_info);