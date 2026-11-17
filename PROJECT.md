# Quizora: Online Quiz System

Team 14's project supports student quiz practice and teacher assessment. This page is the central index for the application, requirements, planning, test evidence, videos, and evaluation preparation.

## Quick links

| Looking for | Open |
| --- | --- |
| Line, branch, and function coverage | [Coverage summary below](#test-results-and-coverage) and [full report index](docs/reports/README.md) |
| Test cases with actual observations | [Expected versus actual results](Deliverable1/Test_Cases.md) |
| Before and after test plans | [Before testing](Deliverable1/Test_Plan_v1_Before.md) / [After testing](Deliverable1/Test_Plan_v2_After.md) |
| Requirements | [Original SRS](Deliverable1/SRS.md) / [Atomic revision](Deliverable1/SRS_Atomic_Revision.md) |
| Architecture and diagrams | [Console SAD](Deliverable1/SAD.md) / [Website architecture](Deliverable1/SAD_Web_Addendum.md) |
| Sprint videos | [Sprint 1: one minute](docs/videos/sprint-1.mp4) / [Sprint 2: two minutes](docs/videos/sprint-2.mp4) |
| Evaluation checklist | [Evidence index](docs/evaluation/README.md) |
| Presentation and viva | [40-45 minute walkthrough](docs/evaluation/walkthrough.md) / [Individual contribution guide](docs/presentation-guide.md) |
| Backlog and issues | [Team14Project](https://github.com/orgs/Team14OnlineQuiz/projects/1) / [SRS-to-issue mapping](docs/evaluation/backlog-traceability.md) |
| CI integration | [CI setup below](#continuous-integration-ci) / [GitHub Actions](https://github.com/Team14OnlineQuiz/Online_quiz_system/actions) |

## What the application does

Students can register, sign in, browse quizzes by subject and grade, answer multiple-choice/true-false/short-answer questions, review feedback, and inspect personal progress. Administrators can create, edit, or archive quizzes, view class analytics, and inspect a student's submitted answers alongside the correct answers.

The starter website catalogue has 28 short quizzes across Mathematics, Science, English, and Computing for grades 6-12. Content is grouped into grade bands and can be extended by an administrator.

## Implementation and iterations

| Version | Implementation | Focus |
| --- | --- | --- |
| Original console, Iterations 1 and 2 | C++17, libsodium Argon2id, versioned local data files | Accounts, authorization, quiz/question maintenance, attempts, scoring, history, recovery, performance, Windows/Linux builds |
| Website, Iteration 3 | HTML/CSS/vanilla JavaScript, Python Flask, SQLite, Werkzeug scrypt | Role-specific pages, subject/grade discovery, mixed question formats, progress analytics, teacher management and answer review |

The console and website are separate applications with separate credentials and storage. The website is an explicit extension of the original console specification. [Scope, schedule, and evidence deviations](docs/evaluation/deviations.md) explain the differences.

## Run the website

From the repository root on Windows:

```powershell
python -m venv .venv
.venv/Scripts/python -m pip install -r web/requirements.txt
.venv/Scripts/python -m web.app --demo
```

Open <http://127.0.0.1:5000>. On Linux/macOS, use `.venv/bin/python`. If the environment already exists, run only the last command.

| Demo role | Email | Password |
| --- | --- | --- |
| Student | learner@quizora.test | Learn@12345 |
| Administrator | admin@quizora.test | Teach@12345 |

Demo history is marked as sample data. Newly registered accounts start with empty history; completed quiz attempts are saved. The server is configured for local demonstrations. See [web setup and deployment notes](web/README.md) for storage, administrator provisioning, and production-hosting requirements. Console build commands are in [README.md](README.md).

## Test results and coverage

Recorded on **17 November 2026**. These are measured reports for the documented source snapshot.

| Application | Passing tests | Line coverage | Branch coverage | Function coverage |
| --- | ---: | ---: | ---: | ---: |
| C++ console | 51 | 80.9% | 65.8% | 93.5% |
| Python web backend | 15 | 86.9% | 75.9% | Not separately reported |

- **C++:** [HTML coverage](docs/reports/cpp-coverage.html), [JSON](docs/reports/cpp-coverage.json), [XML](docs/reports/cpp-coverage.xml), [test log](docs/reports/cpp-tests.txt), [executable smoke](docs/reports/cpp-smoke.txt).
- **Web backend:** [HTML coverage](docs/reports/web/index.html), [JSON](docs/reports/web-coverage.json), [XML](docs/reports/web-coverage.xml), [JUnit test results](docs/reports/web-junit.xml).
- **Acceptance:** [12 cases with actual results and status](Deliverable1/Test_Cases.md), [printable results PDF](docs/reports/test-case-observations.pdf), [observations and CLI transcripts](docs/reports/acceptance-results.json).
- **Performance:** [benchmark output](docs/reports/performance.txt), [raw CSV](docs/reports/performance.csv), [method](docs/performance.md). At 1,000 questions and 10,000 attempts, menu p95 was 445.705 ms and question p95 was 0.0187 ms, both below the 2,000 ms limit.
- **Reproduction:** [environment and command manifest](docs/reports/environment.json), [report instructions](docs/reports/README.md), [pinned evidence dependencies](scripts/requirements-evidence.txt).

Line coverage measures executed lines; branch coverage measures exercised decision outcomes. C++ and Python tools have different branch definitions. Coverage excludes frontend JavaScript/CSS and third-party libraries; C++ exception/unreachable-branch exclusions are documented. Passing tests do not imply complete coverage.

**To view the HTML reports:** download/extract the repository or evaluation pack, then open `docs/reports/cpp-coverage.html` and `docs/reports/web/index.html` in a browser. GitHub's file viewer displays HTML source rather than running the report interface.

## Continuous integration (CI)

GitHub Actions automatically builds and tests changes on pushes to `main` and when pull requests are opened or updated. The C++ workflow also runs for the configured iteration/integration branches and supports manual dispatch; the web workflow also runs for `iteration-3/**` pushes.

| Workflow | Platforms | Automated checks |
| --- | --- | --- |
| [C++ build and requirements tests](.github/workflows/cpp.yml) | Linux and Windows (MSYS2/UCRT64) | Install compiler/libsodium, compile C++17 with warnings treated as errors, run 51 requirement tests, smoke-test the real executable, and verify the p95 performance limit at the required data volume |
| [Quizora web checks](.github/workflows/web.yml) | Linux and Windows | Install Python dependencies, run 15 pytest cases covering authentication/roles/grading/ownership/persistence/concurrent submissions, and check browser JavaScript syntax with Node.js |

Both workflows passed for commit `8664554` when checked on 17 November 2026: [C++ run](https://github.com/Team14OnlineQuiz/Online_quiz_system/actions/runs/44027873760) and [web run](https://github.com/Team14OnlineQuiz/Online_quiz_system/actions/runs/44027872446). Each run preserves its commit, job steps, result and logs.

To demonstrate CI, open a pull request's **Checks** tab or the repository's **Actions** tab, select a workflow run, and open its Linux/Windows job logs. A failed build, test assertion, JavaScript syntax check or benchmark makes the corresponding check fail; the owner can inspect the log, fix the change and rerun the checks. Whether a check blocks merging depends on separately configured repository rules.

Coverage collection is a separate reproducible operation using [scripts/collect_evidence.py](scripts/collect_evidence.py). The checked-in HTML/JSON/XML reports are recorded snapshots; the current workflows rerun the checks above rather than publishing fresh coverage reports. The demonstrated website runs locally; automatic deployment is outside these CI workflows.

Presentation explanation: "We integrated GitHub Actions with our repository so changes are built and tested automatically. Both the console and website are checked on Windows and Linux. This provides repeatable feedback on a commit or pull request before integration and helps identify regressions."

## Security and data integrity

The server enforces roles and result ownership, validates state-changing requests with CSRF tokens, withholds answer keys from learner quiz-start responses, and grades submitted answers against a saved run snapshot. A unique run identifier and transaction prevent duplicate completed results. Console repositories stage file updates and preserve the last committed data when a write fails. Detailed controls and limitations are in the SRS/SAD and [test-plan section 5.1](Deliverable1/Test_Plan_v2_After.md#51-security-validation).

## Sprint and evaluation evidence

The [one-minute Sprint 1](docs/videos/sprint-1.mp4) and [two-minute Sprint 2](docs/videos/sprint-2.mp4) videos demonstrate rebuilt historical C++ snapshots. They were recorded retrospectively on 17 November 2026 and show that recording date. [Video provenance](docs/videos/provenance.json) records source commits, hashes, durations, and successful stage transcripts. The website is demonstrated separately as Iteration 3.

Use the [evaluation index](docs/evaluation/README.md) to follow the faculty checklist. It identifies evidence that is present and remaining review items, including live Project-field verification, formal PR review records, and acceptance of documented deviations.
