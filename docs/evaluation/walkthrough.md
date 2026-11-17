# Quizora evaluation walkthrough

Team 14 | Prepared 17 November 2026 | Use with the evaluator's checklist

## 1 The overall explanation

"Good morning, sir. Our project is Quizora, an Online Quiz System. Students can practise quizzes, review their answers, and track progress. Teachers can manage content and inspect individual student attempts. Iterations 1 and 2 implemented the original C++ console specification. Iteration 3 extended the project into a separate Flask website. We will show the requirements, design, planning, working builds, tests, and each member's implementation evidence."

The strongest product value is the complete practice/feedback/teacher-review workflow. Demonstrate the same new attempt first as the student, then as the administrator. Describe the novelty as integration and usability, not a new grading algorithm or an unsupported claim that no other website offers these features.

## 2 A 40-45 minute order

The evaluator controls the pace; use this as a navigation plan.

| Approximate time | Open and explain |
| --- | --- |
| 0-3 minutes | Introduction, users, problem and console-to-web progression |
| 3-9 | Original SRS, atomic revision, FR/NFR examples, security objectives, use-case diagram |
| 9-14 | Before/after test plans, sections 3/4/5, security section 5.1, traceability |
| 14-19 | Console SAD and current web addendum; component view and two sequence diagrams |
| 19-24 | Project board, backlog mapping, points, assignments, iterations, actual dates |
| 24-27 | One-minute Sprint 1 and two-minute Sprint 2 recordings |
| 27-34 | Live learner workflow, result/progress, then administrator review and editor |
| 34-40 | Individual issues, selected PR diffs, integration decisions |
| 40-45 | Test results, coverage, frameworks, benchmark and viva follow-ups |

Use [the checklist evidence index](README.md) as the first browser tab. It links every required artifact and identifies remaining review items.

## 3 How to explain SRS, FRs, NFRs and atomic wording

The SRS says what the system must do. An FR is behavior such as creating an account or returning a learner's history. An NFR constrains quality, such as response time, recovery, usability, or portability. Security has explicit objectives and verifiable requirements.

Say: "We retain the original 19 parent requirements because the issues reference those IDs. The revision separates independent checks into child IDs, so we can verify them individually without changing historical issue ownership or estimates."

For example, FR-01 is refined into account creation, rejection of an empty username, rejection of a password below eight characters, and duplicate-username rejection. Each can have a different pass/fail result. Atomic does not mean removing context: a performance requirement still needs its operation, workload, machine and threshold.

For NFR-01, explain: "With 1,000 questions and 10,000 attempts, we measure 40 interactions per operation. At least 95% must complete within two seconds, excluding human waiting. The latest recorded menu p95 is 445.705 ms and question p95 is 0.0187 ms." This is the console benchmark, not a website-load claim. With 40 sorted observations, the nearest-rank p95 is the 38th observation.

Show security objectives SO-01 administration restrictions, SO-02 credential/result privacy, and SO-03 valid-state preservation. There are four security parent requirements, exceeding the checklist's minimum of two. The original SRS also retains its UML use-case diagram.

## 4 Why the test plan has two versions

Open [version 1.0 before testing](../../Deliverable1/Test_Plan_v1_Before.md), then [version 1.1 after testing](../../Deliverable1/Test_Plan_v2_After.md).

Say: "The original plan defines preconditions, actions and expected results. It does not claim that the tests already passed. The updated version records the integration/web changes, actual results, frameworks, report links and limitations. We retained both versions rather than overwriting the original expectations."

Sections 3, 4 and 5 explain what is tested, what is excluded, and how testing is performed. Section 5.1 is dedicated to security: salted hashes, direct role checks, ownership, malformed records, CSRF, answer-key secrecy and repeated submission. The original 12 TC IDs retain their SRS traceability. Web tests are added for the later scope.

A good concrete example is TC-07: with answer key A,C,B, submit A,D,B. The real console records one result with 2/3 and 66.7%. TC-08 then signs in as two different learners and verifies that each sees only their own score. Open `acceptance-results.json` to show actual observations and transcripts.

## 5 Architecture: what runs where

