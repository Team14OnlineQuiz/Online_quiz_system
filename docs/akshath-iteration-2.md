# Akshath — Iteration 2 implementation

Branch: \`akshath-iteration-2\`  
Base: \`akshath-iteration-1\`  
Issues: #9 (learner completed-attempt history), #13 (preserve committed files on write failure), #18 (enforce history ownership).

## Learner history and ownership

- Added a separate **View my completed history** option to the authenticated learner menu.
- History derives the owner from \`Session::userId()\`; the console never asks for a user ID.
- \`LearnerHistoryService::historyFor\` independently rejects guest sessions, administrator sessions, invalid owners, and any owner ID other than the signed-in learner's ID.
- Only completed attempts for the authenticated learner are returned. Each row contains quiz title snapshot, UTC submitted date, correct count, total questions, and percentage.
- Unfinished attempts and attempts belonging to other learners are excluded. Attempt titles and counts are snapshots, so later quiz edits do not rewrite historical results.

## File-write recovery

- Added byte-for-byte recovery tests for \`users.db\`, \`questions.db\`, \`quizzes.db\`, and \`attempts.db\`.
- Each test first creates a committed record set, injects a staging-file failure by blocking the corresponding \`.tmp\` path, verifies the write returns \`Error::Storage\`, and compares the committed data file to its original bytes.
- Each test then constructs a fresh store instance and verifies the last committed records are still readable after restart.
- The implementation uses the common \`replaceFile\` adapter: atomic rename on POSIX and replacement move on Windows. In-memory vectors are swapped only after replacement succeeds.

## Verification

The test suite covers multi-learner isolation, direct cross-owner service calls, completed-only filtering, history percentages, and write failure/restart recovery for all four persisted record types. The executable smoke test exercises the real learner history menu after a completed quiz submission.

GitHub Actions validates the branch on Linux and Windows with warning-as-error C++17 builds, the full requirement suite, executable smoke test, and the performance gate.
