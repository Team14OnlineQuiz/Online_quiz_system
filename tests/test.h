#pragma once
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
namespace test {
using Case = std::pair<std::string, std::function<void()>>;
inline std::vector<Case>& cases() { static std::vector<Case> values; return values; }
struct Register {
    Register(const char* name, std::function<void()> body) { cases().emplace_back(name, std::move(body)); }
};
inline void require(bool condition, const char* expression, int line) {
    if (!condition) throw std::runtime_error(std::string(expression) + " at line " + std::to_string(line));
}
}
#define TEST(name) static void name(); static test::Register register_##name(#name, name); static void name()
#define REQUIRE(...) test::require(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)