The original console has a menu/controller layer, domain services, and repository adapters. Explain that business rules can be tested without terminal input, while file persistence is isolated for failure testing and platform differences. Its SAD contains a component diagram, two UML sequence diagrams, internal C++ interfaces, error handling and requirement mappings.

The website uses HTML/CSS/vanilla JavaScript, a Python Flask API, and SQLite. It is a separate implementation, not a browser directly calling C++ methods. The browser handles display and interaction; the server enforces permissions, validates, grades and queries results. The original `.db` files and the web SQLite database are separate.

Show the web addendum's quiz sequence: start saves a snapshot -> learner receives questions without keys -> submit verifies owner and answers -> server grades -> transaction saves a unique attempt -> review returns saved feedback. A second sequence shows teacher class analytics followed by an individual student review.

Why these choices? Flask gives a small API appropriate to this local project. SQLite supplies durable local relational storage and transactions without a separate database server. The trade-off is that we have not demonstrated large-scale hosted concurrency. Snapshotting preserves grading when content changes. A unique run ID makes repeated submissions return the same result.

## 6 Project, backlog and Agile explanation

Open Team14Project manually. Browser automation access to GitHub is currently blocked by a saved permission, so the latest live Project field values were not rechecked automatically. The issue descriptions and authors were checked through the connected GitHub tools.

Show one path: FR-01 -> issue #1 -> three story points -> assigned owner -> Iteration 1 -> implementation commits -> PR #22 -> tests. Repeat quickly with NFR-01 -> #12 -> benchmark and results. Story points express relative effort/complexity/uncertainty; they are not hours or a productivity score. An iteration is a planned delivery interval; a backlog is the prioritized work list.

Explain Todo as planned work, In progress as actively implemented/verified work, and Done as work meeting its acceptance criteria. A closed issue alone does not prove every Project field is correct; check the visible board before the evaluation. Existing issue estimates are preserved in the traceability table.

The prescribed sprint dates were 19-23 October and 26-30 October. The implementation and integration records are in November; the web extension is dated 7 November. Acknowledge the deviation and provide the team's real explanation orally. The repository records factual dates and scope changes; it does not disclose personal reasons or alter dates.

PRs are merged, but the 12 implementation PRs have no submitted GitHub formal-review records. Explain the actual collaboration process without presenting a merge as a formal recorded approval. Exact IEEE-template compliance, acceptance of scope/date deviations, and the freeze requirement remain evaluator review points.

## 7 How to present the videos

Play [Sprint 1](../videos/sprint-1.mp4): registration, administrator question creation, quiz configuration, and learner discovery. It is exactly one minute and uses rebuilt historical commit `ad5cf8b`, whose recorded historical test suite has 30 passing tests.

Play [Sprint 2](../videos/sprint-2.mp4): content setup, learner attempt with invalid-input recovery, scoring, saved history after restart, and administrator results. It is exactly two minutes and uses rebuilt historical commit `e08a033`, whose historical test suite has 51 passing tests.

Both are silent recordings of actual executable output in a dedicated demo window, with scripted disposable inputs and visible recording date. They were recorded on 17 November as retrospective demonstrations of historical builds. They do not prove on-time October completion. Source commits, file hashes, binary hashes and successful stage transcripts are in the video provenance file.

## 8 Live website demonstration

Start from the repository root: `.venv/Scripts/python -m web.app --demo`, then open `http://127.0.0.1:5000`.

Student: `learner@quizora.test` / `Learn@12345`. Administrator: `admin@quizora.test` / `Teach@12345`.

Show the learner home, subject/grade/search filters, and Mathematics grade 6 "Number sense". Its starter key is 0.75, True, 40. Intentionally answer one incorrectly, then show the answer review and newest progress entry. The website displays 2/3 as 67%; the original console uses 66.7%.

Log out and use the administrator account. Open class analytics and the same attempt. Show student name/email, quiz, date, score, selected answers and correct answers. Open the quiz editor to demonstrate validated authoring. Sample history in the demo account is explicitly marked; a newly registered account starts empty. New submitted attempts are persisted.

## 9 Individual contributions and PRs

