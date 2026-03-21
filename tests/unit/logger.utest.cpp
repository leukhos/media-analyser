#include "logger.hpp"

#include <doctest/doctest.h>

#include <iostream>
#include <sstream>
#include <streambuf>

namespace ma = media_analyser;

class CerrCapture {
public:
  CerrCapture() : m_original{std::cerr.rdbuf(m_buffer.rdbuf())} {}
  ~CerrCapture() { std::cerr.rdbuf(m_original); }

  std::string str() const { return m_buffer.str(); }

private:
  std::ostringstream m_buffer;
  std::streambuf* m_original;
};

TEST_SUITE_BEGIN("unit");

TEST_CASE("StdErrLogger::log") {
  ma::StdErrLogger logger;
  CerrCapture cerr;
  std::string message{"log message"};

  SUBCASE("level == LogLevel::Debug") {
    logger.log(ma::LogLevel::Debug, message);

    const std::string output = cerr.str();
    CHECK(output.data() == doctest::Contains("DEBUG"));
    CHECK(output.data() == doctest::Contains(message.data()));
    CHECK(output.back() == '\n');
  }

  SUBCASE("level == LogLevel::Error") {
    logger.log(ma::LogLevel::Error, message);

    const std::string output = cerr.str();
    CHECK(output.data() == doctest::Contains("ERROR"));
    CHECK(output.data() == doctest::Contains(message.data()));
    CHECK(output.back() == '\n');
  }

  SUBCASE("level == LogLevel::Info") {
    logger.log(ma::LogLevel::Info, message);

    const std::string output = cerr.str();
    CHECK(output.data() == doctest::Contains("INFO"));
    CHECK(output.data() == doctest::Contains(message.data()));
    CHECK(output.back() == '\n');
  }

  SUBCASE("level == LogLevel::Warning") {
    logger.log(ma::LogLevel::Warning, message);

    const std::string output = cerr.str();
    CHECK(output.data() == doctest::Contains("WARNING"));
    CHECK(output.data() == doctest::Contains(message.data()));
    CHECK(output.back() == '\n');
  }

  SUBCASE("level label appears before message") {
    logger.log(ma::LogLevel::Debug, message);

    const std::string output = cerr.str();
    const auto level_pos = output.find("DEBUG");
    const auto message_pos = output.find(message);
    CHECK(level_pos < message_pos);
  }

  SUBCASE("successive calls accumulate output in order") {
    std::string second_message{"second message"};

    logger.log(ma::LogLevel::Debug, message);
    logger.log(ma::LogLevel::Info, second_message);

    const std::string output = cerr.str();
    const auto first_pos = output.find(message);
    const auto second_pos = output.find(second_message);
    CHECK(first_pos < second_pos);
  }

  SUBCASE("message is empty") {
    logger.log(ma::LogLevel::Debug, "");

    const std::string output = cerr.str();
    CHECK(output.data() == doctest::Contains("DEBUG"));
    CHECK(output.back() == '\n');
  }
}

TEST_SUITE_END();