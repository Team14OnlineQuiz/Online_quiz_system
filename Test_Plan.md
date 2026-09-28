# Test Plan — Online Quiz System

| | |
|---|---|
| Plan ID | OQS-TP-1.0 |
| Date | 28-09-2026 |
| Based on | `Online_Quiz_System_SRS.md` v1.1 |
| Format | IEEE 829 (section order adapted; see note in §5) |
| Team | _Name / USN ×4 — fill in_ |

---

## 1. Introduction

### 1.1 Purpose
Defines how the Online Quiz System is verified against SRS v1.1: what is tested, how, with what data, and the pass/fail criteria. Section 12 holds the 24 test cases.

### 1.2 Scope
Black-box console testing of all functional, non-functional, and security requirements, plus white-box inspection where the requirement is structural (NFR-07, NFR-08, SEC-04, SEC-05).

### 1.3 Definitions
| Term | Meaning |
|---|---|
| TC | Test case |
| TD | Test data set (Appendix A) |
| Sev | Defect severity (§6) |
| ASan/UBSan | GCC AddressSanitizer / UndefinedBehaviorSanitizer |

### 1.4 References
1. `Online_Quiz_System_SRS.md` v1.1
2. `Architecture_Design_Specification.md` v1.0
3. IEEE Std 829-2008, *Software and System Test Documentation*

---

## 2. Test Items

| Item | Version | Source |
|---|---|---|
| `quiz` executable (7 components + UI Controller) | 1.0 | `src/*.cpp`, built per NFR-07 |
| `tools/make_admin` (creates `admin.cfg`) | 1.0 | `tools/make_admin.cpp` |
| Data files `questions.txt`, `admin.cfg` | — | Appendix A |

---

## 3. Features to Be Tested

| Group | Requirements | Test cases |
|---|---|---|
| Participant quiz | FR-01 – FR-08, FR-16 | TC-01 – TC-04 |
| Administrator | FR-09 – FR-14 | TC-05 – TC-10 |
| Menus, exit, persistence | FR-15, FR-17, FR-18, FR-19, EI-01, EI-02, EI-03 | TC-11, TC-12 |
| Usability | NFR-01 | TC-13 |
| Performance, capacity | NFR-02, NFR-03, NFR-06 | TC-14, TC-17 |
| Reliability, determinism | NFR-04, NFR-05 | TC-15, TC-16 |
| Portability, maintainability | NFR-07, NFR-08, EI-04 | TC-18 |
| Security | SEC-01 – SEC-07 | TC-19 – TC-24 |

## 4. Features Not to Be Tested

| Item | Reason |
|---|---|
| GUI, network, timers, categories, history, leaderboards | Out of scope (SRS §1.2) |
| Persistence of lockout state across restarts | Not required; in-memory by design (Architecture §2.7, residual risk R-1) |
| Correctness of the OS console, file system, compiler | Third-party |
| Cryptographic strength of SHA-256 | Standard algorithm; only its correct use is checked (TC-21) |

---

## 5. Test Approach

> Note: the brief asks for Sections 3, 4, 5 and a Section 5.1 for security validation. Sections 3 and 4 above are features to be tested / not tested; Section 5 is the approach, with 5.1 as security validation.

### 5.1 Security Validation

**Objective:** demonstrate SO-01 (admin protection), SO-02 (question integrity), SO-03 (credential confidentiality).

| Technique | Applied to | Test cases |
|---|---|---|
| Authentication tests: valid, wrong user, wrong password, uniform error message | SEC-01, SEC-02 | TC-19 |
| Brute-force / lockout test: 3 failures then timed block | SEC-03 | TC-20 |
| Credential storage inspection: no plaintext in `admin.cfg` or source; salt uniqueness; no echo | SEC-04 | TC-21 |
| Static scan for banned functions (`gets`, `scanf("%s")`, `strcpy`, `sprintf`) plus oversized-input run under ASan/UBSan | SEC-05 | TC-22, TC-15 |
| Malicious-data tests: invalid fields and the delimiter (pipe) character; file must stay byte-identical on rejection | SEC-06 | TC-23 |
| Authorization tests: unauthenticated access via menu and via direct API driver | SEC-07 | TC-24 |

