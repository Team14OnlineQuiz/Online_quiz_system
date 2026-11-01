#include "quiz/console_input.h"
#include <sodium.h>
#include <iostream>
#include <utility>
#ifdef _WIN32
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
namespace quiz {
InputLine readLine(std::istream& input, std::size_t limit) {
    std::string value;
    bool overflow = false;
    char c = 0;
    bool any = false;
    while (input.get(c)) {
        any = true;
        if (c == '\n') break;
        if (value.size() < limit) value.push_back(c); else overflow = true;
    }
    if (!any && !input) return {InputState::End, {}};
    if (!value.empty() && value.back() == '\r') value.pop_back();
    return {overflow ? InputState::Invalid : InputState::Value, std::move(value)};
}
InputLine readSecret(std::istream& input) {
    if (&input != &std::cin) return readLine(input, 1024);
#ifdef _WIN32
    const HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD original = 0;
    const bool hidden = _isatty(_fileno(stdin)) && GetConsoleMode(handle, &original)
        && SetConsoleMode(handle, original & ~ENABLE_ECHO_INPUT);
    auto value = readLine(input, 1024);
    if (hidden) { SetConsoleMode(handle, original); std::cout << '\n'; }
#else
    termios original{};
    bool hidden = false;
    if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &original) == 0) {
        auto mode = original;
        mode.c_lflag &= static_cast<tcflag_t>(~ECHO);
        hidden = tcsetattr(STDIN_FILENO, TCSANOW, &mode) == 0;
    }
    auto value = readLine(input, 1024);
    if (hidden) { tcsetattr(STDIN_FILENO, TCSANOW, &original); std::cout << '\n'; }
#endif
    return value;
}
void eraseSecret(std::string& value) {
    if (!value.empty()) sodium_memzero(value.data(), value.size());
    value.clear();
}
}
