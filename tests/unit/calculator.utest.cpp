// First-party headers
#include "calculator.hpp"

// Third-party headers
#include <doctest/doctest.h>

// Standard library headers
#include <stdexcept>

TEST_SUITE_BEGIN("unit");

TEST_CASE("Calculator - add") {
  Calculator calculator;

  SUBCASE("addition with positive numbers") {
    CHECK(calculator.add(2, 3) == 5);
  }

  SUBCASE("addition with negative numbers") {
    CHECK(calculator.add(-2, -3) == -5);
  }

  SUBCASE("addition with mixed sign numbers") {
    CHECK(calculator.add(-10, 5) == -5);
  }
}

TEST_CASE("Calculator - subtract") {
  Calculator calculator;

  SUBCASE("subtraction with positive numbers") {
    CHECK(calculator.subtract(5, 3) == 2);
  }

  SUBCASE("subtraction resulting in negative") {
    CHECK(calculator.subtract(10, 15) == -5);
  }
}

TEST_CASE("Calculator - multiply") {
  Calculator calculator;

  SUBCASE("multiplication with positive numbers") {
    CHECK(calculator.multiply(3, 4) == 12);
  }

  SUBCASE("multiplication with negative and positive") {
    CHECK(calculator.multiply(-3, 4) == -12);
  }

  SUBCASE("multiplication with zero") { CHECK(calculator.multiply(0, 4) == 0); }
}

TEST_CASE("Calculator - divide") {
  Calculator calculator;

  SUBCASE("division with even result") {
    CHECK(calculator.divide(10, 2) == doctest::Approx(5.0));
  }

  SUBCASE("division with remainder") {
    CHECK(calculator.divide(7, 2) == doctest::Approx(3.5));
  }

  SUBCASE("division with negative number") {
    CHECK(calculator.divide(-10, 2) == doctest::Approx(-5.0));
  }

  SUBCASE("division by zero throws exception") {
    CHECK_THROWS_AS(calculator.divide(10, 0), std::invalid_argument);
  }
}

TEST_SUITE_END();
