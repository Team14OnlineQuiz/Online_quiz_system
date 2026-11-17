# Software Requirements Specification: atomic revision

Version 1.1 | Prepared 17 November 2026 | Team 14

## 1 Introduction

### 1.1 Purpose

Specify independently verifiable behaviors and preserve traceability to the original 19 requirements and their GitHub issues. This is an evaluation revision prepared after implementation, not a replacement dated as if it existed before development. The [version 1.0 SRS](SRS.md) and original Word source remain unchanged.

### 1.2 Scope

The baseline is the local C++17 console application. The Iteration 3 extension is a separate browser application for subject/grade discovery, mixed question formats, progress analytics, and teacher review. The two applications have separate storage and credentials. There is no automatic data migration.

### 1.3 Definitions

A learner takes quizzes and reviews owned results. An administrator manages content and reviews class results. An atomic requirement specifies one behavior or measurable constraint that can be verified separately. Conditions and record fields may describe one behavior; independent actions receive separate IDs. A parent ID remains the GitHub planning item; its child IDs refine acceptance criteria without creating historical work items.

### 1.4 References

[Original SRS](SRS.md), [baseline test plan](Test_Cases.md), [updated test plan](Test_Plan_v2_After.md), [original SAD](SAD.md), [web SAD addendum](SAD_Web_Addendum.md), and [backlog traceability](../docs/evaluation/backlog-traceability.md). The supplied evaluator checklist asks for clear, measurable FRs/NFRs; the atomic decomposition also addresses the viva feedback reported by the team.

### 1.5 Document overview

Sections 2-3 define the environment and interfaces; sections 4-6 refine the original requirements; section 7 specifies the web extension; sections 8-9 provide use cases and traceability.

## 2 Overall description

The console uses presentation, controller, domain-service, and repository layers. The web extension uses a browser interface, a Flask HTTP API, and SQLite. Learners and administrators have distinct workspaces. Supported console builds are Windows and Linux with C++17 and libsodium. The web app requires Python 3.12 or later and a modern browser. A writable local data directory is required. Production internet hosting and cross-device synchronization are outside the demonstrated local deployment.

## 3 External interfaces

Console interaction uses numbered menus and A-D answers. Web interaction uses forms, catalogue filters, a quiz runner, result review, and analytics. Browser/API messages use JSON over same-origin HTTP in the local demo; internet deployment requires HTTPS. A keyboard and display are sufficient. Console data uses versioned local files; web data uses SQLite. Libraries and build/run commands are identified in the root and web READMEs.

## 4 Atomic functional requirements: console baseline

Each row is a refinement of an existing parent, not a claim of a new completed issue.

