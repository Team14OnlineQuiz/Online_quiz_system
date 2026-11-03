# Software Architecture & Design Specification — Online Quiz System

| | |
|---|---|
| Document ID | OQS-SADS-1.0 |
| Date | 28-09-2026 |
| Based on | `Online_Quiz_System_SRS.md` v1.1 |
| Format | IEEE 1016 (adapted) |
| Language | C++17 |
| Team | _Name / USN ×4 — fill in_ |

---

## 1. Introduction

### 1.1 Purpose
Describes the architecture and detailed design that realise the SRS, so that implementation and the Test Plan (`Test_Plan.md`) can proceed.

### 1.2 Scope
A standalone C++ console application: participant quiz and administrator question management, persisted in two local files.

### 1.3 Definitions
| Term | Meaning |
|---|---|
| Status | Enumerated result code returned by every component operation |
| Session | Object that records whether an administrator is authenticated |
| Bank | In-memory list of questions, mirrored in `questions.txt` |

### 1.4 References
1. `Online_Quiz_System_SRS.md` v1.1
2. `Test_Plan.md` v1.0
3. Diagram sources: `usecase.puml`, `component.puml`, `architecture_layered.puml`, `architecture_security.puml`, `sequence_take_quiz.puml`, `sequence_admin_login.puml`, `sequence_add_question.puml`

---

## 2. Architecture

### 2.1 Drivers
| Driver | Source | Effect |
|---|---|---|
| Separate responsibilities into identifiable modules | NFR-08 | One component per `.h`/`.cpp` pair |
| Never crash on bad input | NFR-04, FR-05, FR-15 | Validate at the boundary; status codes, no unchecked input |
| Admin operations only when authenticated | SEC-01, SEC-07 | Central authentication component; session token required by every admin call |
| Data survives restarts | FR-17 | Dedicated storage component with safe writes |
| Portable, no third-party libraries | NFR-07 | C++17 standard library only |

### 2.2 Architecture pattern
**Layered (3-tier)**: Presentation → Application (business) → Data Access, with an Error Handler as a cross-cutting concern. See `architecture_layered.puml`.

Rules:
1. Calls go downward only; no upward calls or callbacks.
2. Only the Storage Manager touches files.
3. Only the UI Controller reads or writes the console.

| Alternative | Why not |
|---|---|
| Single-file procedural program | Fails NFR-08; hard to test in units |
| MVC | Extra controller/view indirection for a text menu; layered gives the same separation with less overhead |

### 2.3 Component diagram and description
Diagram: `component.puml`. Eight components:

| Component | Layer | Responsibility | Key requirements |
|---|---|---|---|
| UI Controller | Presentation | Menus, prompts, display; bounded line reading; password reading without echo; orchestrates use cases | EI-01, EI-02, FR-15, FR-18, NFR-01, SEC-04, SEC-05 |
| Input Validator | Presentation | Pure functions validating names, menu choices, answers, question data | FR-01, FR-05, FR-10, FR-15, SEC-06 |
| Quiz Manager | Application | Builds a quiz, records answers, produces the result | FR-02–FR-08, FR-16 |
| Score Calculator | Application | Counts correct answers | FR-07, NFR-05 |
| Question Manager | Application | Owns the bank; add, list, modify, remove; enforces authorization and capacity | FR-10–FR-13, NFR-06, SEC-07 |
| Auth Manager | Application | Credential check, lockout, session grant/revoke; contains the internal `Sha256` helper | FR-09, FR-14, SEC-01–SEC-04 |
| Storage Manager | Data access | Reads/writes `questions.txt`; reads `admin.cfg`; skips malformed records | FR-17, FR-19 |
| Error Handler | Cross-cutting | Maps `Status` to user-facing text; reports warnings | FR-05, FR-16, NFR-03/04 |

The "Bounded Input Reader" shown in `architecture_security.puml` is a function group inside the UI Controller (`readLine`, `readPassword`), not a ninth component.

### 2.4 Data architecture

**`questions.txt`** — one record per line, six fields separated by the pipe character:
```
text|optionA|optionB|optionC|optionD|correctLetter
```
Rules: text 1–200 chars, options 1–100 chars, letter A–D, no pipe character inside fields, at most 100 records.

**`admin.cfg`** — one line:
```
username:salt:hash
```
`salt` is 16 random bytes in hex; `hash` is hex SHA-256 of `salt + password`. Created by `tools/make_admin <user> <password>`.

