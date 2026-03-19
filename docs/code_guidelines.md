# C++ Code Guidelines

This document provides coding standards for this C++ project.

## Quick Reference

### Headers (Order & Grouping)
1. Precompiled headers (`#include "pch.h"`)
2. First-party headers (`#include "calculator.hpp"`)
3. Third-party headers (`#include <doctest/doctest.h>`)
4. Standard library (`#include <iostream>`)

### Test Naming (DocTest)
- Test cases: `TEST_CASE("Module - Scenario")`
- Subcases: `SUBCASE("specific scenario")`

### Benchmark Naming
- Pattern: `benchmark_component_operation_scenario`
- Example: `benchmark_vector_push_back_empty`

### Formatting
- LLVM style via clang-format
- 2-space indentation, 80 character line length
- Run: `clang-format -i src/**/*.{cpp,h} tests/**/*.{cpp,h} benches/**/*.cpp`

---

## Header Inclusion Rules

### Inclusion Policy
- **No implicit inclusion**: All dependencies must be explicitly included
- **No transitive dependencies**: Do not rely on headers included by other headers
- **Forward declarations**: Use when possible to reduce compilation dependencies
- **Group comments**: Always add group comment headers

### Header Order (Mandatory)

```cpp
// Precompiled header (if used)
#include "pch.h"

// First-party headers
#include "calculator.hpp"
#include "math_utils.h"

// Third-party headers
#include <doctest/doctest.h>

// Standard library headers
#include <iostream>
#include <vector>
```

- Sort alphabetically within each group
- Separate groups with blank lines

---

## Testing Guidelines (DocTest)

### Test Organization

This project uses two types of tests:

| Type | Location | Purpose |
|------|----------|---------|
| Unit tests | `src/*.cpp` (embedded) | Test implementation details |
| Functional tests | `tests/*.cpp` | Test public API |

### Unit Tests (Embedded in Source)

Unit tests live in source files and are stripped from production builds:

```cpp
// src/calculator.cpp
#include "calculator.hpp"

#include <doctest/doctest.h>

#include <stdexcept>

int Calculator::add(int a, int b) { return a + b; }

// Tests are conditionally compiled with CALCULATOR_ENABLE_TEST
#ifdef CALCULATOR_ENABLE_TEST
TEST_CASE("Calculator - addition") {
  Calculator calc;
  CHECK(calc.add(2, 3) == 5);
}
#endif
```

**CMake handling:**
- `CALCULATOR_ENABLE_TEST=OFF`: Test code is not compiled (excluded via `#ifdef`)
- `CALCULATOR_ENABLE_TEST=ON`: Test dependencies are installed via vcpkg, tests are compiled and run via CTest

### Functional Tests (tests/ Directory)

Test public API from a user's perspective:

```cpp
// tests/calculator.test.cpp
#include "calculator.hpp"

#include <doctest/doctest.h>

TEST_CASE("Calculator - workflow") {
  Calculator calc;
  int result = calc.add(100, 50);
  result = calc.subtract(result, 30);
  CHECK(result == 120);
}
```

### Test Naming Convention

Format: `TEST_CASE("Module - Scenario")`

```cpp
TEST_CASE("Calculator - addition with positive numbers")
TEST_CASE("Calculator - division by zero throws exception")
```

### Test Structure (AAA Pattern)

All tests use Arrange-Act-Assert:

```cpp
TEST_CASE("Calculator - addition") {
  Calculator calc;
  int expected = 8;

  int result = calc.add(5, 3);

  CHECK(result == expected);
}
```

### Exception Testing

```cpp
CHECK_THROWS_AS(calc.divide(10, 0), std::invalid_argument);
CHECK_THROWS_WITH(calc.divide(10, 0), "Division by zero");
```

### Mocking (Trompeloeil)

```cpp
#include <trompeloeil.hpp>

class MockCalculator : public CalculatorInterface {
public:
  MAKE_MOCK2(add, int(int, int), override);
};

TEST_CASE("Mock example") {
  MockCalculator mock;
  REQUIRE_CALL(mock, add(2, 3)).RETURN(5);
  CHECK(mock.add(2, 3) == 5);
}
```

---

## Benchmarking Guidelines

### Location
All benchmarks in `benches/` directory with `.benchmark.cpp` suffix.

### Naming Convention

Pattern: `benchmark_[component]_[operation]_[scenario]`

```cpp
// Basic
static void benchmark_vector_push_back(benchmark::State& state);

// With scenario
static void benchmark_vector_push_back_preallocated(benchmark::State& state);

// With size indicator
static void benchmark_sort_quicksort_random_1k(benchmark::State& state);
```

### Size Indicators
- `1k`, `1m`: Element count (~1,000 / ~1,000,000)
- `4kb`, `1mb`: Byte size

### Registration

```cpp
BENCHMARK(benchmark_vector_push_back);
BENCHMARK(benchmark_sort_quicksort)->Range(1<<10, 1<<18);
```
