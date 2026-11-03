# Software Requirements Specification — Online Quiz System

| | |
|---|---|
| Version | 1.1 |
| Date | 28-09-2026 |
| Language | C++ (C++17) |
| Team | _Name / USN ×4 — fill in_ |
| Standard | IEEE 830-1998 / ISO/IEC/IEEE 29148:2018 (structure) |

**Revision history**

| Ver | Date | Change |
|---|---|---|
| 1.0 | 27-09-2026 | Initial SRS |
| 1.1 | 28-09-2026 | Added persistence, credential model, quiz size, input formats; merged duplicate security items; added diagram |

---

## 1. Introduction

### 1.1 Purpose
Defines the requirements of the Online Quiz System, a console application for taking multiple-choice quizzes and managing the question bank. It is the basis for architecture, design, and testing.

### 1.2 Scope
A standalone, offline C++ console application with two roles:
- **Participant** — enters a name, answers MCQs, sees a score.
- **Administrator** — logs in, then adds, views, modifies, and deletes questions.

"Online" is the product name only; there is no network functionality. Out of scope: GUI, web/mobile, networking, participant history, timers, categories.

### 1.3 Definitions

| Term | Meaning |
|---|---|
| MCQ | Multiple-choice question: 1 text, 4 options (A–D), 1 correct option |
| Question bank | All stored questions (`questions.txt`) |
| Quiz | Up to the first 10 questions of the bank, in stored order |
| Score | Number of correct answers |
| Session | Period from successful admin login to logout |

### 1.4 References
1. IEEE Std 830-1998, *Recommended Practice for SRS*.
2. ISO/IEC/IEEE 29148:2018, *Requirements Engineering*.
3. SE Mini-Project Deliverables Part-1, course brief.

### 1.5 Overview
§2 describes the product, §3 the requirements, §4 security, §5 use cases, §6 traceability.

---

## 2. Overall Description

### 2.1 Product perspective
Self-contained executable. Reads and writes two local files:
- `questions.txt` — the question bank, one record per line: `text|A|B|C|D|correct`
- `admin.cfg` — admin username, salt, and password hash

### 2.2 Product functions
Participant: take quiz, view result. Administrator: login, add / view / modify / delete question, logout. System: validate input, persist data, handle errors.

### 2.3 Users
| User | Assumed skill |
|---|---|
| Participant | Basic computer literacy; can type at a console |
| Administrator | Knows the admin menu; holds valid credentials |

### 2.4 Constraints
- C++17, no third-party libraries.
- Console interface only.
- Runs on Windows (MinGW g++) and Linux (g++).

### 2.5 Assumptions and dependencies
- `admin.cfg` is provisioned before first admin login.
- Each question has exactly one correct option.
- A missing `questions.txt` is treated as an empty bank.

---

## 3. Specific Requirements

Priority: **H** high, **M** medium. Verify: **T** test, **I** inspection, **A** analysis.

### 3.1 External interfaces
| ID | Requirement |
|---|---|
| EI-01 | User interface: text console with numbered menus. Main menu: `1 Take Quiz`, `2 Administrator Login`, `3 Exit`. |
| EI-02 | Admin menu: `1 Add`, `2 View`, `3 Modify`, `4 Delete`, `5 Logout`. |
| EI-03 | Software interface: file I/O to `questions.txt` and `admin.cfg` via the C++ standard library only. |
| EI-04 | No hardware or network interface. |

### 3.2 Functional requirements

| ID | Requirement | Pri | Ver |
|---|---|---|---|
| FR-01 | The system shall require a participant name of 1–30 characters (letters, digits, spaces) before a quiz starts, and re-prompt if invalid. | H | T |
| FR-02 | The system shall start a quiz containing the first min(10, N) questions of the bank, in stored order, where N is the bank size. | H | T |
| FR-03 | The system shall display one question at a time with its four options labelled A–D. | H | T |
| FR-04 | The system shall accept exactly one letter A–D (case-insensitive) as the answer to each question. | H | T |
| FR-05 | On any other answer input, the system shall display "Invalid option" and re-prompt for the same question without advancing. | H | T |
| FR-06 | After each valid answer, the system shall display "Correct!" or "Wrong! Correct answer: X". | M | T |
| FR-07 | The system shall calculate the score as the count of correct answers. | H | T |
| FR-08 | After the last question, the system shall display the participant name and the score as `score/total`. | H | T |
| FR-09 | The system shall require an administrator username and password before showing the admin menu. | H | T |
| FR-10 | The system shall add a question only if: text is 1–200 characters; exactly four options of 1–100 characters each; correct answer is A–D; text and options contain no `\|` character. Otherwise it shall reject it with a message and re-prompt. | H | T |
| FR-11 | The system shall list all questions numbered from 1, showing text, options, and correct answer. | H | T |
| FR-12 | The system shall modify a question selected by its list number and re-apply the FR-10 rules to the new values. | H | T |
| FR-13 | The system shall delete a selected question only after the administrator confirms with `y`; any other input cancels. | H | T |
| FR-14 | The system shall end the admin session on logout and return to the main menu. | H | T |
| FR-15 | The system shall reject any main-menu or admin-menu input that is not a listed option and re-prompt. | H | T |
| FR-16 | If the bank has zero questions, the system shall refuse to start a quiz and display "No questions available". | H | T |
| FR-17 | The system shall save the question bank to `questions.txt` immediately after each add, modify, or delete, and load it at startup. | H | T |
| FR-18 | Selecting `3 Exit` shall terminate the program with exit code 0. | M | T |
| FR-19 | On load, the system shall skip malformed records and display a warning with the count of records skipped. | M | T |

