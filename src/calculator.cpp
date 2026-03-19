// First-party headers
#include "calculator.hpp"

// Standard library headers
#include <stdexcept>

int Calculator::add(int first_value, int second_value) {
  return first_value + second_value;
}

int Calculator::subtract(int first_value, int second_value) {
  return first_value - second_value;
}

int Calculator::multiply(int first_value, int second_value) {
  return first_value * second_value;
}

double Calculator::divide(int first_value, int second_value) {
  if (second_value == 0) {
    throw std::invalid_argument("Division by zero");
  }

  return static_cast<double>(first_value) / second_value;
}
