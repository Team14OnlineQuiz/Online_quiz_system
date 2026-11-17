# Software Test Plan: after-testing revision

Version 1.1 | Prepared 17 November 2026 | Team 14 | Status: executed evidence and documented limitations

## 1 Test-plan identifier and purpose

TP-QUIZ-1.1 updates the original version 1.0 plan after sprint integration and the Iteration 3 web extension. [Before-testing version](Test_Plan_v1_Before.md) preserves the submitted expected cases; this version adds actual results, web scope, frameworks, and report links. Version dates are retained honestly; this revision was prepared after implementation.

## 2 Test items and references

Items: console executable, authentication, authoring, quiz flow, scoring, history, repositories, and the Flask web API. References: [original SRS](SRS.md), [atomic revision](SRS_Atomic_Revision.md), [console SAD](SAD.md), [web SAD](SAD_Web_Addendum.md), and [backlog mapping](../docs/evaluation/backlog-traceability.md). The baseline production source is commit `ad39b8f`; evidence-preparation changes add reporting and an optional smoke executable path, without changing production application source.

## 3 Features to be tested

| Area | Requirements | Verification |
| --- | --- | --- |
| Console accounts and security | FR-01/02, SEC-01/02 | Registration, duplicates, generic login failure, salted hashes, direct-role denial |
| Authoring and discovery | FR-03/04/05 | Content validation, unknown references, saved question/quiz reload, learner catalogue |
| Attempt and score | FR-06/07/08 | Three-question A,C,B fixture, invalid X retry, A,D,B -> 2/3 -> 66.7% |
| History and administrator results | FR-09/10, SEC-03 | Separate learner histories, cross-learner admin report, unauthorized calls |
| Persistence, usability, data validation | FR-11, NFR-02/03, SEC-04 | Restart, fault-injected failed writes, prompts, malformed records |
| Performance and platforms | NFR-01/04 | 1,000 questions/10,000 attempts, 40 samples, Windows execution plus recorded Windows/Linux CI |
| Web accounts and navigation | IT3-01/02 | Flask auth/role tests; catalogue response checks; prior browser UI evidence |
| Web quiz, progress and management | IT3-03/04/05 | Three formats, grading, ownership, snapshots, concurrent retries, analytics, admin review |

## 4 Features not to be tested

Cloud-scale capacity, proctoring, email delivery, password reset, payments, synchronization, semantic/AI answer grading, and automatic console-to-web migration are outside the release. The cryptographic library's implementation is not audited. Reported code coverage excludes browser JavaScript/CSS, third-party libraries, and tests themselves. Earlier browser checks are visual/manual evidence, not an automated end-to-end coverage percentage. Local Windows results are not represented as a locally executed Linux build.

## 5 Test approach

Use black-box real-executable scenarios for observable console behavior; C++ unit/integration tests for direct service permissions and injected storage failure; pytest with Flask's test client and isolated temporary databases for web routes; and a repeatable benchmark for NFR-01. Browser UI navigation is demonstrated separately. Each executable scenario launches a fresh process against a disposable directory, so persistence is tested across restarts. Known answer key A,C,B and submitted A,D,B verify the original rounding requirement.

### 5.1 Security validation

This section is dedicated to the evaluator's security-validation criterion.

| Check | Expected result | Recorded evidence |
| --- | --- | --- |
| Salted password storage | No plaintext; independent salt per same-password account | TC-02 acceptance fixture; C++ password tests; web hash/restart test |
| Administrator authorization | Learner/guest denied in controller and direct services | C++ admin-authorization tests; web role test |
| Result ownership | Learner A cannot retrieve learner B's results | TC-08; web mixed-grading/ownership and progress tests |
| Malformed local records | Rejected without privileged account or valid invented score | C++ malformed-role and attempt-adapter tests |
| CSRF | Missing/incorrect token rejected for mutation requests | `test_csrf_login_logout_and_roles` |
| Answer-key secrecy before submission | Quiz-start response omits answer keys | `test_mixed_question_grading_idempotency_and_ownership` |
| Duplicate/concurrent submission | One completed attempt for a run | `test_concurrent_submission_writes_one_attempt` |

These checks validate application controls. They are not a penetration test, cryptographic certification, or proof that every security branch was covered.

### 5.2 Entry and exit criteria

Entry: build/dependencies available; isolated writable test data; original requirement IDs preserved. Exit for this evidence run: 51 C++ tests pass, 15 web cases pass, CLI smoke passes, recorded TC-01 to TC-12 evidence passes within its stated scope, and both p95 measurements meet two seconds. Full line/branch coverage is not an exit claim. Formal document acceptance remains an evaluator/team review decision.

