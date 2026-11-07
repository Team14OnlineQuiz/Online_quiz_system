# Quizora web application

The Iteration 3 website adds learner and administrator workspaces, a searchable catalogue, quiz authoring, mixed-format attempts, answer reviews, and progress analytics. It runs as a Flask application with a durable SQLite database.

## Run locally

From the repository root, with Python 3.12 or later:

```powershell
python -m venv .venv
.venv/Scripts/python -m pip install -r web/requirements.txt
.venv/Scripts/python -m web.app --demo
```

On Linux or macOS, use `.venv/bin/python` in place of `.venv/Scripts/python`.

Open <http://127.0.0.1:5000>. Optional `--port 5001` changes the port. `--data DIRECTORY` selects the data directory, which defaults to `data/web`.

The explicit `--demo` option creates two local demonstration accounts:

| Role | Email | Password |
| --- | --- | --- |
| Learner | learner@quizora.test | Learn@12345 |
| Administrator | admin@quizora.test | Teach@12345 |

The demo learner has sample history, identified by a demo-workspace notice. Real attempts are saved when you finish quizzes. Newly registered accounts start with empty progress. Restarting with `--demo` preserves the existing accounts and avoids duplicating sample attempts.

For a fresh workspace, choose another data directory and omit `--demo`. Register learner accounts in the browser. Create the first administrator interactively:

```powershell
.venv/Scripts/python -m web.app --data data/my-web --init-admin
.venv/Scripts/python -m web.app --data data/my-web
```

## Pages and behavior

- `/login` and `/register` support sign-in and learner registration.
- `/dashboard` adapts the home page to the authenticated role.
- `/quizzes` filters the catalogue by subject, grade, and search text. Teachers can edit or archive quizzes here.
- `/quiz/<run>` presents multiple-choice, true/false, and short-answer questions with back/next navigation and unanswered validation.
- `/results/<attempt>` shows a saved score and explanation for each answer.
- `/progress` shows accuracy, study time, streak, daily activity, subject performance, and result history. Administrator accounts see class aggregates.
- `/admin/new` and `/admin/edit/<id>` provide the quiz editor. Each quiz can contain 1–30 questions.

The initial catalogue contains 28 short quizzes across Mathematics, Science, English, and Computing for grades 6–12. Starter content is grouped by the grade bands 6–8, 9–10, and 11–12. Administrators can replace or extend it for their curriculum.

## Data and security

The web application uses its own `quiz.sqlite3` database. Existing C++ console `.db` files and credentials require a separate migration before they can be used on the website. The C++ application remains available through its existing build scripts.

Passwords use Werkzeug's scrypt hashes. Signed sessions use an installation-specific key stored in the selected data directory, with HttpOnly and SameSite=Strict cookies. Mutation requests require a session-bound CSRF token. Role and ownership checks run on the server. Quiz-start responses omit answer keys. Starting a quiz saves a server-side snapshot so later edits do not change its grading. A unique run identifier and SQLite transaction make repeated or concurrent submissions return the same saved result.

Each question has equal weight. Short answers compare Unicode-normalized text while ignoring case and repeated whitespace. They use exact matching after that normalization. Dates and chart day boundaries use UTC. Study time is elapsed time between starting and submitting a quiz, capped at 24 hours per attempt.

The built-in server binds to loopback for local development. For an internet deployment, use a production WSGI server behind HTTPS, set `QUIZ_HTTPS=1`, provide a durable `QUIZ_SECRET_KEY` or persistent private data directory, and configure hosting for that environment. Runtime database and session keys are ignored by Git.

## Validation

```powershell
.venv/Scripts/python -m pip install pytest==9.2.0
.venv/Scripts/python -m pytest web -q
node --check web/static/app.js
```

CI runs web tests on Windows and Linux. Tests cover authentication and CSRF, role restrictions, all question types, answer-key redaction, ownership, empty and populated analytics, quiz edits and archives, persistent hashes, malformed input, and concurrent submission idempotency.

Iteration 3 work is tracked in issues #32–#36, assigned to `sickDoughnut-007`, for 31 story points. The intended board sequence is Todo → In progress → Done. Project iteration and board updates require access to Team14Project through the GitHub browser session.