**Build for security testing:** `g++ -std=c++17 -Wall -Wextra -g -fsanitize=address,undefined -fstack-protector-strong -o quiz_asan src/*.cpp`.

**Objective coverage:** SO-01 → TC-19, 20, 22, 24. SO-02 → TC-22, 23, 24. SO-03 → TC-19, 21.

### 5.2 Functional testing
Black-box, requirement-based. Each FR is exercised with one valid case, and with boundary and invalid inputs. Console input is scripted from files (`./quiz < input.txt > out.txt`) or entered manually for interactive prompts (password echo).

### 5.3 Non-functional testing
- **Performance (NFR-02/03):** scripted run on TD-4, timed with `time` and per-prompt timestamps; 5 runs, worst case counts.
- **Reliability (NFR-04):** oversized and EOF input, ASan build.
- **Determinism (NFR-05):** 5 identical runs, transcripts compared with `diff`.
- **Portability/maintainability (NFR-07/08):** clean build on both platforms; file-structure inspection.
- **Usability (NFR-01):** prompt-by-prompt inspection.

### 5.4 Levels and regression
| Level | Scope |
|---|---|
| Unit | Input Validator, Score Calculator, Auth Manager, Question Manager driven by small `tests/*.cpp` drivers |
| System | Console tests in §12 |
| Regression | Full §12 suite re-run after any defect fix |

### 5.5 Test-case identification
IDs `TC-01` – `TC-24` match the SRS §6 traceability table. Priority: **H** must pass for release, **M** should pass.

---

## 6. Pass/Fail Criteria

**Test case:** passes when every step's actual result matches the expected result exactly (messages that the SRS quotes must match verbatim).

**Release:** all H test cases pass, at least 90% of M test cases pass, and no open Sev-1 or Sev-2 defects.

| Sev | Definition |
|---|---|
| 1 Critical | Crash, data loss, security bypass |
| 2 Major | Requirement not met, no workaround |
| 3 Minor | Requirement met with cosmetic or wording issue |

## 7. Suspension and Resumption
- **Suspend:** build fails; the program crashes at startup; a Sev-1 defect blocks more than 3 test cases.
- **Resume:** after a fixed build passes a smoke test (TC-05 and TC-11).

## 8. Test Environment

| Item | Setting |
|---|---|
| OS | Ubuntu 22.04 (primary), Windows 10/11 (MinGW g++) |
| Compiler | g++ ≥ 11, `-std=c++17` |
| Tools | bash, `time`, `diff`, `sha256sum`, `grep`, cppcheck, ASan/UBSan |
| Data reset | Before each TC, copy the named TD into the working directory |
| Test admin | username `admin`, password `Quiz@2026` (test only; created with `make_admin`) |

## 9. Test Deliverables
This plan, test data (Appendix A), input scripts, execution log (TC, date, tester, result, defect ID), defect list, and a summary report with the traceability matrix in §13.

## 10. Responsibilities and Schedule

| Role | Owner | Duties |
|---|---|---|
| Test lead | Member 1 | Plan, coverage, report |
| Test designers | Members 2, 3 | Cases, data, scripts |
| Test executor / defect tracker | Member 4 | Run, log, retest |

| Phase | Timing |
|---|---|
| Plan and test-case design | Part-1 (by 28-09-2026) |
| Scripts and data | Start of Part-2 |
| Execution and defect fixing | Part-2 |
| Regression and report | End of Part-2 |

## 11. Risks

| ID | Risk | Mitigation |
|---|---|---|
| R-1 | Lockout resets when the program restarts | Accepted for v1.0; documented; not tested |
| R-2 | 60 s lockout test is slow | Automate with a sleep; run once per cycle |
| R-3 | Password echo can only be checked manually | Manual step in TC-21, recorded with a screenshot |
| R-4 | Timing results vary by machine | Record the machine spec; use worst of 5 runs |

---

## 12. Test Cases

