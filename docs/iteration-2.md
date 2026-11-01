# Akshay's Iteration 2 implementation

This branch builds on `codex/akshay-iteration-1` (PR #20).

| Issue | Implementation | Validation |
| --- | --- | --- |
| #10 FR-10 | Authorized administrator summary over validated completed attempt snapshots | Multi-learner results, stable evaluation counts, unfinished exclusion, guest/learner denial, malformed reload, real executable smoke |
| #12 NFR-01 | Indexed active-question reader and reproducible volume benchmark | 1,000 questions, 10,000 attempts, 40 samples per operation, p95 threshold of 2 seconds |
| #15 NFR-04 | C++17 build scripts, isolated file replacement adapter, Windows/Linux CI | Warning-clean builds, 24 requirement tests including sign-in/quiz-input/logout integration, executable smoke, performance gate |

The summary's `AttemptStore` and the quiz callback are explicit team integration boundaries. Quiz configuration, scoring, attempt writes, question authoring, and learner history remain with their assigned owners. The quiz smoke supplies a deterministic prompt snapshot through the real console callback; it verifies authentication and input handling without claiming the unfinished full quiz pipeline works.

See [storage-format.md](storage-format.md) for adapter contracts and [performance.md](performance.md) for the benchmark method and recorded local results.

## Windows reference build

Local environment: Windows x64 build 10.0.26300, GCC 14.2.0 (MSYS2 UCRT64), official libsodium 1.0.22 MinGW win64 archive. From the repository root:

```powershell
./scripts/build.ps1 -SodiumRoot C:\path\to\libsodium-win64
./build/quiz_tests.exe
python scripts/smoke_test.py
./build/quiz_benchmark.exe
```

The archive contains headers and the static `lib/libsodium.a`. The script links Windows system dependencies and uses `SODIUM_STATIC`. Do not mix 32-bit libraries with the 64-bit compiler. A Python 3 interpreter is needed only for the executable smoke test.

## Linux reference build

The GitHub Actions `linux` job supplies Ubuntu, GCC, pkg-config, libsodium development headers, and Python 3. It prints the exact kernel, compiler, and library versions in each run.

```bash
sudo apt-get update
sudo apt-get install g++ pkg-config libsodium-dev
bash scripts/build.sh
./build/quiz_tests
python3 scripts/smoke_test.py
./build/quiz_benchmark
```

The Windows CI job uses the same Bash script in MSYS2 UCRT64 with GCC, libsodium, pkgconf, and Python packages. Both platforms compile with `-std=c++17 -Wall -Wextra -Werror -pedantic -O2`. No generated binaries or runtime data are committed.

File replacement differences are confined to `account_store.cpp`: `MoveFileExW` on Windows and `std::filesystem::rename` on Linux. Terminal echo differences are confined to `console_input.cpp`; neither service nor domain logic calls a platform API.
