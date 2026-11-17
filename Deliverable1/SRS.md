# Software Requirements Specification

Team 14 | Deliverable 1 | Part A | Version 1.1 | Updated 17 November 2026

Defines the functional, non-functional, and security requirements for the console baseline in user-story form. Every numbered requirement starts with "As a learner", "As an administrator", or "As a user". Original technical acceptance criteria, IDs, thresholds and traceability are retained. The [original version 1.0 SRS](SRS_v1_Baseline.md), dated 28 September 2026, is preserved separately. The [atomic revision](SRS_Atomic_Revision.md) supplies individually verifiable user stories and criteria, including the later website extension.

Companion files: [SRS](SRS.md) (Part A), [test cases](Test_Cases.md) (Part B), and [SAD](SAD.md) (Part C).

## 1 Introduction

### 1.1 Purpose

Specify a local console application in which an administrator maintains multiple choice questions and quizzes, learners take quizzes, and the system calculates and stores results. The primary audience is the development team, reviewer, and test team.

### 1.2 Scope

The first release supports one user interacting with the console at a time. It stores named users, question banks, quiz definitions, attempts, and scores on the same machine. Internet play, payments, proctoring, multimedia questions, live multiplayer, and synchronization across devices are out of scope.

### 1.3 Terms

| Term | Meaning |
| --- | --- |
| Learner | Registered user who can take quizzes and inspect own results. |
| Administrator | Authorized user who can manage questions, configure quizzes, and view summary results. |
| Question | Prompt with four choices and exactly one correct answer. |
| Attempt | One submitted or in progress quiz run belonging to one learner. |

## 2 Overall description

The system has a menu driven console interface, a controller for input and role checks, services for authentication and quiz logic, and a repository adapter for local files. A quiz references an ordered set of active question IDs. The administrator prepares quiz content; the learner signs in, selects a quiz, answers each question, submits, and sees a score.

### 2.1 Users and operating environment

- Learners use a keyboard and terminal capable of displaying UTF 8 text.

- Administrators work at the same console but see management commands after authentication.

- Target environment: a C++17 compiler and a writable local data directory. No network service or browser is required.

### 2.2 Assumptions and constraints

- Each question has four text choices and exactly one correct choice, identified internally as A through D.

- An attempt is scored once on submission; each correct answer earns one point and there is no negative marking.

- The application must preserve data across a normal restart. Local storage is not a defense against an operating system administrator with direct file access.

## 3 External interface requirements

The console shall show a numbered menu with instructions for every prompt, report invalid entries without terminating, and display quiz scores as both correct count and percentage. Files used for persistence are internal implementation details and shall not be edited by learners through the application.

## 4 Functional requirements

### FR-01  Registration

- As a learner, I want to register a learner account with a unique username and a valid password so that I can access quizzes and my saved results.

**Acceptance criteria:** The system shall create a learner account when a unique nonempty username and a password of at least 8 characters are provided; duplicate usernames shall be rejected.

### FR-02  Sign in

- As a user, I want to sign in using my registered credentials so that I can access the actions authorized for my stored role.

**Acceptance criteria:** The system shall authenticate a registered username and password before displaying learner or administrator actions; an invalid pair shall show a generic failure message.

### FR-03  Question maintenance

- As an administrator, I want to maintain a validated question bank so that I can provide correct and usable quiz content.

**Acceptance criteria:** An authenticated administrator shall create, edit, or deactivate a question only when its prompt and four choices are nonempty and exactly one correct choice is selected.

### FR-04  Quiz configuration

- As an administrator, I want to configure quizzes from active questions so that I can provide valid assessments.

**Acceptance criteria:** An authenticated administrator shall create or edit a quiz with a nonempty title and at least one active question; a quiz may not reference an unknown question.

### FR-05  Quiz discovery

- As a learner, I want to discover and select an available quiz so that I can choose what to practise.

**Acceptance criteria:** An authenticated learner shall see available quizzes with a title and question count and may select one by its displayed identifier.

### FR-06  Attempt flow

- As a learner, I want to answer the configured quiz questions in order so that I can complete an attempt.

**Acceptance criteria:** For a selected quiz the system shall present each configured question exactly once, with choices A through D, and record one selected choice per question.

### FR-07  Input recovery

- As a learner, I want invalid answers to be rejected without advancing the question so that I can correct my input without losing my place.

