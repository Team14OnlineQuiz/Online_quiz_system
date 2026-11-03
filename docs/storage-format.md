# Local storage integration contract

These versioned UTF-8 files belong to repository adapters, not to console users. All user-facing writes must be validated and committed before success is reported. Filenames are fixed inside the selected \`--data\` directory; usernames never become paths.

## Accounts

\`users.db\` begins with \`QUIZ_USERS_V1\`. Each subsequent line has:

\`\`\`text
userId role "username" "encodedArgon2idHash"
\`\`\`

Role \`0\` means learner, \`1\` means administrator. Strings use the C++ \`std::quoted\` encoding, with backslash escapes for quotes and backslashes. IDs and usernames are unique. Use \`AccountStore::insert\` rather than constructing credential records yourself. \`AuthService::registerLearner\` always selects Learner; \`provisionFirstAdmin\` only works on an empty account store.

The account adapter writes a replacement file before swapping committed in-memory state. OS-dependent replacement is isolated in \`replaceFile\` in \`account_store.cpp\`.

## Attempt snapshots

The administrator summary and learner history consume \`attempts.db\`, beginning with \`QUIZ_ATTEMPTS_V1\`:

\`\`\`text
attemptId userId completed correctCount totalQuestions "quizTitleSnapshot" "submittedAtUTC"
\`\`\`

\`completed\` is \`0\` or \`1\`. Counts satisfy \`0 <= correctCount <= totalQuestions\` and \`totalQuestions > 0\`. Dates use valid calendar timestamps such as \`2026-10-20T10:00:00Z\`. Only completed rows appear in either summary. Unknown account IDs fail the administrator summary instead of becoming anonymous valid scores.

The title and evaluation counts are snapshots: later edits to quiz definitions do not alter past scores. \`FileAttemptStore\` implements both reading and \`AttemptWriter::insert\`. It writes only completed attempts, uses a temporary file followed by atomic replacement, and updates the in-memory snapshot only after the replacement succeeds. \`AttemptService::submit\` derives the owner ID from the authenticated learner session and refuses guest or administrator submissions.

Learner history uses only the owner ID from the current authenticated \`Session\`. \`LearnerHistoryService::historyFor\` also checks that any supplied owner ID equals the signed-in learner's ID, independently denying cross-learner access. The console does not accept an owner ID.

\`FileAttemptStore::reload\` stages all parsed rows before replacing its previous in-memory snapshot. Invalid files report a recoverable error and do not expose partially parsed results.

## Question catalog and quiz definitions

\`questions.db\` begins with \`QUIZ_QUESTIONS_V1\`. Records contain:

\`\`\`text
questionId active correctLetter "prompt" "choiceA" "choiceB" "choiceC" "choiceD"
\`\`\`

IDs are positive and unique, \`active\` is 0 or 1, and \`correctLetter\` is uppercase A-D. Text fields are nonempty, bounded, and contain no control characters. \`QuestionCatalog\` indexes active prompts by ID; its learner-facing \`QuestionPrompt\` excludes the answer key. Missing files produce an empty catalog. Malformed records reject the load. Question creation, editing, and archive operations require an administrator session.

Quiz definitions are persisted in \`quizzes.db\`, with stable quiz IDs, title, active flag, and ordered question IDs. \`QuizCatalog\` provides administrator-only create, modify, archive, listing, and lookup operations. Its learner listing and selection APIs require an authenticated learner session and return no answer keys.

## Failure and restart behavior

Every write first serializes the complete staged record set to a sibling \`.tmp\` file, flushes and closes it, then replaces the committed file through \`replaceFile\`. POSIX uses filesystem rename; Windows uses \`MoveFileExW\` with replacement and write-through flags. The adapter reports a storage error if staging or replacement fails. The previous committed file is not opened for truncation, and in-memory state changes only after the replacement succeeds.

The cross-platform recovery tests inject failures by creating a directory at the temporary-file path. They compare the committed file byte-for-byte before and after the failed write, then construct a fresh store instance to confirm restart reloads the last committed state for accounts, questions, quizzes, and attempts.
