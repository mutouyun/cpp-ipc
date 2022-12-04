#include <tuple>

#include "benchmark/benchmark.h"

#include "fmt/format.h"
#include "fmt/chrono.h"

#include "libimp/fmt.h"

namespace {

void imp_fmt_string(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = imp::fmt("hello world.hello world.hello world.hello world.hello world.");
  }
}

void imp_multi_fmt_string(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = imp::fmt("hello world.", "hello world.", "hello world.", "hello world.", "hello world.");
  }
}

void fmt_fmt_string(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("hello world.hello world.hello world.hello world.hello world.");
  }
}

void fmt_multi_fmt_string(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("{}{}{}{}{}", 
                              "hello world.", " hello world.", " hello world.", " hello world.", " hello world.");
  }
}

void imp_fmt_int(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = imp::fmt(654321);
  }
}

void imp_multi_fmt_int(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = imp::fmt(654321, 654321, 654321, 654321, 654321);
  }
}

void fmt_fmt_int(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("{}", 654321);
  }
}

void fmt_multi_fmt_int(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("{}{}{}{}{}", 654321, 654321, 654321, 654321, 654321);
  }
}

void imp_fmt_float(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = imp::fmt(654.321);
  }
}

void imp_multi_fmt_float(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = imp::fmt(654.321, 654.321, 654.321, 654.321, 654.321);
  }
}

void fmt_fmt_float(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("{}", 654.321);
  }
}

void fmt_multi_fmt_float(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("{}{}{}{}{}", 654.321, 654.321, 654.321, 654.321, 654.321);
  }
}

void imp_fmt_chrono(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = imp::fmt(std::chrono::system_clock::now());
  }
}

void fmt_fmt_chrono(benchmark::State &state) {
  for (auto _ : state) {
    std::ignore = fmt::format("{}", std::chrono::system_clock::now());
  }
}

} // namespace

BENCHMARK(imp_fmt_string);
BENCHMARK(fmt_fmt_string);
BENCHMARK(imp_fmt_int);
BENCHMARK(fmt_fmt_int);
BENCHMARK(imp_fmt_float);
BENCHMARK(fmt_fmt_float);

BENCHMARK(imp_multi_fmt_string);
BENCHMARK(fmt_multi_fmt_string);
BENCHMARK(imp_multi_fmt_int);
BENCHMARK(fmt_multi_fmt_int);
BENCHMARK(imp_multi_fmt_float);
BENCHMARK(fmt_multi_fmt_float);

BENCHMARK(imp_fmt_chrono);
BENCHMARK(fmt_fmt_chrono);
