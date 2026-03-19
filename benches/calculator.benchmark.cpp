// First-party headers
#include "calculator.hpp"

// Third-party headers
#include <benchmark/benchmark.h>

static void benchmark_calculator_add_basic(benchmark::State& state) {
  Calculator calculator;
  for (auto _ : state) {
    benchmark::DoNotOptimize(calculator.add(42, 17));
  }
}
BENCHMARK(benchmark_calculator_add_basic);

static void benchmark_calculator_subtract_basic(benchmark::State& state) {
  Calculator calculator;
  for (auto _ : state) {
    benchmark::DoNotOptimize(calculator.subtract(42, 17));
  }
}
BENCHMARK(benchmark_calculator_subtract_basic);

static void benchmark_calculator_multiply_basic(benchmark::State& state) {
  Calculator calculator;
  for (auto _ : state) {
    benchmark::DoNotOptimize(calculator.multiply(42, 17));
  }
}
BENCHMARK(benchmark_calculator_multiply_basic);

static void benchmark_calculator_divide_basic(benchmark::State& state) {
  Calculator calculator;
  for (auto _ : state) {
    benchmark::DoNotOptimize(calculator.divide(42, 17));
  }
}
BENCHMARK(benchmark_calculator_divide_basic);

static void benchmark_calculator_add_range(benchmark::State& state) {
  Calculator calculator;
  for (auto _ : state) {
    benchmark::DoNotOptimize(calculator.add(state.range(0), state.range(1)));
  }
}
BENCHMARK(benchmark_calculator_add_range)->Args({8, 32})->Args({64, 128})->Args({512, 1024});

BENCHMARK_MAIN();