### 2.5 Source layout
```
quiz/
├── src/
│   ├── main.cpp
│   ├── types.h                 (Status, Question, Session, Quiz, results)
│   ├── ui_controller.{h,cpp}
│   ├── input_validator.{h,cpp}
│   ├── quiz_manager.{h,cpp}
│   ├── score_calculator.{h,cpp}
│   ├── question_manager.{h,cpp}
│   ├── auth_manager.{h,cpp}    (+ sha256.{h,cpp}, internal)
│   ├── storage_manager.{h,cpp}
│   └── error_handler.{h,cpp}
├── tools/make_admin.cpp
├── tests/                      (unit drivers, e.g. test_authz.cpp)
└── data/                       (questions.txt, admin.cfg)
```
Build: `g++ -std=c++17 -Wall -o quiz src/*.cpp`.

### 2.6 Traceability: requirements → components

| Requirement | Component(s) | Design ref |
|---|---|---|
| FR-01 | Input Validator, UI Controller | §3.3.2, seq. Take Quiz |
| FR-02 – FR-04 | Quiz Manager, Question Manager | §3.3.3, seq. Take Quiz |
| FR-05 | Input Validator, Error Handler | §3.3.2, §3.4 |
| FR-06 – FR-08 | Quiz Manager, Score Calculator | §3.3.3, §3.3.4 |
| FR-09 | Auth Manager | §3.3.6, seq. Admin Login |
| FR-10 | Question Manager, Input Validator | §3.3.5, seq. Add Question |
| FR-11 – FR-13 | Question Manager | §3.3.5 |
| FR-14 | Auth Manager, UI Controller | §3.3.6 |
| FR-15, FR-18 | UI Controller, Input Validator | §3.3.1 |
| FR-16 | Quiz Manager, Error Handler | §3.3.3, seq. Take Quiz |
| FR-17, FR-19 | Storage Manager | §3.3.7, §3.5.3 |
| NFR-01 | UI Controller | §3.3.1 |
| NFR-02, NFR-03 | Quiz Manager, Storage Manager (bank held in memory) | §2.4, §3.5.3 |
| NFR-04 | Input Validator, UI Controller, Error Handler | §3.3.1, §3.4 |
| NFR-05 | Score Calculator | §3.3.4 |
| NFR-06 | Question Manager | §3.3.5 |
| NFR-07, NFR-08 | All (source layout) | §2.5 |
| SEC-01 – SEC-04 | Auth Manager, Storage Manager, UI Controller | §2.7, §3.3.6 |
| SEC-05 | UI Controller, Input Validator | §2.7, §3.3.1 |
| SEC-06 | Question Manager, Input Validator | §3.3.5 |
| SEC-07 | Question Manager (`Session` parameter) | §3.3.5 |

All 38 SRS requirements (19 FR, 8 NFR, 7 SEC, 4 EI) are allocated. EI-01/02 → UI Controller; EI-03 → Storage Manager; EI-04 → no network code exists.

### 2.7 Security architecture
Diagram: `architecture_security.puml`. Four trust zones; data flows only forward through them.

| Zone | Contents | Control |
|---|---|---|
| 1 Input boundary | Bounded reader, Input Validator | Reads capped at 1000 chars; every input validated before use (SEC-05) |
| 2 Access control | Auth Manager, Session | Credential check, 3-strike/60 s lockout, uniform error (SEC-01–03) |
| 3 Trusted core | Quiz Manager, Question Manager | Admin calls require `Session::isAdmin()` (SEC-07); data validated again (SEC-06) |
| 4 Protected data | Storage Manager, files | Malformed records rejected on load; only salted hash stored (SEC-04) |

**Design decisions**
- `Session` can be set only by `Auth Manager` (`friend`). Any other code can only read it.
- Login evaluates username and hash together and compares in constant time, so timing does not reveal which part failed.
- If `admin.cfg` cannot be read, login fails closed.
- The password is read with echo disabled (termios on Linux, `SetConsoleMode` on Windows).
- Writes go to `questions.tmp`, then rename, so a crash never leaves a half-written bank.

**Threats and controls**

