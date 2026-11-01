#pragma once
#include <istream>
#include <ostream>
#include <string>
namespace quiz {
enum class InputState { Value, Invalid, End };
struct InputLine { InputState state; std::string value; };
InputLine readLine(std::istream& input, std::size_t limit);
InputLine readSecret(std::istream& input);
void eraseSecret(std::string& value);
}
