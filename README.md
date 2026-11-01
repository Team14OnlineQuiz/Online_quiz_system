# Online Quiz System

C++17 console implementation based on the attached Deliverable 1 used for the GitHub issues. The root SRS v1.1 describes an older scope; `docs/implementation-baseline.md` will track the differences explicitly.

## Build

Install a C++17 compiler and the official libsodium development library. Do not replace password hashing with a home-written algorithm.

Windows with MinGW and the official libsodium win64 archive:

```powershell
$env:SODIUM_ROOT = 'C:\path\to\libsodium-win64'
./scripts/build.ps1
./build/quiz_tests.exe
./build/quiz.exe
```

Linux:

```bash
sudo apt-get install g++ pkg-config libsodium-dev
bash scripts/build.sh
./build/quiz_tests
./build/quiz
```

Keep runtime data and build output out of Git. Source modules and tests are separated so team members can integrate their assigned stories independently.
