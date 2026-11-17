# Software Test Plan and Test Cases

**Before-testing baseline:** original Part B, version 1.0. This comparison copy was prepared on 17 November 2026; its original expected results are not a historical execution report.

Team 14 | Deliverable 1 | Part B | Version 1.0 | 28 September 2026

Contains the original test approach, TC-01 to TC-12, expected results, and requirement traceability. These are planned cases, not an execution report. Extracted from Deliverable 1, version 1.0, dated 28 September 2026. Requirement IDs, test IDs, expected results, and diagrams retain the source baseline. The later Iteration 3 website is documented separately in web/README.md.

Companion files: [SRS](SRS.md) (Part A), [test cases](Test_Cases.md) (Part B), and [SAD](SAD.md) (Part C).

## 1 Introduction

This test plan defines how Team 14 will validate the console based Online Quiz System against the identifiers in Part A. The planned tests are specifications for execution after implementation; their outcomes are not claimed as passed in this document.

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

## 6 Planned test cases

### TC-01  Register learner

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Empty data; username alex | Register alex with a valid 8 character password, then repeat username | First succeeds; duplicate rejected; one account stored | FR-01, FR-11 |

### TC-02  Authenticate and protect password

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Seed learner alex | Sign in with wrong then right password; inspect account file | Wrong pair denied with generic message; right pair accepted; no plaintext password stored and unique salt exists | FR-02, SEC-01 |

### TC-03  Deny unauthorized authoring

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Learner session active | Invoke create question and create quiz operations | Both return access denied; no question or quiz change persists | FR-03, FR-04, SEC-02 |

### TC-04  Validate authoring and configuration

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Admin session; one active question | Try empty option and unknown question reference, then create valid question and quiz | Invalid entries rejected; valid data saved and reloaded after restart | FR-03, FR-04, FR-11 |

### TC-05  Browse and complete attempt

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Learner; 3 question quiz with key A C B | List quizzes; choose quiz; enter A C B | Quiz listed with count 3; each question displayed once; 3 answers stored | FR-05, FR-06 |

### TC-06  Recover from bad choice

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Attempt on first question | Enter X, then A | X rejected with guidance; same question reappears; only A recorded | FR-07, NFR-03 |

### TC-07  Score known answers

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| 3 question quiz with key A C B | Answer A D B and submit | 2 of 3 correct; displayed percentage 66.7%; one completed result | FR-08 |

### TC-08  Enforce result ownership

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Two learners each with one result | As learner A request history; attempt internal access to B history | Only A records returned; B records denied | FR-09, SEC-03 |

### TC-09  Administrator summary

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Two completed learner results | As admin open summary, then as learner attempt same operation | Admin sees both results; learner is denied | FR-10, SEC-02 |

### TC-10  Storage failure and malformed data

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Disposable data directory | Inject write failure; restart using malformed account record | Failure reported; prior committed data readable; malformed record rejected without creating admin session | NFR-02, SEC-04 |

### TC-11  Response time at load

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| 1,000 questions and 10,000 attempts | Measure at least 20 menu and question loads | 95th percentile is at most 2 seconds on the documented machine | NFR-01 |

### TC-12  Cross platform build

| Precondition | Action | Expected result | Trace |
| --- | --- | --- | --- |
| Windows and Linux C++17 toolchains | Build and execute one sign in and quiz smoke test on each | Both compile and complete smoke test using documented commands | NFR-04 |

## 7 Traceability and reporting

The trace column in each test case maps directly to Part A. Requirement coverage before execution is 11 functional, 4 nonfunctional, and 4 security requirements. Record actual status as Pass, Fail, or Blocked with defect reference and evidence location; this plan contains expected results only.
