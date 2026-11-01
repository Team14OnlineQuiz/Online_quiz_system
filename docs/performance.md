# NFR-01 performance check

Build normally, then run `build/quiz_benchmark.exe` on Windows or `./build/quiz_benchmark` on Linux from the repository root. A nonzero exit means setup failed or either nearest-rank 95th percentile exceeded 2,000 ms. Raw observations are written to `build/performance.csv`.

The benchmark creates and removes a disposable dataset containing 1,000 active questions and 10,000 completed attempts. It checks the loaded counts and takes 40 samples per operation using `std::chrono::steady_clock`. Scripted input excludes human waiting time.

- Menu samples include real Argon2id sign-in, administrator menu dispatch, reloading and rendering all 10,000 results, logout, and exit.
- Question samples use an indexed lookup from the loaded catalog, render the selected prompt, and accept an answer through `AnswerSession`.

Fixture generation and initial catalog loading are outside the interaction timings. The catalog is loaded once per application lifecycle; lookups do not rescan its file. The benchmark exercises the implemented components and is a regression check when the team's quiz authoring and attempt-writing modules are integrated. It does not claim those other stories are complete.

## Recorded local run

On 2026-11-01, Windows with GCC 14.2.0, C++17 `-O2`, and libsodium 1.0.22 produced:

| Operation | Samples | 95th percentile | Limit |
| --- | ---: | ---: | ---: |
| Administrator flow | 40 | 146.003 ms | 2,000 ms |
| Question lookup, rendering, and answer | 40 | 0.0091 ms | 2,000 ms |

Timings depend on the machine and concurrent load. CI reruns the same threshold on Windows and Linux; the executable prints its OS and compiler version for each run.
