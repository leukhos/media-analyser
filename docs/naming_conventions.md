# C++ Naming Conventions

## Quick Reference

| Element | Convention | Example |
|---------|------------|---------|
| Classes/Structs | PascalCase | `Calculator`, `HttpClient` |
| Variables | snake_case | `counter`, `file_name` |
| Functions/Methods | snake_case | `process_data()`, `get_name()` |
| Member variables | `m_` prefix | `m_value`, `m_is_valid` |
| Static members | `m_` prefix | `m_instance_count` |
| Global variables | `g_` prefix | `g_debug_mode` |
| Constants | SCREAMING_SNAKE_CASE | `MAX_BUFFER_SIZE` |
| Namespaces | snake_case | `data_processing`, `network` |
| Trait interfaces | `-able` suffix | `Drawable`, `Serializable` |
| Service interfaces | `I` prefix | `ILogger`, `ICalculator` |
| Enums | PascalCase | `enum class Color { Red, Green }` |
| Type aliases | PascalCase | `using StringList = ...` |
| Files | snake_case | `data_processor.cpp` |

---

## Types

```cpp
class DataProcessor { };           // PascalCase
struct Configuration { };          // PascalCase
enum class Status { Success, Failure };  // PascalCase enum and variants
using StringList = std::vector<std::string>;  // PascalCase alias
```

## Variables and Members

```cpp
class Calculator {
private:
  int m_value;                     // m_ prefix for members
  bool m_is_valid;
  static int m_instance_count;     // m_ prefix for static members too

public:
  static const int MAX_VALUE = 100;  // SCREAMING_SNAKE_CASE for constants
};

void process(int input_value) {    // snake_case parameter
  int local_result = 0;            // snake_case local
}

// Globals (avoid when possible, but use g_ prefix when needed)
static bool g_debug_mode = false;
```

**Rationale for prefixes**:
- `m_` immediately identifies member access, avoiding ambiguity with parameters
- `g_` flags global state access, making dependencies visible during code review
- Constants use SCREAMING_SNAKE_CASE without prefix (already distinct)

## Functions

```cpp
void parse_file();               // snake_case function
int calculate_sum();
```

## Namespaces

```cpp
namespace data_processing {        // snake_case for feature namespaces
  void process_data();
}

namespace network::tcp {           // Nested namespaces use snake_case
  class Client { };
}
```

## Error Types

```cpp
// Error types use "Error" suffix (aligned with std::runtime_error pattern)
class FileNotFoundError : public std::runtime_error { };
class ParseError : public std::runtime_error { };
```

## Interfaces

Two conventions based on interface purpose:

**Trait-like interfaces** (`-able` suffix) - small, capability-focused:
```cpp
class Drawable {
public:
  virtual void draw() = 0;
  virtual ~Drawable() = default;
};

class Serializable {
  virtual void serialize(std::ostream&) = 0;
  virtual void deserialize(std::istream&) = 0;
};
```

**Service/Component interfaces** (`I` prefix) - contracts for DI, mocking, plugins:
```cpp
class ILogger {
public:
  virtual void info(std::string_view message) = 0;
  virtual void error(std::string_view message) = 0;
  virtual ~ILogger() = default;
};

class ICalculator {
public:
  virtual int add(int a, int b) = 0;
  virtual int subtract(int a, int b) = 0;
  virtual ~ICalculator() = default;
};

// Mock implementations use the I-prefixed interface
class MockCalculator : public ICalculator { };
class MockLogger : public ILogger { };
```

**When to use which**:
- `-able`: Describes a capability ("what can it do?") - typically 1-3 methods
- `I` prefix: Defines a service contract ("what is it?") - used for dependency injection and mocking

## Files

| Type | Pattern | Example |
|------|---------|---------|
| Header | snake_case.h | `http_client.h` |
| Source | snake_case.cpp | `http_client.cpp` |
| Test | snake_case.test.cpp | `http_client.test.cpp` |
| Benchmark | snake_case.benchmark.cpp | `http_client.benchmark.cpp` |

**Conversion rule**: PascalCase class → snake_case file
- `HttpClient` → `http_client.h`
- `DataProcessor` → `data_processor.cpp`

---

## Avoid

```cpp
// Hungarian notation
int nCounter;              // Use: int counter;
std::string strName;       // Use: std::string name;

// No prefix for members
int value;                 // Use: int m_value;

// I prefix for trait-like interfaces
class IDrawable { };       // Use: class Drawable { };

// Exception suffix for errors
class IoException { };     // Use: class IoError { };
```

## Rationale

These conventions prioritize:
- **STL alignment**: snake_case for functions/variables matches `std::` style
- **Readability**: PascalCase types are visually distinct from snake_case values
- **Clarity**: `m_` and `g_` prefixes make scope immediately visible
- **Safety**: Global access (`g_`) is flagged as a code smell indicator
