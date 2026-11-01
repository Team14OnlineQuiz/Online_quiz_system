#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
core=()
for source in src/*.cpp; do [[ "$source" == src/main.cpp ]] || core+=("$source"); done
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pedantic -O2 -Iinclude "${core[@]}" src/main.cpp $(pkg-config --cflags --libs libsodium) -o build/quiz
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pedantic -O2 -Iinclude "${core[@]}" tests/*.cpp $(pkg-config --cflags --libs libsodium) -o build/quiz_tests