| Threat | Control | SEC |
|---|---|---|
| Guessing admin credentials | Lockout; uniform error message | SEC-02, SEC-03 |
| Using admin functions without login | `Session` required by every list/add/modify/remove | SEC-07 |
| Corrupting the bank through input (pipe character, over-long text) | Validator; length limits | SEC-05, SEC-06 |
| Buffer overflow | `std::string`, `std::getline`, capped reads; banned C functions | SEC-05 |
| Password disclosure | No echo; salted hash; no plaintext in source | SEC-04 |
| Corrupt or edited data file | Record-by-record validation on load; bad records skipped | FR-19 |

**Residual risks (accepted, out of scope of v1.1)**
| ID | Risk |
|---|---|
| R-1 | Lockout state is in memory, so restarting the program resets it |
| R-2 | `questions.txt` is plain text; anyone with file access can read answer keys or edit it. Mitigation: OS file permissions |
| R-3 | Single salted SHA-256 is fast to brute-force offline; a slow KDF would need a third-party library |
| R-4 | No audit log |

---

## 3. Detailed Design

### 3.1 Shared types (`types.h`)

```cpp
enum class Status {
    OK,
    ERR_INVALID_NAME,      // FR-01
    ERR_INVALID_OPTION,    // FR-05, FR-15
    ERR_INVALID_QUESTION,  // FR-10 (reason in message)
    ERR_EMPTY_BANK,        // FR-16
    ERR_BANK_FULL,         // NFR-06
    ERR_NOT_FOUND,         // question number out of range
    ERR_UNAUTHORIZED,      // SEC-07
    ERR_AUTH_FAILED,       // SEC-02
    ERR_LOCKED,            // SEC-03
    ERR_IO,                // file read/write failure
    ERR_MALFORMED_RECORD   // FR-19 (internal)
};

struct Question {
    std::string text;                    // 1..200
    std::array<std::string, 4> options;  // each 1..100
    char correct;                        // 'A'..'D'
};

class Session {                          // SEC-07
public:
    bool isAdmin() const { return admin_; }
private:
    friend class AuthManager;
    bool admin_ = false;
};

struct Quiz {
    std::string participant;
    std::vector<Question> questions;     // max 10
    std::vector<char> answers;
    size_t index = 0;
};

struct AnswerResult { bool correct; char keyLetter; };
struct QuizResult   { std::string participant; int score; int total; };
```

### 3.2 Sequence diagrams
| Use case | File | Shows |
|---|---|---|
| UC-01 Take Quiz | `sequence_take_quiz.puml` | Empty-bank check, name validation loop, answer loop with invalid input, feedback, scoring |
| UC-02 Admin Login | `sequence_admin_login.puml` | Lock check, credential read, hash compare, failure counter and lockout, fail-closed on config error |
| UC-03 Add Question | `sequence_add_question.puml` | Authorization, capacity, validation, save, rollback on write failure |

### 3.3 API design (internal C++ interfaces)
The application has no network API; the "API" is the set of component interfaces below. All operations return `Status` (or a value with a `Status` out-parameter). No exceptions cross a component boundary.

#### 3.3.1 UI Controller
```cpp
class UiController {
public:
    UiController(QuizManager&, QuestionManager&, AuthManager&);
    int run();                                   // main loop; returns process exit code
private:
    bool readLine(std::string& out, size_t maxLen = 1000);  // false on EOF; truncates to maxLen
    bool readPassword(std::string& out);         // no echo
    void mainMenu();                             // FR-15, EI-01
    void takeQuizFlow();                         // UC-01
    void adminLoginFlow();                       // UC-02
    void adminMenu(Session&);                    // EI-02, UC-03..UC-07
};
```
`run()` returns 0 on Exit (FR-18). On EOF the loop ends and returns 0.

#### 3.3.2 Input Validator
```cpp
namespace InputValidator {
    Status validateName(const std::string&);                       // 1..30, letters/digits/space
    Status validateMenuChoice(const std::string&, int lo, int hi, int& out);
    Status validateAnswer(const std::string&, char& letterOut);    // one of A-D, any case -> upper
    Status validateQuestion(const Question&, std::string& reason); // FR-10 rules
    bool   isConfirm(const std::string&);                          // exactly "y"
}
```
Pure functions, no I/O: directly unit-testable.

#### 3.3.3 Quiz Manager
```cpp
class QuizManager {
public:
    explicit QuizManager(const QuestionManager&);
    Status       checkAvailable() const;                  // ERR_EMPTY_BANK if bank is empty
    Status       startQuiz(const std::string& name, Quiz& out);   // ERR_INVALID_NAME, ERR_EMPTY_BANK
    bool         hasNext(const Quiz&) const;
    const Question& current(const Quiz&) const;
    AnswerResult submitAnswer(Quiz&, char letter);        // records, advances
    QuizResult   result(const Quiz&) const;               // uses ScoreCalculator
};
```
`startQuiz` copies `min(10, N)` questions in stored order via `QuestionManager::quizSet(10)`.

