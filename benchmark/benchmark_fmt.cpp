#include <tuple>

#include "benchmark/benchmark.h"

#include "fmt/format.h"
#include "fmt/chrono.h"

#include "libimp/fmt.h"

namespace {

void imp_fmt_string(benchmark::State &state) {
  for (auto _ : state) {
    imp::fmt("hello world. hello world. hello world. hello world. hello world.");
  }
}

void fmt_format_string(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("hello world. hello world. hello world. hello world. hello world.");
  }
}

void imp_fmt_int(benchmark::State &state) {
  for (auto _ : state) {
    imp::fmt(654321);
  }
}

void fmt_format_int(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("{}", 654321);
  }
}

void imp_fmt_float(benchmark::State &state) {
  for (auto _ : state) {
    imp::fmt(654.321);
  }
}

void fmt_format_float(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("{}", 654.321);
  }
}

void imp_fmt_chrono(benchmark::State &state) {
  for (auto _ : state) {
    imp::fmt(std::chrono::system_clock::now());
  }
}

void fmt_format_chrono(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("{}", std::chrono::system_clock::now());
  }
}

} // namespace

BENCHMARK(imp_fmt_string);
BENCHMARK(fmt_format_string);
BENCHMARK(imp_fmt_int);
BENCHMARK(fmt_format_int);
BENCHMARK(imp_fmt_float);
BENCHMARK(fmt_format_float);
BENCHMARK(imp_fmt_chrono);
BENCHMARK(fmt_format_chrono);