**Acceptance criteria:** When an answer is not A, B, C, or D, the system shall show an error and re-prompt the same question without recording the invalid input.

### FR-08  Scoring

- As a learner, I want my submitted answers to be scored accurately so that I can understand my performance.

**Acceptance criteria:** On submission the system shall calculate correctCount as the count of stored answers matching the answer key and percentage as 100 × correctCount / totalQuestions, rounded to one decimal place.

### FR-09  Own results

- As a learner, I want to view my own completed results so that I can track my progress privately.

**Acceptance criteria:** A learner shall see only their own completed attempts with quiz title, date, correct count, total count, and percentage.

### FR-10  Admin summary

- As an administrator, I want to view completed results across learners so that I can monitor their performance.

**Acceptance criteria:** An administrator shall see completed attempts with username, quiz title, date, and score; an ordinary learner shall not be able to invoke this operation.

### FR-11  Persistence

- As a user, I want successful changes to be saved before confirmation and available after restart so that I can rely on my committed data.

**Acceptance criteria:** The system shall save account, quiz, question, and completed attempt records before confirming successful create, edit, or submission operations, and reload them after restart.

## 5 Nonfunctional requirements

### NFR-01  Responsiveness

- As a user, I want menu transitions and question loads to meet the specified two-second response-time target so that I can use the application promptly at the required data volume.

**Acceptance criteria:** With at most 1,000 stored questions and 10,000 attempts on the reference machine, a menu transition or question load shall complete within 2 seconds for 95% of local interactions, excluding time waiting for user input.

### NFR-02  Recovery

- As a user, I want the last committed data to remain readable when a file write fails so that I can recover without losing confirmed work.

**Acceptance criteria:** If a file write fails, the operation shall report failure and leave the last committed data file readable; updates shall use a temporary file and atomic replacement where the operating system supports it.

### NFR-03  Usability

- As a user, I want clear input guidance and recoverable error messages so that I can correct invalid choices without an application crash.

**Acceptance criteria:** Every menu and question prompt shall show accepted input values; an invalid choice shall produce a specific correction prompt without an application crash.

### NFR-04  Portability

- As a user, I want the console application to run on supported Windows and Linux environments so that I can use the documented platform available to me.

**Acceptance criteria:** The source shall compile as C++17 on a supported Windows or Linux build with a documented build command; platform specific file handling shall be isolated in the repository adapter.

## 6 Security objectives and requirements

Objectives: SO-01 restrict question and quiz changes and aggregate result access to administrators; SO-02 protect stored credentials and avoid exposing a learner’s results to another learner; SO-03 preserve valid saved state when input or storage operations fail.

### SEC-01  Password protection

- As a user, I want my password protected with a vetted adaptive hash and an independent salt so that plaintext credentials are not stored.

**Acceptance criteria:** The system shall use a vetted password hashing library with a unique random salt per account and an adaptive hash algorithm such as Argon2id; it shall never persist plaintext passwords.

### SEC-02  Role checks

- As an administrator, I want administrator operations restricted to authorized administrator sessions so that learner accounts cannot perform privileged actions.

**Acceptance criteria:** The controller shall check the authenticated role on every administrator operation, including direct internal calls, and shall deny a learner account.

### SEC-03  Result ownership

- As a learner, I want access to my history bound to my authenticated identity so that other learners cannot retrieve my results.

**Acceptance criteria:** The result service shall compare the authenticated learner ID with the owner ID on every own history request; it shall not accept an arbitrary owner ID from console input.

### SEC-04  File validation

- As a user, I want malformed stored records rejected with a recoverable error so that corrupt data is not trusted as privileged accounts or valid scores.

**Acceptance criteria:** On loading local data, the repository shall reject malformed records and report a recoverable error rather than interpreting them as privileged accounts or valid scores.

## 7 Use case model

![Original use case diagram](diagrams/use-case.png)

*Figure 1  UML use case diagram for learner and administrator*

Main success path for Take quiz: sign in, browse a quiz, select it, answer each question, submit, and view the result. Alternate path: invalid answer repeats the current prompt. Precondition: an administrator has configured at least one available quiz.

## 8 Requirements acceptance summary

The requirements are accepted only after the test cases in Part B pass on the documented reference environment. Any implementation change that affects the behavior, score formula, storage, or security checks requires an updated requirement and test mapping.
