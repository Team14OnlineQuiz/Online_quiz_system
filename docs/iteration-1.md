# Iteration 1 implementation and validation

Owner: sickDoughnut-007. Issues: #1 registration, #3 authentication, #7 input recovery, #16 password hashing.

The implementation is split into small commits for build structure, hashing, account storage, registration, authentication, console integration, input recovery, and CI. Commit times reflect actual work; the original sprint dates are planning metadata and are not backdated.

Local validation on Windows with MinGW g++ 14.2.0 and libsodium 1.0.22:

- Build with C++17, `-Wall -Wextra -Werror -pedantic`.
- 18 requirement tests pass, including persistence across restart, failed-save rollback, credential inspection, generic authentication failure, role-safe sessions, and invalid answers repeating the current question.
- The real executable is checked by `scripts/smoke_test.py` using disposable credentials. It provisions an administrator, rejects repeated provisioning, registers a learner, restarts, signs in both roles, logs out, and inspects persisted credentials.

The committed GitHub Actions workflow runs the build, requirement tests, and executable smoke test on Linux and Windows. Its actual run is the cross-platform evidence; local Windows results alone are not claimed as a Linux pass.

Integration boundaries for teammates:

- `AccountStore` and `Session` provide stable account/session types.
- `ConsoleAction` accepts the authenticated session, input stream, and output stream. The quiz and summary implementations can be connected without parsing credentials again.
- `AnswerSession` accepts an ordered question snapshot and records valid answers only. Scoring, authoring, and attempt persistence are separate tasks.
- Only the user's assigned issues are progressed. Other team members' issues remain open.
