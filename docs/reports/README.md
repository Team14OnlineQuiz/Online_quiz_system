# Test and coverage reports

Recorded 17 November 2026 against production source at `ad39b8f`, with evidence-preparation/reporting changes. These are actual generated results; percentages are not targets or fabricated completion values.

| Evidence | Result | Open |
| --- | --- | --- |
| C++ requirement tests | 51 tests, 0 failures | [Test log](cpp-tests.txt) |
| Real executable smoke | Passed | [Smoke log](cpp-smoke.txt) |
| Original acceptance scenarios | TC-01 to TC-12 passed within recorded boundaries | [Detailed observations/transcripts](acceptance-results.json) |
| Web tests | 15 passed, 0 failures | [JUnit report](web-junit.xml) |
| C++ line coverage | 80.9% (926/1145) | [HTML](cpp-coverage.html), [JSON](cpp-coverage.json), [XML](cpp-coverage.xml) |
| C++ branch coverage | 65.8% (1291/1961) | Same C++ coverage report |
| C++ function coverage | 93.5% (145/155) | Same C++ coverage report |
| Web line coverage | 86.9% (304/350) | [HTML](web/index.html), [JSON](web-coverage.json), [XML](web-coverage.xml) |
| Web branch coverage | 75.9% (88/116) | Same web coverage report |
| Performance | Menu p95 445.705 ms; question p95 0.0187 ms; both below 2,000 ms | [Output](performance.txt), [40-sample CSV](performance.csv) |
| Environment and commands | Python/compiler/gcov/platform, instrumentation flags, commands and exit codes | [Manifest](environment.json) |
| GitHub evidence | 24 closed requirements/stories; 12 merged PRs; formal review arrays empty | [Snapshot](github-evidence.json) |

## What the coverage means

Line coverage is the fraction of executable lines exercised. Branch coverage is the fraction of measured decision outcomes exercised. C++ and Python tools count branches differently; these percentages should not be compared as if they share a denominator. Pytest's combined coverage display is 84.1%; the separate web line and branch values above use their own denominators.

C++ scope includes instrumented production `src/*.cpp` and `include/quiz` headers run through the 51-test suite and real executable smoke. Tests, benchmark code, standard-library headers, and libsodium are excluded. Compiler-generated exception/unreachable branches are excluded using gcovr's documented options. The normal benchmark uses its separate optimized build, not the coverage build.

Web scope is `web/app.py` and `web/seed.py`, exercised by Flask's in-process test client. This does not measure browser JavaScript/CSS or browser end-to-end behavior. Remaining missing lines include command-line bootstrap/admin initialization and several validation/error paths. All missing lines and branches remain visible in the HTML/JSON report.

## Reproduce

From the repository root, create the documented virtual environment and install:

```powershell
.venv/Scripts/python -m pip install -r web/requirements.txt -r scripts/requirements-evidence.txt
$env:SODIUM_ROOT = 'C:\path\to\libsodium-win64'
./scripts/build.ps1
.venv/Scripts/python scripts/collect_evidence.py
./build/quiz_benchmark.exe | Tee-Object -FilePath docs/reports/performance.txt
Copy-Item build/performance.csv docs/reports/performance.csv
.venv/Scripts/python scripts/acceptance_scenarios.py
```

Save benchmark stdout to `docs/reports/performance.txt` when regenerating the acceptance report. The first recorded run used a separately completed web coverage run and `collect_evidence.py --skip-web`; the exact equivalent web command is below. Install matching compiler/gcov and libsodium-dev on Linux and omit `SODIUM_ROOT`.

```text
python -m pytest web -q --cov=web.app --cov=web.seed --cov-branch --cov-report=term-missing --cov-report=html:docs/reports/web --cov-report=json:docs/reports/web-coverage.json --cov-report=xml:docs/reports/web-coverage.xml --junitxml=docs/reports/web-junit.xml
```

Open HTML reports locally in a browser. GitHub's source-file view does not execute arbitrary report HTML. Reports are a recorded snapshot; rerun them after production changes.