Conventions: input lines are shown in `code`; "Main" = main menu; "Admin" = admin menu; `<empty>` = press Enter only. Admin cases assume a successful login with `admin` / `Quiz@2026` unless stated.

### 12.1 Participant

#### TC-01 — Name and answer validation

**Requirements:** FR-01, FR-05 · **Priority:** H · **Data:** TD-1

**Steps**

1. Main: `1`.
2. Name: `<empty>`, then 31 × `a`, then `Asha!`.
3. Name: `Asha 1`.
4. At Q1 enter: `E`, `<empty>`, `AB`, `1`, then `b`.

**Expected:** Step 2: each entry shows a name error stating 1–30 letters/digits/spaces, and re-prompts; quiz does not start. Step 3: accepted, Q1 shown. Step 4: first four entries each show "Invalid option" and redisplay Q1 (counter stays at 1); `b` is accepted and Q2 is shown.

#### TC-02 — Quiz size, order, and display

**Requirements:** FR-02, FR-03, FR-04 · **Priority:** H · **Data:** TD-1, then TD-2

**Steps**

1. TD-1 (12 questions): start quiz as `Asha`; answer `a` (lowercase) at Q1, then any letters.
2. Repeat with TD-2 (5 questions).

**Expected:** 1: exactly 10 questions are shown as "Question k of 10", in file order (Q1…Q10); each has options A–D; lowercase `a` accepted; Q11 and Q12 never appear. 2: 5 questions, "Question k of 5".

#### TC-03 — Feedback, scoring, and result

**Requirements:** FR-06, FR-07, FR-08 · **Priority:** H · **Data:** TD-1

**Steps**

1. Start as `Asha`; answer `B A C D A B C D C D`.
2. New run; answer `A B A A B A A A B A`.

**Expected:** 1: "Correct!" for Q1–Q8; "Wrong! Correct answer: A" for Q9; "Wrong! Correct answer: B" for Q10; result shows `Participant : Asha` and `Score : 8/10`. 2: every answer is "Wrong! …" with the right key; result `0/10`.

#### TC-04 — Empty question bank

**Requirements:** FR-16 · **Priority:** H · **Data:** TD-3, and no file

**Steps**

1. With an empty `questions.txt`, Main: `1`.
2. Delete `questions.txt`, restart, Main: `1`.

**Expected:** Both: "No questions available" is displayed, no quiz starts, and the main menu is shown again.

### 12.2 Administrator

#### TC-05 — Admin login success

**Requirements:** FR-09 · **Priority:** H · **Data:** TD-2, TD-6

**Steps**

- Main: `2`; username `admin`; password `Quiz@2026`.

**Expected:** Admin menu (1 Add, 2 View, 3 Modify, 4 Delete, 5 Logout) is shown.

#### TC-06 — Add question: valid and invalid

**Requirements:** FR-10 · **Priority:** H · **Data:** TD-2

**Steps**

- Admin `1`, then:
- a. text `What is 2+2?`, options `3`,`4`,`5`,`6`, correct `B`.
- b. text `<empty>`.
- c. text of 201 characters.
- d. valid text, option B `<empty>`.
- e. option D of 101 characters.
- f. correct answer `E`.
- g. text `x|A|B|C|D|A` (contains the pipe character).
- Then Admin `2` (view).

**Expected:** a: "Question added."; view shows 6 questions with the new one as #6. b–g: each rejected with a reason and re-prompt; nothing stored; view still shows 6.

#### TC-07 — View questions

**Requirements:** FR-11 · **Priority:** H · **Data:** TD-2, TD-3

**Steps**

1. TD-2: Admin `2`.
2. TD-3: Admin `2`.

**Expected:** 1: five questions numbered 1–5, each showing text, options A–D, and correct answer. 2: "No questions available".

#### TC-08 — Modify question

**Requirements:** FR-12 · **Priority:** H · **Data:** TD-2

**Steps**

- Admin `3`, then:
- a. number `2`, new text `Updated Q?`, same options, correct `C`.
- b. numbers `0`, `6`, `x`.
- c. number `2`, text `<empty>`.
- Then Admin `2`.