### 3.3 Non-functional requirements

| ID | Category | Requirement | Pri | Ver |
|---|---|---|---|---|
| NFR-01 | Usability | Every input prompt shall state the accepted values, e.g. `Enter answer (A-D):`. | M | I |
| NFR-02 | Performance | With a bank of up to 100 questions, the next question shall display within 1 s of a valid answer. | M | T |
| NFR-03 | Performance | Program startup, including loading 100 questions, shall complete within 2 s. | M | T |
| NFR-04 | Reliability | The program shall not crash for any single input line of up to 1000 characters; longer input shall be truncated to that limit and treated as invalid. | H | T |
| NFR-05 | Determinism | The same question bank and answer sequence shall always yield the same score. | M | T |
| NFR-06 | Capacity | The system shall support up to 100 questions. Attempting to add a 101st shall be rejected with a message. | M | T |
| NFR-07 | Portability | The source shall compile without errors or warnings under `g++ -std=c++17 -Wall` on Linux and MinGW on Windows. | M | T |
| NFR-08 | Maintainability | Each component in the architecture (Quiz Manager, Question Manager, Auth Manager, Input Validator, Score Calculator, Storage Manager, Error Handler) shall be implemented in its own `.h`/`.cpp` pair. | M | I |

---

## 4. Security

### 4.1 Objectives
| ID | Objective |
|---|---|
| SO-01 | Protect administrative functions from unauthorised use. |
| SO-02 | Preserve the integrity of the question bank. |
| SO-03 | Preserve the confidentiality of administrator credentials. |

### 4.2 Requirements
| ID | Requirement | Objective | Ver |
|---|---|---|---|
| SEC-01 | The system shall verify administrator credentials against `admin.cfg` before granting a session. | SO-01 | T |
| SEC-02 | On failed login, the system shall deny access and show the same message "Invalid credentials" whether the username or the password was wrong. | SO-01, SO-03 | T |
| SEC-03 | After 3 consecutive failed logins, the system shall block further login attempts for 60 s. | SO-01 | T |
| SEC-04 | Admin passwords shall be stored only as a salted hash in `admin.cfg`, never in plaintext or in source code; the password shall not be echoed while typed. | SO-03 | I, T |
| SEC-05 | All console input shall be read with a length limit; unbounded reads (`gets`, unbounded `scanf("%s")`) are prohibited. | SO-01, SO-02 | I |
| SEC-06 | The system shall validate every added or modified question per FR-10 before it is stored. | SO-02 | T |
| SEC-07 | Add, view, modify, and delete operations shall execute only while an authenticated session is active. | SO-01, SO-02 | T |

---

## 5. Use Cases

Diagram: `usecase.puml`. Actors: Participant (P), Administrator (A).

### UC-01 Take Quiz — P
- **Pre:** Application running.
- **Main flow:** 1. P selects Take Quiz. 2. System asks name (FR-01). 3. System shows question and options (FR-03). 4. P enters A–D. 5. System shows Correct/Wrong (FR-06). 6. Repeat 3–5 for all questions. 7. System shows score (FR-08).
- **Alt A — invalid name/answer:** system shows error, re-prompts, resumes.
- **Alt B — empty bank:** system shows "No questions available" and returns to main menu (FR-16).
- **Post:** Score displayed; main menu shown.

### UC-02 Admin Login — A
- **Pre:** None.
- **Main flow:** 1. A selects Administrator Login. 2. System asks username and password. 3. System verifies (SEC-01). 4. System shows admin menu.
- **Alt A — bad credentials:** "Invalid credentials"; return to main menu; count failure (SEC-02).
- **Alt B — locked:** system shows remaining lock time (SEC-03).
- **Post:** Authenticated session active.