### 5.3 Environment and framework

Windows laptop; GCC/gcov 14.2.0; C++17; official libsodium 1.0.22. Coverage builds use `-O0 -g --coverage`; the separate normal `-O2` benchmark is used for latency. C++ tests use the repository's lightweight `TEST`/`REQUIRE` harness, not GoogleTest. CLI scenarios use Python subprocess assertions. Web tests use Python 3.12.14, pytest 9.2.0, Flask's test client, pytest-cov 7.1.0, and coverage.py 7.17.3. C++ reports use gcovr 8.8. Exact commands/platform details: [environment.json](../docs/reports/environment.json).

## 6 Executed test cases: expected and actual results

Execution evidence recorded on **17 November 2026**. Actual results below describe observed output, persisted state, measured timings, and executed unit/CI checks. [Recorded observations and CLI transcripts](../docs/reports/acceptance-results.json), [C++ test log](../docs/reports/cpp-tests.txt), and [performance output](../docs/reports/performance.txt) support the results. The [before-testing plan](Test_Plan_v1_Before.md) retains the original expected-only cases.

### TC-01  Register learner

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Empty data; username alex | Register alex with a valid 8 character password, then repeat username | First succeeds; duplicate rejected; one account stored | The eight-character registration unit test created one learner and reloaded it successfully. The CLI displayed "Learner account saved."; repeating alex displayed "Username already exists." and left users.db unchanged. | PASS | FR-01, FR-11 |

### TC-02  Authenticate and protect password

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Seed learner alex | Sign in with wrong then right password; inspect account file | Wrong pair denied with generic message; right pair accepted; no plaintext password stored and unique salt exists | Wrong username and wrong password both displayed "Invalid credentials." Valid credentials displayed the learner menu. Inspection found three distinct Argon2id salts for three same-password accounts and no plaintext password. | PASS | FR-02, SEC-01 |

### TC-03  Deny unauthorized authoring

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Learner session active | Invoke create question and create quiz operations | Both return access denied; no question or quiz change persists | Direct create-question and create-quiz calls from learner/guest sessions returned AccessDenied. No question or quiz was added by those calls. The learner CLI displayed no administrator menu. | PASS | FR-03, FR-04, SEC-02 |

### TC-04  Validate authoring and configuration

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Admin session; one active question | Try empty option and unknown question reference, then create valid question and quiz | Invalid entries rejected; valid data saved and reloaded after restart | The unit test rejected an empty answer option without saving a question. The CLI rejected an empty prompt and unknown question ID 9999. Three valid questions and the configured quiz were visible after restarting the process. | PASS | FR-03, FR-04, FR-11 |

### TC-05  Browse and complete attempt

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Learner; 3 question quiz with key A C B | List quizzes; choose quiz; enter A C B | Quiz listed with count 3; each question displayed once; 3 answers stored | The catalogue listed "Acceptance fixture" with 3 questions. Each of the three questions appeared once. Answers A,C,B produced "Attempt saved." and "Score: 3/3". | PASS | FR-05, FR-06 |

### TC-06  Recover from bad choice

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Attempt on first question | Enter X, then A | X rejected with guidance; same question reappears; only A recorded | Entering X displayed "Invalid answer." and repeated question 1. A then advanced to question 2. Question 1 appeared twice, while each following question appeared once; invalid X was not recorded. | PASS | FR-07, NFR-03 |

### TC-07  Score known answers

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| 3 question quiz with key A C B | Answer A D B and submit | 2 of 3 correct; displayed percentage 66.7%; one completed result | Against key A,C,B, submitting A,D,B displayed "Attempt saved." once, "Score: 2/3", and "Percentage: 66.7%". Exactly one completed result was reported. | PASS | FR-08 |

### TC-08  Enforce result ownership

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Two learners each with one result | As learner A request history; attempt internal access to B history | Only A records returned; B records denied | Alex history showed only the 3/3 result; Blair history showed only the 2/3 result. A direct request by Alex for Blair history returned AccessDenied in the recorded ownership test. | PASS | FR-09, SEC-03 |

### TC-09  Administrator summary

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Two completed learner results | As admin open summary, then as learner attempt same operation | Admin sees both results; learner is denied | The administrator summary included named results for alex (3/3) and blair (2/3). Learner, guest and logged-out administrator calls returned AccessDenied in the recorded summary tests. | PASS | FR-10, SEC-02 |

### TC-10  Storage failure and malformed data

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Disposable data directory | Inject write failure; restart using malformed account record | Failure reported; prior committed data readable; malformed record rejected without creating admin session | Fault-injection tests kept the committed account, question, quiz and attempt files byte-for-byte unchanged and readable after restart. Malformed role and invalid attempt-score records were rejected without creating an administrator session. | PASS | NFR-02, SEC-04 |

