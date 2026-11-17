# Software Test Plan and Test Cases

Team 14 | Deliverable 1 | Part B | Version 1.1 | Updated 17 November 2026

Contains the baseline test approach and TC-01 to TC-12 with original preconditions/actions/expectations, actual observed results, status, and requirement traceability. The [before-testing version](Test_Plan_v1_Before.md) preserves the original version 1.0 expectations. The [full after-testing plan](Test_Plan_v2_After.md) also documents the later web scope and its test evidence.

Companion files: [SRS](SRS.md) (Part A), [test cases](Test_Cases.md) (Part B), and [SAD](SAD.md) (Part C).

## 1 Introduction

This document records validation of the console-based Online Quiz System against the identifiers in Part A. Original expected outcomes remain in each case; the separate actual-result and status columns report the recorded execution evidence. Planned methods and scope below retain the baseline test approach.

## 2 Scope and test items

Test items are the console executable, authentication service, question and quiz management, attempt/scoring logic, result access, and local repository. Tests use a disposable directory and known sample data. Network and browser behavior are outside scope.

## 3 Features to be tested

| Area | Requirements | Verification |
| --- | --- | --- |
| Accounts and access | FR-01, FR-02, SEC-01, SEC-02 | Register, sign in, deny role escalation; inspect persisted credential representation. |
| Quiz authoring | FR-03, FR-04 | Validate question fields, references, and admin edits. |
| Quiz delivery | FR-05, FR-06, FR-07, NFR-03 | Browse, answer, and recover from invalid input. |
| Scores and results | FR-08, FR-09, FR-10, SEC-03 | Known answer key, ownership, and admin summary. |
| Persistence and quality | FR-11, NFR-01, NFR-02, NFR-04, SEC-04 | Restart, failure injection, timing, and two platform builds. |

## 4 Features not to be tested

Online synchronization, timed proctoring, negative marking, multimedia questions, payment handling, and concurrent sessions are excluded because they are outside the release scope. The operating system’s access controls and the cryptographic library’s own implementation are not audited; the application’s use of those facilities is tested.

## 5 Test approach

Use black box console scenarios for observable behavior, unit tests for scoring and validation, integration tests for persistence and role checks, and manual review of rendered prompts. Reset the data directory before each independent test. Seed one administrator, two learners, one quiz with three questions, and a known answer key A, C, B. Record actual results, environment, tester, and date during execution.

### 5.1 Security validation

Verify that stored passwords are salted hashes, that direct admin service calls reject learner sessions, and that a learner cannot read another learner’s history. Use malformed persisted records in a disposable directory to check safe rejection. Never include actual user passwords in screenshots or test logs.

### 5.2 Entry and exit criteria

- Entry: build succeeds, isolated test directory exists, seed fixtures are available, and the requirement version matches this plan.

- Exit: all critical security and scoring cases pass; all planned cases are executed; remaining defects are documented with severity and a retest decision.

### 5.3 Environment and evidence

Record compiler and version, operating system, build command, seed files, observed console transcript, and test result. For NFR-01, collect at least 20 local transitions with 1,000 questions and 10,000 attempts on one documented machine and report the 95th percentile. For NFR-04, compile and run smoke tests on Windows and Linux.

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

## 7 Traceability and reporting

The trace column maps each case to the original 11 functional, four non-functional, and four security requirements. PASS statuses reflect the recorded evidence and the boundaries stated above. The original before-testing plan is preserved separately; rerun affected cases when production behavior changes. The [report index](../docs/reports/README.md) identifies frameworks, coverage, environment, and limitations.