### UC-03 Add Question — A
- **Pre:** Authenticated.
- **Main flow:** 1. A selects Add. 2. System asks text, four options, correct letter. 3. System validates (FR-10). 4. System saves (FR-17). 5. System shows success.
- **Alt A — invalid data or bank full:** system rejects, states the reason, and re-prompts or returns to the admin menu.
- **Post:** Bank contains the new question.

### UC-04 View Questions — A
- **Pre:** Authenticated.
- **Main flow:** 1. A selects View. 2. System lists all questions (FR-11).
- **Alt A — empty bank:** "No questions available".

### UC-05 Modify Question — A
- **Pre:** Authenticated; bank not empty.
- **Main flow:** 1. A selects Modify. 2. System lists questions. 3. A enters question number. 4. A enters new values. 5. System validates (FR-12), saves, and shows success.
- **Alt A — invalid number or data:** error and re-prompt.

### UC-06 Delete Question — A
- **Pre:** Authenticated; bank not empty.
- **Main flow:** 1. A selects Delete. 2. System lists questions. 3. A enters number. 4. System asks `y/n`. 5. A enters `y`. 6. System deletes, saves, and shows success.
- **Alt A — not `y`:** deletion cancelled.
- **Alt B — invalid number:** error and re-prompt.

### UC-07 Admin Logout — A
- **Pre:** Authenticated.
- **Main flow:** 1. A selects Logout. 2. System ends session and shows main menu (FR-14).
- **Post:** No admin operation is possible without logging in again.

---

## 6. Traceability

Component names match `component.puml`. TC IDs are reserved for the Test Plan.

| Requirement | Use case | Component | Test case |
|---|---|---|---|
| FR-01, FR-05 | UC-01 | Input Validator | TC-01 |
| FR-02, FR-03, FR-04 | UC-01 | Quiz Manager | TC-02 |
| FR-06, FR-07, FR-08 | UC-01 | Quiz Manager, Score Calculator | TC-03 |
| FR-16 | UC-01 | Quiz Manager | TC-04 |
| FR-09 | UC-02 | Auth Manager | TC-05 |
| FR-10 | UC-03 | Question Manager, Input Validator | TC-06 |
| FR-11 | UC-04 | Question Manager | TC-07 |
| FR-12 | UC-05 | Question Manager | TC-08 |
| FR-13 | UC-06 | Question Manager | TC-09 |
| FR-14 | UC-07 | Auth Manager | TC-10 |
| FR-15, FR-18 | — | UI Controller, Input Validator | TC-11 |
| FR-17, FR-19 | UC-03/05/06 | Storage Manager | TC-12 |
| NFR-01 | — | UI Controller | TC-13 |
| NFR-02, NFR-03 | UC-01 | Quiz Manager, Storage Manager | TC-14 |
| NFR-04 | — | Input Validator, Error Handler | TC-15 |
| NFR-05 | UC-01 | Score Calculator | TC-16 |
| NFR-06 | UC-03 | Question Manager | TC-17 |
| NFR-07, NFR-08 | — | All | TC-18 (build/inspection) |
| SEC-01, SEC-02 | UC-02 | Auth Manager | TC-19 |
| SEC-03 | UC-02 | Auth Manager | TC-20 |
| SEC-04 | UC-02 | Auth Manager, Storage Manager | TC-21 |
| SEC-05 | — | Input Validator | TC-22 |
| SEC-06 | UC-03, UC-05 | Question Manager, Input Validator | TC-23 |
| SEC-07 | UC-03–UC-07 | Auth Manager, UI Controller | TC-24 |

| Security objective | Satisfied by |
|---|---|
| SO-01 | SEC-01, SEC-02, SEC-03, SEC-05, SEC-07 |
| SO-02 | SEC-05, SEC-06, SEC-07 |
| SO-03 | SEC-02, SEC-04 |

---

## Appendix A — Sample console flow

```
========================================
          ONLINE QUIZ SYSTEM
========================================
1. Take Quiz
2. Administrator Login
3. Exit
Enter choice (1-3): 1
Enter your name: Asha

Question 1 of 10
Which data structure follows FIFO?
A. Stack
B. Queue
C. Tree
D. Graph
Enter answer (A-D): B
Correct!
...
========================================
             QUIZ RESULT
========================================
Participant : Asha
Score       : 8/10
========================================
```

## Appendix B — Diagram files
| File | Content |
|---|---|
| `usecase.puml` | UML use case diagram |
| `component.puml` | Component diagram |
| `architecture_layered.puml` | Architecture 1: layered pattern |
| `architecture_security.puml` | Architecture 2: security zones |
