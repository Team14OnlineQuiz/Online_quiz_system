# Local storage integration contract

These versioned UTF-8 files belong to repository adapters, not to console users. All user-facing writes must be validated and committed before success is reported. Filenames are fixed inside the selected `--data` directory; usernames never become paths.

## Accounts

`users.db` begins with `QUIZ_USERS_V1`. Each subsequent line has:

```text
userId role "username" "encodedArgon2idHash"
```

Role `0` means learner, `1` means administrator. Strings use the C++ `std::quoted` encoding, with backslash escapes for quotes and backslashes. IDs and usernames are unique. Use `AccountStore::insert` rather than constructing credential records yourself. `AuthService::registerLearner` always selects Learner; `provisionFirstAdmin` only works on an empty account store.

The account adapter writes a replacement file before swapping committed in-memory state. OS-dependent replacement is isolated in `replaceFile` in `account_store.cpp`.

## Attempt snapshots

The administrator summary consumes `attempts.db`, beginning with `QUIZ_ATTEMPTS_V1`:

```text
attemptId userId completed correctCount totalQuestions "quizTitleSnapshot" "submittedAtUTC"
```

`completed` is `0` or `1`. Counts satisfy `0 <= correctCount <= totalQuestions` and `totalQuestions > 0`. Dates use valid calendar timestamps such as `2026-10-20T10:00:00Z`. Only completed rows are shown in the administrator summary. Unknown account IDs fail the summary instead of becoming anonymous valid scores.

The title and evaluation counts are snapshots: later edits to quiz definitions do not alter past scores. The attempt writer is another member's persistence task; this branch adds the validated read adapter and `AttemptStore` interface needed for #10. A shared in-memory writer can implement the interface without changing the summary service.

`FileAttemptStore::reload` stages all parsed rows before replacing its previous in-memory snapshot. Invalid files report a recoverable error and do not expose partially parsed results. Call the service with the current authenticated `Session`; direct learner and guest calls are denied independently of menu visibility.
