#pragma once
#include <chrono>
#include <filesystem>
struct TempDirectory {
    std::filesystem::path path;
    TempDirectory() {
        static unsigned sequence = 0;
        path = std::filesystem::current_path() / "build" / ("test-data-"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
            + "-" + std::to_string(++sequence));
        std::filesystem::create_directories(path);
    }
    ~TempDirectory() { std::error_code ec; std::filesystem::remove_all(path, ec); }
};
