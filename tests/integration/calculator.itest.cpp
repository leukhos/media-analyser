// First-party headers
#include "calculator.hpp"

// Third-party headers
#include <doctest/doctest.h>

// Standard library headers
#include <stdexcept>

TEST_SUITE_BEGIN("implementation");

TEST_CASE("Calculator - functional test for basic arithmetic workflow") {
  // This test simulates a real-world usage scenario
  Calculator calculator;

  SUBCASE("performing multiple operations in sequence") {
    // Arrange
    int initial_value = 100;
    int add_amount = 50;
    int subtract_amount = 30;
    int multiply_factor = 2;

    // Act - Simulate a calculation workflow
    int after_add = calculator.add(initial_value, add_amount);
    int after_subtract = calculator.subtract(after_add, subtract_amount);
    int final_result = calculator.multiply(after_subtract, multiply_factor);

    // Assert - Verify the final result
    CHECK(final_result == 240); // (100 + 50 - 30) * 2
  }

  SUBCASE("calculating average using division") {
    // Arrange - Sum of values
    int sum = calculator.add(10, 20);
    sum = calculator.add(sum, 30);
    int count = 3;

    // Act - Calculate average
    double average = calculator.divide(sum, count);

    // Assert
    CHECK(average == doctest::Approx(20.0));
  }
}

TEST_CASE("Calculator - functional test for error handling") {
  Calculator calculator;

  SUBCASE("division by zero in a calculation chain") {
    // Arrange
    int numerator = calculator.multiply(5, 4); // 20

    // Act & Assert - Verify exception is thrown
    CHECK_THROWS_AS(calculator.divide(numerator, 0), std::invalid_argument);
  }

  SUBCASE("recovery after exception") {
    // Arrange
    Calculator calc;

    // Act - Try division by zero
    try {
      calc.divide(10, 0);
    } catch (const std::invalid_argument&) {
      // Exception caught, calculator should still work
    }

    // Assert - Calculator still functional after exception
    int result = calc.add(5, 5);
    CHECK(result == 10);
  }
}

TEST_SUITE_END();
