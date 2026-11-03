# Requirements validation report (Deliverable 1 v1.0 baseline)

Date: 2026-11-01. Environment: Linux, g++ 13.3.0, libsodium 1.0.18, C++17 `-Wall -Wextra -Werror -pedantic -O2`.
Commands run: `bash scripts/build.sh`, `./build/quiz_tests` (24 tests, 0 failures), `python3 scripts/smoke_test.py` (pass),
`./build/quiz_benchmark` (menu p95 103.8 ms, question p95 0.005 ms; limit 2000 ms), plus manual runs of `build/quiz`.

Verdicts: **Met**, **Partial**, **Not implemented**. "Met" means behaviour was observed, not just that code exists.

## Part 1: Accounts and access
| Req | Verdict | Evidence |
|---|---|---|
| FR-01 | Met | 7-char password rejected; valid learner saved; duplicate rejected (manual run + `registration_validates_boundaries_and_duplicate_names`). |
| FR-02 | Met | Wrong password and unknown user give the same "Invalid credentials."; correct pairs open learner/admin menus. |
| SEC-01 | Met | `users.db` stores `$argon2id$v=19$m=65536,t=2,p=1$<unique salt>$...`; plaintext occurs 0 times; libsodium used. |
| SEC-02 | Partial | `ResultService::summary` denies non-admin sessions (tested). No other admin operations exist to check. |

## Part 2: Authoring, quiz configuration, delivery
| Req | Verdict | Evidence |
|---|---|---|
| FR-03 Question maintenance | Not implemented | No code writes `questions.db`; `QuestionCatalog` is a read-only adapter. |
| FR-04 Quiz configuration | Not implemented | No quiz entity, storage or editor in `src/` or `include/`. |
| FR-05 Quiz discovery | Not implemented | Console prints "No quizzes available." because `main.cpp` passes an empty `takeQuiz` callback. |
| FR-06 Attempt flow | Partial | `AnswerSession` shows each question once with A-D and records valid answers (tested), but nothing supplies a quiz to it. |
| FR-07 Input recovery | Met | Invalid input repeats the same question and records nothing (3 tests). Answers must be uppercase; document this choice. |

## Part 3: Scoring and results
| Req | Verdict | Evidence |
|---|---|---|
| FR-08 Scoring | Not implemented | No code compares answers with the key. Only display of count and one-decimal percentage exists (admin summary). |
| FR-09 Own results | Not implemented | No own-history service. |
| FR-10 Admin summary | Met | Admin sees completed attempts; learner, guest, logged-out admin denied (tests). Reads `attempts.db`, which nothing in the app writes yet. |
| SEC-03 Result ownership | Not implemented | Depends on FR-09. |

## Part 4: Persistence, quality, file validation
| Req | Verdict | Evidence |
|---|---|---|
| FR-11 | Partial | Accounts only. Quizzes, questions and attempts are never written. |
| NFR-01 | Met | Benchmark: 1,000 questions, 10,000 attempts, 40 samples, p95 well under 2 s. |
| NFR-02 | Partial | Temp file + atomic replace and failure test exist for accounts only. |
| NFR-03 | Met | Prompts state accepted values; invalid input gives a correction prompt, no crash. |
| NFR-04 | Met on Linux | Warning-clean build. Windows result relies on the GitHub Actions run (not verified here). |
| SEC-04 | Met | Accounts, attempts and questions reject malformed records on load (tests). |

## Summary
Met: FR-01, FR-02, FR-07, FR-10, NFR-01, NFR-03, SEC-01, SEC-04 (NFR-04 on Linux).
Partial: FR-06, FR-11, NFR-02, SEC-02.
Not implemented: FR-03, FR-04, FR-05, FR-08, FR-09, SEC-03.
The take-quiz-and-get-a-score path (TC-05, TC-07, TC-08) cannot run until FR-03/04/05/06/08 and an attempt writer exist.

## Documentation gaps
- Root `Online_Quiz_System_SRS.md`, `Test_Plan.md` and `Architecture_Design_Specification.md` describe an older scope with different IDs.
- Deliverable 1 exists only as a .docx; the Part 3 test-case update should be made against it.
