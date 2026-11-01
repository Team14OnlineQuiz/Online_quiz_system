# Online Quiz System

C++17 console application using the selected Deliverable 1 baseline. The root SRS v1.1 describes an older scope; \`docs/implementation-baseline.md\` records the differences.

## Build

Install a C++17 compiler and the official libsodium development library. Password hashing uses the vetted libsodium Argon2id API.

Windows with MinGW and the official libsodium win64 archive:

\`\`\`powershell
$env:SODIUM_ROOT = 'C:\path\to\libsodium-win64'
./scripts/build.ps1
./build/quiz_tests.exe
python scripts/smoke_test.py
./build/quiz.exe
\`\`\`

Linux:

\`\`\`bash
sudo apt-get install g++ pkg-config libsodium-dev
bash scripts/build.sh
./build/quiz_tests
python3 scripts/smoke_test.py
./build/quiz
\`\`\`

Build output and runtime data stay out of Git.

## First use

Run \`build/quiz --init-admin\` (\`build/quiz.exe --init-admin\` on Windows) once in an empty data directory. Enter an administrator username and password at the prompts. There is no default administrator password, and passwords are never passed as command-line arguments. Run the application normally to register learners and sign in. \`--data DIRECTORY\` selects another local data directory.

## Learner workflow

After signing in, learners can choose:

1. **Take quiz** — discover available quizzes by identifier, title, and question count; select an identifier to start.
2. **View my completed history** — see only the signed-in learner's completed attempts, including quiz title, UTC submission date, correct/total count, and percentage.
3. **Logout** — destroy the authenticated session.

The learner-facing question snapshot does not contain the answer key. Answers are graded inside the quiz service. An attempt is reported as saved only after its completed record has been written successfully. An interrupted attempt is not stored as completed.

## Administrator workflow

After signing in with an administrator account, the administrator can:

1. **Question maintenance** — list, add, modify, and archive questions.
2. **Quiz configuration** — list, create, modify, and archive quizzes using question identifiers.
3. **Results summary** — view completed attempt snapshots across learners.
4. **Logout** — destroy the administrator session.

The controller checks the authenticated role before dispatching administrator actions. The catalog and result services independently enforce administrator authorization, so direct learner or guest calls are denied too.

## Local data

All files are versioned UTF-8 files stored under the selected data directory:

- \`users.db\` — accounts and encoded Argon2id hashes.
- \`questions.db\` — question text, options, answer key, and active flag.
- \`quizzes.db\` — stable quiz IDs, titles, and ordered question references.
- \`attempts.db\` — completed attempt snapshots, owner ID, quiz title, UTC date, and score.

Storage adapters stage writes to a sibling \`.tmp\` file and commit using the platform replacement adapter. In-memory records change only after successful replacement. See [storage-format.md](docs/storage-format.md).

See [Akshath Iteration 1 implementation](docs/akshath-iteration-1.md), [Akshay Iteration 1](docs/iteration-1.md), [Akshay Iteration 2](docs/iteration-2.md), and [performance measurements](docs/performance.md). GitHub Actions compiles with warnings treated as errors, runs requirement tests, executes the real console smoke test, and checks the benchmark on Linux and Windows.