### TC-11  Response time at load

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| 1,000 questions and 10,000 attempts | Measure at least 20 menu and question loads | 95th percentile is at most 2 seconds on the documented machine | With 1,000 questions and 10,000 attempts, 40 samples per operation gave menu p95 = 445.705 ms and question p95 = 0.0187 ms. Both were below the 2,000 ms limit. | PASS | NFR-01 |

### TC-12  Cross platform build

| Precondition | Action | Expected result | Actual result observed | Status | Trace |
| --- | --- | --- | --- | --- | --- |
| Windows and Linux C++17 toolchains | Build and execute one sign in and quiz smoke test on each | Both compile and complete smoke test using documented commands | The local Windows build and real-executable smoke test passed. The recorded main CI run also completed the C++17 build and sign-in/quiz smoke tests successfully on both Windows and Linux. Linux evidence comes from CI, not a local laptop run. | PASS | NFR-04 |

**Evidence boundaries:** TC-01 includes the executed eight-character registration unit test; TC-04 includes the executed empty-option validation unit test. TC-03, TC-08, TC-09 and TC-10 combine CLI observations with direct-service/fault-injection tests. TC-12 uses the [recorded Windows/Linux CI run](https://github.com/Team14OnlineQuiz/Online_quiz_system/actions/runs/44013570777) alongside the local Windows build and smoke test.

Additional web cases are recorded in [web-junit.xml](../docs/reports/web-junit.xml). Their named tests cover session/CSRF/roles, hashed persistence, mixed grading/idempotency/ownership, progress, administrator student review, edit/archive snapshots, seven invalid editor inputs, concurrent submission, and unknown endpoints/security headers. This yields 15 executed cases because one validation test is parameterized.

## 7 Pass/fail criteria and results

No unit/web/CLI assertion failed in the recorded run. Performance passed the specified workload and threshold. Coverage measures exercised code; a passing suite does not mean 100% code coverage or absence of defects. The updated tests do not establish production internet readiness or full browser compatibility. The report index identifies uncovered areas and tool-specific branch definitions.

## 8 Suspension and resumption

Suspend if compilation fails, fixtures cannot be created, test processes time out, or the recorded binary/source does not match the intended build. Fix the environment or defect, regenerate disposable fixtures, and rerun the affected suite. Preserve previous reports when comparing versions; do not relabel a failed run as passed.

## 9 Test deliverables

Before/after plans; C++ test log and smoke log; acceptance JSON; web JUnit XML; HTML/XML/JSON coverage; benchmark output/CSV; framework/environment manifest; GitHub issue/PR/CI snapshot; sprint recordings and recording provenance. Index: [docs/reports](../docs/reports/README.md).

## 10 Testing tasks

Install pinned evidence dependencies, build instrumented binaries, run unit/API tests and CLI smoke, execute known-answer acceptance scenarios, run the normal benchmark, generate reports, inspect video content, and review requirement mappings. Reproduction commands are in the report index.

## 11 Environmental needs

C++17 compiler, matching gcov, libsodium headers/library, Python virtual environment, writable build/report folders, and a browser for manual demonstrations. Reports are generated with isolated data; the live classroom database is not reset. CI supplies a separate Linux environment.

## 12 Responsibilities

Each issue owner explains their implemented code and PRs. Integration/verification evidence is shared project evidence. The fourth presenter's role is to explain requirements, architecture, and testing methodology; no unrecorded implementation PR is attributed to that presenter.

## 13 Staffing and training

All presenters should rehearse both roles, explain score calculation and ownership, distinguish test assertions from coverage, and locate their supporting PR/report. The architecture presenter should understand the separate console and web data stores and the scope change.

## 14 Schedule

The prescribed windows were Sprint 1: 19-23 October 2026 and Sprint 2: 26-30 October 2026. GitHub records later November implementation/integration and an additional Iteration 3. This evidence run was performed on 17 November. See [deviations](../docs/evaluation/deviations.md); dates are not backdated.

## 15 Risks and contingencies

Risks include incomplete branch coverage, manual-only browser evidence, late delivery, lack of recorded formal PR approvals, and the web scope extension. Use the original console build to demonstrate the submitted baseline, retain the later website as an explicit extension, keep recordings as fallback, and discuss the documented limits. Faculty acceptance of deviations and document formatting is not assumed.

## 16 Review and approval

Prepared for team review and evaluation. No faculty approval, peer review, or historical signature is invented. The supplied checklist requests IEEE-format documents; these numbered sections support its requested content, while exact institutional-template compliance remains a review item.