| Atomic ID | Requirement |
| --- | --- |
| FR-01.1 | The system shall create a learner account from valid registration details. |
| FR-01.2 | Registration shall reject an empty username. |
| FR-01.3 | Registration shall reject a password shorter than eight characters. |
| FR-01.4 | Registration shall reject an existing username. |
| FR-02.1 | Valid stored credentials shall establish an authenticated session. |
| FR-02.2 | The session role shall be obtained from the stored account. |
| FR-02.3 | Invalid usernames and invalid passwords shall produce the same generic failure. |
| FR-03.1 | An authorized administrator shall be able to create a valid question. |
| FR-03.2 | An authorized administrator shall be able to modify a question. |
| FR-03.3 | An authorized administrator shall be able to deactivate a question. |
| FR-03.4 | Question validation shall reject an empty prompt. |
| FR-03.5 | Question validation shall reject an empty answer option. |
| FR-03.6 | Question validation shall reject an answer key outside the four choices. |
| FR-04.1 | An authorized administrator shall be able to create a valid quiz. |
| FR-04.2 | An authorized administrator shall be able to modify a quiz. |
| FR-04.3 | Quiz validation shall reject an empty title. |
| FR-04.4 | Quiz validation shall reject a quiz with no active question. |
| FR-04.5 | Quiz validation shall reject an unknown question reference. |
| FR-05.1 | Quiz discovery shall display each available quiz's identifier, title, and question count. |
| FR-05.2 | A learner shall be able to select an available quiz by identifier. |
| FR-06.1 | An attempt shall present configured questions in their saved order. |
| FR-06.2 | The question prompt shall display the four configured choices. |
| FR-06.3 | The attempt shall record one valid selected choice for each answered question. |
| FR-07.1 | An answer outside uppercase A-D shall be rejected. |
| FR-07.2 | An invalid answer shall leave the current question unchanged. |
| FR-07.3 | An invalid answer shall not be stored as a selected choice. |
| FR-07.4 | Invalid input shall cause the same question to be prompted again. |
| FR-08.1 | Submission shall calculate the number of selected answers matching the answer key. |
| FR-08.2 | Percentage shall be calculated as 100 times correct count divided by question count. |
| FR-08.3 | The console shall display the percentage to one decimal place. |
| FR-09.1 | Learner history shall return the authenticated learner's completed attempts. |
| FR-09.2 | Each history record shall display quiz title, date, correct/total counts, and percentage. |
| FR-10.1 | The administrator summary shall include completed attempts across learners. |
| FR-10.2 | A summary record shall display username, quiz title, date, and score. |
| FR-10.3 | The administrator summary shall deny a learner session. |
| FR-11.1 | Account creation shall report success only after the account is saved. |
| FR-11.2 | Question changes shall report success only after the changes are saved. |
| FR-11.3 | Quiz changes shall report success only after the changes are saved. |
| FR-11.4 | Submission shall report success only after the completed attempt is saved. |
| FR-11.5 | Saved accounts shall be recoverable after a normal restart. |
| FR-11.6 | Saved questions shall be recoverable after a normal restart. |
| FR-11.7 | Saved quizzes shall be recoverable after a normal restart. |
| FR-11.8 | Saved completed attempts shall be recoverable after a normal restart. |

## 5 Atomic non-functional requirements: console baseline

| Atomic ID | Requirement and verification boundary |
| --- | --- |
| NFR-01.1 | On the documented reference machine, at least 95% of the measured menu transitions shall finish within two seconds with 1,000 questions and 10,000 attempts, excluding human input time. |
| NFR-01.2 | On that reference machine and dataset, at least 95% of measured question loads shall finish within two seconds, excluding human input time. |
| NFR-02.1 | A failed write shall report failure. |
| NFR-02.2 | A failed write shall leave the last committed data readable. |
| NFR-02.3 | File updates shall be staged in a temporary file. |
| NFR-02.4 | A staged file shall replace the committed file atomically where supported by the operating system. |
| NFR-03.1 | A menu or question prompt shall identify accepted input values. |
| NFR-03.2 | Invalid input shall produce a specific correction message. |
| NFR-03.3 | Invalid input shall not crash the application. |
| NFR-04.1 | The console source shall compile as C++17 on the documented Windows toolchain. |
| NFR-04.2 | The console source shall compile as C++17 on the documented Linux toolchain. |
| NFR-04.3 | Build commands shall be documented for each supported platform. |
| NFR-04.4 | Platform-specific persistence operations shall be isolated in the repository adapter. |

The NFR-01 measurement is a console benchmark; it is not a website latency or concurrent-user capacity claim.

## 6 Security objectives and atomic requirements

Objectives: SO-01 restrict administration to administrators; SO-02 protect credentials and learner-result privacy; SO-03 preserve valid committed state during invalid input or storage failure.

| Atomic ID | Requirement |
| --- | --- |
| SEC-01.1 | Stored password verification shall use a vetted adaptive password-hashing library. |
| SEC-01.2 | Each account's hash shall use an independently generated random salt. |
| SEC-01.3 | Password verification shall use the library's verification API. |
| SEC-01.4 | Persisted account records shall not contain plaintext passwords. |
| SEC-02.1 | The controller shall check the stored role before dispatching an administrator operation. |
| SEC-02.2 | Direct calls to administrator services shall deny learner sessions. |
| SEC-03.1 | History ownership shall be derived from the authenticated session. |
| SEC-03.2 | A learner history query shall exclude other learners' attempts. |
| SEC-03.3 | An arbitrary supplied owner identifier shall not authorize access to history. |
| SEC-04.1 | Repository loading shall reject malformed records. |
| SEC-04.2 | A malformed record shall produce a recoverable error. |
| SEC-04.3 | A malformed account record shall not create an administrator session. |

