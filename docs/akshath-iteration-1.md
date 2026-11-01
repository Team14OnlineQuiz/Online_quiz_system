# Akshath — Iteration 1 implementation

Branch: \`akshath-iteration-1\`  
Baseline: \`akshay-iteration-2\`  
Issues: #2 (quiz discovery and selection), #11 (record persistence), #17 (administrator authorization).

## Implementation

- Extended the validated question catalog with administrator-only create, edit, and archive operations. Question records are staged in a sibling temporary file and committed with the repository's platform-specific atomic replacement adapter.
- Added a persistent quiz catalog with stable identifiers, title snapshots, and ordered question references. Quiz configuration operations independently require an administrator session.
- Added learner-only quiz discovery and selection. The learner menu lists quiz identifiers, titles, and question counts; selecting a valid identifier starts an answer session without exposing the answer key.
- Added completed-attempt persistence. A result is reported as saved only after the full attempt has been written and the staged file has been atomically replaced. Incomplete attempts are not committed.
- Extended the console with separate learner history and administrator-only question maintenance, quiz configuration, and aggregate results actions. Authorization is checked at the controller boundary and again in the underlying services.
- Preserved the existing Argon2id account store, session model, and cross-platform build/test approach.

## Storage files

- \`users.db\`: existing account records (already persistent in the baseline).
- \`questions.db\`: versioned question records, including active flag and answer key.
- \`quizzes.db\`: versioned quiz records with stable IDs and ordered question IDs.
- \`attempts.db\`: versioned completed-attempt snapshots with owner ID, quiz title, UTC submission time, and score counts.

Writes use a \`.tmp\` staging file and the existing \`replaceFile\` adapter. In-memory state is updated only after the replacement succeeds.

## Verification added

- Learner discovery is denied for guest and administrator sessions.
- Quiz selection rejects unknown identifiers and starts a real answer flow for valid identifiers.
- Completed submissions survive a new application/store instance; incomplete attempts are not saved.
- Guest and learner calls to question maintenance, quiz configuration, and aggregate results are rejected by service methods.
- Learner attempts are bound to the authenticated session owner.
- Injected failures for question, quiz, and attempt writes leave the previous committed file readable after reload.
- The executable smoke test covers account registration, authenticated quiz discovery, answer submission, learner-owned history, and administrator summary.
- CI push filters include the new \`akshath-iteration-*\` branches.

## Validation status

The GitHub Actions workflow is the authoritative compiler/test gate for Linux and Windows. Local container execution in this session does not have the libsodium development library installed, so no local compile pass is claimed. The PR will remain open for CI results and review; it will not be merged automatically.