Akshay (`sickDoughnut-007`) has 12 closed assigned issues, totaling 56 story points. Iteration 1: #1 registration, #3 authentication, #7 recovery, #16 hashing. Iteration 2: #10 admin summary, #12 performance, #15 builds. Iteration 3: #32 authentication, #33 dashboard/filters, #34 mixed questions/grading, #35 analytics, #36 administrator management/validation.

For PR #22, open authentication/password code and explain stored-role sessions, generic login failure and Argon2id salts. For PR #23, explain administrator permission checks, the benchmark workload and Windows/Linux checks. For PR #37, explain Flask sessions, the saved quiz snapshot, server grading, analytics and the browser pages. For PR #38, show the administrator result regression: learner identity and selected/correct answers now appear in a teacher-oriented review. Choose two files you understand rather than scrolling through the whole PR.

Akshath's recorded issues cover discovery/persistence/authorization and learner history/ownership/recovery, with PRs #24/#25. Advitha's recorded issues and PRs #26-#31 cover question/quiz maintenance, record validation, prompts, question flow and scoring. Each person should show their own actual evidence and explain integration with the shared services. The later integrated application connects these contributions.

The fourth member's speaking role is requirements, architecture and testing methodology. Their explanation can be: "I will explain the design and verification approach. Requirements map to backlog items and test cases. The console separates its UI, services and repositories; the website separates browser presentation, Flask processing and SQLite storage. We use positive, negative, permission and persistence scenarios, and coverage reports show which code paths those tests exercised." This is a presentation role, not an invented implementation PR.

## 10 Reports and testing framework

Open [the report index](../reports/README.md). It contains 51 passing C++ tests, a passing executable smoke, original TC observations, 15 passing web cases, coverage HTML/JSON/XML, web JUnit XML, environment details and benchmark CSV.

C++ tests use a lightweight custom `TEST`/`REQUIRE` harness in `tests/test.h`. The CLI smoke and acceptance scenarios use Python subprocess assertions. Web tests use pytest plus Flask's test client, not Selenium. Temporary directories/databases isolate tests from the live classroom data. GitHub Actions runs Windows/Linux checks on main pushes and PRs.

GCC/gcov instrumentation and gcovr generate C++ coverage. Pytest-cov/coverage.py generate Python backend coverage. C++ line coverage is 80.9% and branch coverage 65.8%; web backend line coverage is 86.9% and branch coverage 75.9%. The terminal's web combined figure is 84.1%, which combines line and branch opportunities; it is not the separate line value.

Line coverage asks whether an executable line ran. Branch coverage asks whether decision outcomes ran. Passing tests verify their assertions; coverage measures exercised code. Neither establishes that all bugs are absent. Browser JavaScript/CSS and third-party libraries are outside these percentages. C++ compiler-generated unreachable/exception branches are excluded and documented. The HTML reports expose missing code, including web startup and some validation/error paths.

## 11 Short answers for follow-up questions

**How are passwords protected?** Console: libsodium Argon2id with random per-account salts. Website: Werkzeug scrypt. The stored hash is verified by the library; plaintext passwords are not stored.

**Why server-side grading?** The browser is untrusted. The server uses the saved answer key, while the quiz-start response omits it. A learner cannot submit a trusted score value.

**How does ownership work?** The server derives the account from its authenticated session and checks the attempt owner. An administrator has an explicitly authorized wider result view.

**What is idempotency?** Submitting the same run again returns the existing attempt instead of adding another result. A unique run ID and transaction support this, including concurrent retries.

**Why snapshots?** A teacher's later quiz edit must not change the questions or key used to grade an already-started run, nor alter a completed review.

**What is the difference between validation and testing?** Application validation rejects an invalid request while the app runs. Tests deliberately exercise such requests and assert the expected response/state.

**What is CI?** Automated checks run for code changes and PRs. A passing CI run provides build/test evidence for that commit; it does not mean the website is deployed online.

**What are the limits?** Local deployment; starter grade-band content; normalized exact-match short answers; separate console/web data; no proctoring, email verification, password reset or automatic migration. Explain these as the implemented scope.

Close with: "We have shown the requirement-to-issue-to-code-to-test chain, the working baseline builds, and the extended website. The remaining schedule, review-record and scope-approval points are documented for discussion."