The console implements SEC-01 using libsodium Argon2id. The web extension independently uses Werkzeug scrypt; both use vetted adaptive hashing, but they do not share account files.

## 7 Iteration 3 web-extension requirements

These refine the acceptance criteria of issues #32-#36. The browser extension is a documented scope change from version 1.0.

| Atomic ID | Requirement | Parent issue |
| --- | --- | --- |
| IT3-01.1 | Web registration shall create a learner account. | #32 |
| IT3-01.2 | Web login shall authenticate a stored account. | #32 |
| IT3-01.3 | Logout shall clear the authenticated session. | #32 |
| IT3-01.4 | Navigation shall display the signed-in role's workspace. | #32 |
| IT3-01.5 | An administrator endpoint shall reject a learner request. | #32 |
| IT3-01.6 | A state-changing API request with an invalid CSRF token shall be rejected. | #32 |
| IT3-02.1 | The learner home shall display recent account activity. | #33 |
| IT3-02.2 | The catalogue shall filter quizzes by selected subject. | #33 |
| IT3-02.3 | The catalogue shall filter quizzes by selected grade from 6 through 12. | #33 |
| IT3-02.4 | The catalogue shall filter quizzes by entered search text. | #33 |
| IT3-02.5 | The catalogue shall show an empty state when no quiz matches the combined filters. | #33 |
| IT3-03.1 | The quiz runner shall accept answers to multiple-choice questions. | #34 |
| IT3-03.2 | The quiz runner shall accept answers to true/false questions. | #34 |
| IT3-03.3 | The quiz runner shall accept answers to short-answer questions. | #34 |
| IT3-03.4 | Incomplete submissions shall be rejected. | #34 |
| IT3-03.5 | The server shall grade answers against the saved run snapshot. | #34 |
| IT3-03.6 | Repeated submissions of the same run shall return the existing saved attempt. | #34 |
| IT3-03.7 | A learner quiz-start response shall omit answer keys. | #34 |
| IT3-03.8 | A submitted result shall provide the answer review and explanations. | #34 |
| IT3-04.1 | Learner progress shall aggregate only the signed-in learner's attempts. | #35 |
| IT3-04.2 | Progress shall report subject accuracy from saved attempts. | #35 |
| IT3-04.3 | Progress shall report daily activity using UTC dates. | #35 |
| IT3-04.4 | A new learner shall see empty history. | #35 |
| IT3-05.1 | An administrator shall be able to create a validated quiz. | #36 |
| IT3-05.2 | An administrator shall be able to edit an active quiz. | #36 |
| IT3-05.3 | An administrator shall be able to archive an active quiz. | #36 |
| IT3-05.4 | Class analytics shall aggregate saved attempts across learners. | #36 |
| IT3-05.5 | Administrator answer review shall identify the learner. | #36 / PR #38 |
| IT3-05.6 | Administrator answer review shall show each submitted and correct answer. | #36 / PR #38 |

## 8 Use cases

The original UML use-case diagram is retained below. The web extension adds subject/grade filtering, personal analytics, and detailed teacher review to the corresponding discovery/result flows.

![Original learner and administrator use cases](diagrams/use-case.png)

UC-01: learner registers, signs in, selects a quiz, answers, submits, and reviews an owned result. Alternative flows: invalid credentials, invalid answer, unavailable quiz, incomplete submission, or failed save.

UC-02: administrator signs in, creates or edits valid quiz content, reviews class activity, and opens a learner's submitted result. Alternative flows: invalid content or denied permissions.

## 9 Traceability and review

The parent-to-issue and issue-to-test mapping is in [backlog traceability](../docs/evaluation/backlog-traceability.md). Child IDs inherit the parent's implementation evidence; an existing broad test does not imply a separate assertion for every child. Tests and remaining coverage gaps are identified in [the post-testing plan](Test_Plan_v2_After.md) and [reports](../docs/reports/README.md). Faculty acceptance of this revised scope/format is a review decision, not an assertion made by the implementation team.