**Expected:** a: success message. b: "No such question number." and re-prompt for each. c: rejected per FR-10. View: #2 is `Updated Q?` with key C; all other questions unchanged.

#### TC-09 — Delete with confirmation

**Requirements:** FR-13 · **Priority:** H · **Data:** TD-2

**Steps**

- Admin `4`, then:
- a. number `3`, confirm `y`.
- b. number `1`, confirm `n`.
- c. number `1`, confirm `x`.
- d. number `9`.
- Then Admin `2`.

**Expected:** a: success; b, c: "cancelled", nothing deleted; d: "No such question number." View: 4 questions remain, the old #3 is gone, and numbering is 1–4.

#### TC-10 — Logout

**Requirements:** FR-14 · **Priority:** H · **Data:** TD-2

**Steps**

1. Admin `5`.
2. Main: `2`.

**Expected:** 1: main menu is shown. 2: credentials are requested again (session ended).

### 12.3 Menus, exit, persistence

#### TC-11 — Menu validation and exit

**Requirements:** FR-15, FR-18, EI-01, EI-02 · **Priority:** H · **Data:** TD-2

**Steps**

1. Main: `0`, `4`, `abc`, `<empty>`, `1.5`.
2. Log in; Admin: `0`, `6`, `x`.
3. Log out; Main: `3`; then run `echo $?`.

**Expected:** 1 and 2: each entry shows "Invalid option…" and re-prompts; main menu lists items 1–3 exactly as EI-01, admin menu lists 1–5 as EI-02. 3: program ends; exit code `0`.

#### TC-12 — Persistence and corrupt-file handling

**Requirements:** FR-17, FR-19, EI-03 · **Priority:** H · **Data:** TD-2, TD-5

**Steps**

1. Log in, add a valid question, delete #1, modify #2; `sha256sum questions.txt`; exit.
2. Restart; Admin `2`.
3. Replace file with TD-5; start program.
4. Take a quiz.
5. (Design check) Make `questions.txt` read-only; log in; add a valid question.

**Expected:** 1: `questions.txt` already reflects each change right after the operation. 2: view shows the same data as before exit. 3: warning "2 record(s) skipped". 4: quiz has 4 questions. 5: "Could not save data file. Question not added."; view unchanged.

### 12.4 Non-functional

#### TC-13 — Prompt clarity

**Requirements:** NFR-01 · **Priority:** M · **Data:** TD-2

**Steps**

- Visit every prompt: main menu, name, answer, admin username/password, admin menu, add fields, number, confirmation.

**Expected:** Every prompt states its accepted values (e.g. `Enter answer (A-D):`, `Enter choice (1-3):`, `Confirm (y/n):`).

#### TC-14 — Response times

**Requirements:** NFR-02, NFR-03 · **Priority:** M · **Data:** TD-4

**Steps**

1. Start with input `3` only; measure with `time ./quiz`.
2. Run a scripted 10-question quiz; timestamp each prompt.
3. Repeat 5×.

**Expected:** Step 1: ≤ 2 s. Step 2: each next question appears ≤ 1 s after the answer. Worst of 5 runs is used.

#### TC-15 — Oversized and closed input

**Requirements:** NFR-04 · **Priority:** H · **Data:** TD-2

**Steps**

- Using the ASan build, feed lines of 1000, 5000, and 1,000,000 characters at: name, main menu, answer, admin username, password, question text. Then close stdin (Ctrl-D) at a prompt.

**Expected:** No crash, no sanitizer report; over-long input is truncated to 1000 characters and treated as invalid; the program keeps running. On closed stdin, the program exits cleanly without looping.

#### TC-16 — Deterministic scoring

**Requirements:** NFR-05 · **Priority:** M · **Data:** TD-1

**Steps**

- Run the TC-03 step 1 input 5 times; `diff` transcripts.

**Expected:** All 5 runs give `8/10`; transcripts are identical.

#### TC-17 — Bank capacity

**Requirements:** NFR-06 · **Priority:** M · **Data:** TD-4