#### 3.3.4 Score Calculator
```cpp
namespace ScoreCalculator {
    int calculate(const std::vector<Question>&, const std::vector<char>& answers);
}
```
Deterministic, no state, no I/O (NFR-05).

#### 3.3.5 Question Manager
```cpp
class QuestionManager {
public:
    explicit QuestionManager(StorageManager&);
    Status load(int& skipped);                                        // FR-19
    size_t count() const;
    std::vector<Question> quizSet(size_t max) const;                  // no session; used by Quiz Manager
    // Admin operations: every one requires an admin Session (SEC-07)
    Status list  (const Session&, std::vector<Question>& out) const;  // FR-11
    Status add   (const Session&, const Question&);                   // FR-10, NFR-06
    Status modify(const Session&, size_t number1, const Question&);   // FR-12 (1-based)
    Status remove(const Session&, size_t number1);                    // FR-13 (confirmation in UI)
};
```
Every mutator follows the same order: (1) check `session.isAdmin()` → `ERR_UNAUTHORIZED`; (2) check capacity/index; (3) validate (`ERR_INVALID_QUESTION`); (4) change the in-memory bank; (5) `saveQuestions`; (6) on `ERR_IO`, roll back step 4 and return `ERR_IO`.

#### 3.3.6 Auth Manager
```cpp
class AuthManager {
public:
    explicit AuthManager(StorageManager&);
    Status login(const std::string& user, const std::string& pass, Session&);
                                          // OK, ERR_AUTH_FAILED, ERR_LOCKED, ERR_IO
    void   logout(Session&);              // FR-14
    int    lockRemainingSeconds() const;  // 0 if not locked
private:
    int failures_ = 0;
    std::chrono::steady_clock::time_point lockedUntil_{};
    static bool constantTimeEquals(const std::string&, const std::string&);
};
```
Constants: `MAX_FAILURES = 3`, `LOCK_SECONDS = 60`.

#### 3.3.7 Storage Manager
```cpp
struct Credentials { std::string user, salt, hashHex; };

class StorageManager {
public:
    StorageManager(std::string questionsPath, std::string adminPath);
    Status loadQuestions(std::vector<Question>&, int& skipped);  // missing file -> OK + empty
    Status saveQuestions(const std::vector<Question>&);          // temp file + rename
    Status readCredentials(Credentials&);                        // ERR_IO if absent/invalid
};
```

#### 3.3.8 Error Handler
```cpp
namespace ErrorHandler {
    std::string message(Status, int arg = 0);   // user text; arg = seconds or count
    void warn(const std::string&);              // non-fatal notice (e.g. skipped records)
}
```

### 3.4 Error handling

**Strategy**
1. Validate at the boundary (UI Controller + Input Validator); invalid input never reaches business logic.
2. Components return `Status`; the UI Controller asks `ErrorHandler::message()` for the text and re-prompts or returns to the menu.
3. Exceptions (`std::bad_alloc`, `std::filesystem::filesystem_error`) are caught inside Storage Manager and converted to `ERR_IO`. `main()` has a last-resort `try/catch` that prints "Unexpected error" and exits with code 1, so users never see a raw crash.
4. Security failures fail closed: unreadable `admin.cfg` means no login.
5. Data-changing operations roll back on failure, so memory and file stay consistent.

**Scenarios**

