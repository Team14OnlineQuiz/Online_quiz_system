# Online Quiz System

C++17 console implementation based on the attached Deliverable 1 used for the GitHub issues. The root SRS v1.1 describes an older scope; `docs/implementation-baseline.md` will track the differences explicitly.

## Build

Install a C++17 compiler and the official libsodium development library. Do not replace password hashing with a home-written algorithm.

Windows with MinGW and the official libsodium win64 archive:

```powershell
$env:SODIUM_ROOT = 'C:\path\to\libsodium-win64'
./scripts/build.ps1
./build/quiz_tests.exe
python scripts/smoke_test.py
./build/quiz_benchmark.exe
./build/quiz.exe
```

Linux:

```bash
sudo apt-get install g++ pkg-config libsodium-dev
bash scripts/build.sh
./build/quiz_tests
python3 scripts/smoke_test.py
./build/quiz_benchmark
./build/quiz
```

Keep runtime data and build output out of Git. Source modules and tests are separated so team members can integrate their assigned stories independently.

## First use

Run `build/quiz --init-admin` (`build/quiz.exe --init-admin` on Windows) once in an empty data directory. Enter a username and password at the prompts. Then run the application normally to register learners and sign in. `--data DIRECTORY` selects another local data directory for either command. There is no default administrator password, and passwords are never passed as command-line arguments.

The learner and administrator menus appear only after authentication. Logout destroys the session. Files contain encoded Argon2id hashes, never plaintext passwords. Authentication failures use one generic message.

Question authoring and quiz configuration belong to other team members. The console accepts a quiz callback for their implementation and shows `No quizzes available.` until a catalog is supplied. That integration boundary does not complete their stories.

`AnswerSession` is the answer-input component for the quiz callback: pass an ordered snapshot of `QuestionPrompt` values and call `readNext` until `complete()`. Invalid input repeats the same question without appending an answer; EOF leaves the partially completed state available to the caller. `answers()` contains one valid uppercase A-D choice per accepted question. Scoring and completion persistence remain separate services.

Administrators can view completed attempt snapshots through the summary menu. The shared attempt writer must follow [the storage contract](docs/storage-format.md). Missing attempt data produces an empty summary; invalid data produces a recoverable error. Learners and unauthenticated sessions cannot call the summary service.

See [Iteration 1](docs/iteration-1.md), [Iteration 2 and platform commands](docs/iteration-2.md), and [performance measurements](docs/performance.md). GitHub Actions builds and runs the tests, executable smoke, and performance gate on both Windows and Linux.