**Steps**

- Log in; Admin `1`; enter a valid question (bank already holds 100). Then Admin `2`.

**Expected:** "Question bank is full (100)."; view still shows 100 questions.

#### TC-18 — Build and structure

**Requirements:** NFR-07, NFR-08, EI-04 · **Priority:** M · **Data:** —

**Steps**

1. On Ubuntu and on Windows/MinGW: `g++ -std=c++17 -Wall -o quiz src/*.cpp`.
2. List `src/`.
3. `grep -rnE "socket|<winsock|<curl" src/`.

**Expected:** 1: 0 errors, 0 warnings on both. 2: each of Quiz Manager, Question Manager, Auth Manager, Input Validator, Score Calculator, Storage Manager, Error Handler has its own `.h`/`.cpp` pair. 3: no matches.

### 12.5 Security (see §5.1)

#### TC-19 — Authentication and uniform error

**Requirements:** SEC-01, SEC-02 · **Priority:** H · **Data:** TD-6

**Steps**

1. Main `2`: `admin` / `wrong`.
2. Main `2`: `root` / `Quiz@2026`.
3. Compare the two outputs byte for byte.
4. Main `2`: `admin` / `Quiz@2026`; log out.
5. Main `2`: `root` / `wrong`.

**Expected:** Steps 1, 2, 5: "Invalid credentials", no admin menu, return to main. Outputs of 1, 2, 5 are identical. Step 4: admin menu shown.

#### TC-20 — Lockout after 3 failures

**Requirements:** SEC-03 · **Priority:** H · **Data:** TD-6

**Steps**

1. Three consecutive wrong logins.
2. Immediately try the correct password.
3. Wait 61 s; try the correct password.
4. Separate run: 2 wrong logins, 1 correct, then 2 wrong logins.

**Expected:** 2: blocked with "Too many failed attempts. Try again in N s." (N ≤ 60), even with the correct password. 3: login succeeds. 4: no lockout (counter reset by the successful login).

#### TC-21 — Credential storage and echo

**Requirements:** SEC-04 · **Priority:** H · **Data:** TD-6

**Steps**

1. Open `admin.cfg`.
2. `grep -c "Quiz@2026" admin.cfg`; `grep -rnE "Quiz@2026|password *=" src/`.
3. Run `make_admin` twice with the same password; compare files.
4. Type the password at the login prompt (manual).

**Expected:** 1: username, salt, and a 64-hex-character hash only. 2: 0 matches in both. 3: salts and hashes differ. 4: no characters are echoed.

#### TC-22 — Bounded input

**Requirements:** SEC-05 · **Priority:** H · **Data:** —

**Steps**

1. `grep -rnE "\bgets *\(|scanf *\(\"%s\"|\bstrcpy *\(|\bsprintf *\(" src/`.
2. Run `cppcheck --enable=warning,style src/`.
3. Repeat TC-15 on the sanitizer build.

**Expected:** 1: no matches. 2: no buffer-related findings. 3: no ASan/UBSan reports.

#### TC-23 — Rejected data leaves bank intact

**Requirements:** SEC-06 · **Priority:** H · **Data:** TD-2

**Steps**

1. `sha256sum questions.txt` → H1.
2. Log in; attempt every invalid add from TC-06 b–g, and modify #1 to each invalid value.
3. `sha256sum questions.txt` → H2.

**Expected:** Every attempt rejected; H1 equals H2.

#### TC-24 — Authorization on all admin operations

**Requirements:** SEC-07 · **Priority:** H · **Data:** TD-2

**Steps**

1. Not logged in: at Main, enter `1`–`5` and observe.
2. After logout, repeat.
3. Run `tests/test_authz.cpp`: with a default (unauthenticated) `Session`, call list, add, modify, remove.
4. Inspect `question_manager.cpp`.

**Expected:** 1, 2: Main has no path to admin operations (1 = Take Quiz, 2 = login, 3 = exit, others invalid). 3: every call returns `ERR_UNAUTHORIZED`; bank and file unchanged. 4: every list/add/modify/remove function takes `const Session&` and checks `isAdmin()` before doing anything else.