| Situation | Status | User message | Recovery | Req |
|---|---|---|---|---|
| Invalid or empty name | `ERR_INVALID_NAME` | "Invalid name. Use 1-30 letters, digits or spaces." | Re-prompt | FR-01 |
| Answer not A–D | `ERR_INVALID_OPTION` | "Invalid option" | Re-prompt same question | FR-05 |
| Menu input not listed | `ERR_INVALID_OPTION` | "Invalid option. Enter a number from 1 to N." | Re-prompt | FR-15 |
| Bank empty at quiz start | `ERR_EMPTY_BANK` | "No questions available" | Return to main menu | FR-16 |
| Bad credentials | `ERR_AUTH_FAILED` | "Invalid credentials" | Return to main menu | SEC-02 |
| Locked out | `ERR_LOCKED` | "Too many failed attempts. Try again in N s." | Return to main menu | SEC-03 |
| Admin config unreadable | `ERR_IO` | "Admin configuration unavailable." | Login denied | SEC-01 |
| Admin op without session | `ERR_UNAUTHORIZED` | "Access denied. Please log in." | Return to main menu | SEC-07 |
| Invalid question data | `ERR_INVALID_QUESTION` | Reason, e.g. "Option B must be 1-100 characters." | Re-prompt or return to admin menu | FR-10 |
| 101st question | `ERR_BANK_FULL` | "Question bank is full (100)." | Return to admin menu | NFR-06 |
| Unknown question number | `ERR_NOT_FOUND` | "No such question number." | Re-prompt | FR-12/13 |
| Delete not confirmed | — | "Deletion cancelled." | Return to admin menu | FR-13 |
| Save fails | `ERR_IO` | "Could not save data file. Question not added." (or "…changed"/"…deleted") | Rollback | FR-17 |
| Malformed records on load | `ERR_MALFORMED_RECORD` (internal) | "N record(s) skipped" | Load the valid ones | FR-19 |
| Line longer than 1000 chars | — | Treated as invalid input for that prompt | Re-prompt | NFR-04 |
| EOF on input | — | none | Exit with code 0 | NFR-04 |
| Uncaught exception | — | "Unexpected error" | Exit with code 1 | NFR-04 |

### 3.5 Key algorithms

#### 3.5.1 Login with lockout
```
login(user, pass, session):
    if now < lockedUntil_:            return ERR_LOCKED
    if !storage.readCredentials(c):   return ERR_IO
    h  = hex(sha256(c.salt + pass))
    ok = constantTimeEquals(user, c.user) & constantTimeEquals(h, c.hashHex)
    if ok:
        failures_ = 0; session.admin_ = true; return OK
    failures_++
    if failures_ >= 3:
        lockedUntil_ = now + 60 s; failures_ = 0
    return ERR_AUTH_FAILED
```
After a failure the UI calls `lockRemainingSeconds()` and adds a lock notice if it is above 0.

#### 3.5.2 Quiz loop
```
checkAvailable() -> ERR_EMPTY_BANK if count()==0
name  <- prompt until validateName == OK
quiz  <- startQuiz(name)              // min(10, N) questions
while hasNext(quiz):
    show current(quiz)
    letter <- prompt until validateAnswer == OK
    r <- submitAnswer(quiz, letter)   // show Correct! / Wrong! Correct answer: X
show result(quiz)                     // score/total
```

#### 3.5.3 Load and save
```
loadQuestions:
    if file missing: bank = empty; return OK
    for each line:
        f = split(line, '|')
        if f.size != 6 or validateQuestion fails or bank.size == 100:
            skipped++; continue
        bank.push_back(...)
saveQuestions:
    write all records to questions.tmp; flush; check stream state
    on failure: return ERR_IO
    rename(questions.tmp, questions.txt)      // replaces target
```
The bank is loaded once at startup and kept in memory, so quiz operations do not touch the disk (NFR-02).

### 3.6 Design constraints and decisions
| # | Decision | Reason |
|---|---|---|
| D-1 | `checkAvailable()` runs before the name prompt | Avoid asking for a name when no quiz can start (FR-16) |
| D-2 | `list` requires `Session` but `quizSet` does not | SEC-07 covers admin viewing; participants must still read questions |
| D-3 | Lockout kept in memory | Simplicity; residual risk R-1 |
| D-4 | `tools/make_admin` provisions credentials | SRS assumes `admin.cfg` exists before first login |
| D-5 | Bank size cap of 100 held in Question Manager | NFR-06, bounded memory and load time |
| D-6 | EOF on stdin ends the program cleanly | Prevents an infinite prompt loop (NFR-04) |

---

## 4. Requirement → Test Traceability (summary)

| Component | Test cases |
|---|---|
| UI Controller, Input Validator | TC-01, 11, 13, 15, 22 |
| Quiz Manager, Score Calculator | TC-02, 03, 04, 14, 16 |
| Question Manager | TC-06, 07, 08, 09, 17, 23, 24 |
| Auth Manager | TC-05, 10, 19, 20, 21 |
| Storage Manager | TC-12 |
| All / build | TC-18 |

The full requirement → test-case matrix is in `Test_Plan.md` §13.