---

## 13. Traceability

### 13.1 Requirement → test case

| Requirement | TC | Requirement | TC |
|---|---|---|---|
| FR-01 | TC-01 | NFR-01 | TC-13 |
| FR-02 | TC-02 | NFR-02 | TC-14 |
| FR-03 | TC-02 | NFR-03 | TC-14 |
| FR-04 | TC-02 | NFR-04 | TC-15 |
| FR-05 | TC-01 | NFR-05 | TC-16 |
| FR-06 | TC-03 | NFR-06 | TC-17 |
| FR-07 | TC-03 | NFR-07 | TC-18 |
| FR-08 | TC-03 | NFR-08 | TC-18 |
| FR-09 | TC-05 | SEC-01 | TC-19 |
| FR-10 | TC-06 | SEC-02 | TC-19 |
| FR-11 | TC-07 | SEC-03 | TC-20 |
| FR-12 | TC-08 | SEC-04 | TC-21 |
| FR-13 | TC-09 | SEC-05 | TC-22 |
| FR-14 | TC-10 | SEC-06 | TC-23 |
| FR-15 | TC-11 | SEC-07 | TC-24 |
| FR-16 | TC-04 | EI-01, EI-02 | TC-11 |
| FR-17 | TC-12 | EI-03 | TC-12 |
| FR-18 | TC-11 | EI-04 | TC-18 |
| FR-19 | TC-12 | | |

**Coverage:** 19 FR + 8 NFR + 7 SEC + 4 EI = 38 requirements; all traced to at least one TC; 24 TCs (H: 19, M: 5). This matches SRS §6.

### 13.2 Test case → design component

| TC | Component(s) |
|---|---|
| TC-01, 11, 15, 22 | Input Validator, UI Controller |
| TC-02, 03, 04, 14, 16 | Quiz Manager, Score Calculator |
| TC-05, 10, 19, 20, 21 | Auth Manager |
| TC-06, 07, 08, 09, 17, 23, 24 | Question Manager |
| TC-12 | Storage Manager |
| TC-13 | UI Controller |
| TC-18 | All |

---

## Appendix A — Test Data

| ID | Description |
|---|---|
| TD-1 | 12-question bank (below); answer key `B A C D A B C D A B C D` |
| TD-2 | First 5 lines of TD-1 |
| TD-3 | Empty `questions.txt` |
| TD-4 | 100-question bank: `for i in $(seq 1 100); do echo "Question $i?\|A1\|B1\|C1\|D1\|A"; done > questions.txt` |
| TD-5 | Lines 1–4 of TD-1, plus `Broken line\|A\|B\|C` and `Bad key?\|1\|2\|3\|4\|E` (2 malformed) |
| TD-6 | `admin.cfg` created by `make_admin admin Quiz@2026` |

**TD-1 (`questions.txt`, one record per line: text, options A–D, correct letter, separated by the pipe character)**

```
Which data structure follows FIFO?|Stack|Queue|Tree|Graph|B
Which language is used for this project?|C++|Java|Python|Go|A
What does CPU stand for?|Central Program Unit|Control Process Unit|Central Processing Unit|Core Processing Utility|C
Which of these is NOT a loop in C++?|for|while|do-while|repeat|D
Time complexity of binary search?|O(log n)|O(n)|O(n log n)|O(1)|A
Which symbol ends a C++ statement?|:|;|,|.|B
Which is a valid C++ header for I/O?|<stdio>|<conio>|<iostream>|<input>|C
Which is a UML behavioural diagram?|Class diagram|Component diagram|Package diagram|Sequence diagram|D
Which is a stable sorting algorithm?|Merge sort|Quick sort|Heap sort|Selection sort|A
Which SDLC model is risk-driven?|Waterfall|Spiral|Big Bang|Build-and-fix|B
What does SRS stand for?|System Run Script|Software Release Sheet|Software Requirements Specification|Structured Result Summary|C
Which testing checks internal code paths?|Black-box|Acceptance|Smoke|White-box|D
```
