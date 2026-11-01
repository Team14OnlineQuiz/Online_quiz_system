"""Exercise real registration, quiz selection, submission, history and summary flows."""
import os
from pathlib import Path
import subprocess
import tempfile

exe = Path("build/quiz.exe" if os.name == "nt" else "build/quiz").resolve()
with tempfile.TemporaryDirectory(prefix="quiz-smoke-", dir="build") as directory:
    data = Path(directory).resolve()

    def run(arguments, text, expected=0):
        result = subprocess.run(
            [str(exe), "--data", str(data), *arguments],
            input=text, text=True, capture_output=True, timeout=30,
        )
        assert result.returncode == expected, (result.stdout, result.stderr)
        return result.stdout

    assert "Administrator account saved." in run(
        ["--init-admin"], "owner\nsetup-test-password\n"
    )
    before = (data / "users.db").read_bytes()
    run(["--init-admin"], "second\nsecond-test-password\n", expected=1)
    assert before == (data / "users.db").read_bytes(), "Repeated setup changed credentials"

    assert "Learner account saved." in run(
        [], "1\nalex\nlearner-test-password\n3\n"
    )
    assert "Learner account saved." in run(
        [], "1\nblair\nlearner-test-password\n3\n"
    )

    (data / "questions.db").write_text(
        'QUIZ_QUESTIONS_V1\n'
        '1 1 B "What is 2 + 2?" "3" "4" "5" "6"\n'
    )
    (data / "quizzes.db").write_text(
        'QUIZ_QUIZZES_V1\n'
        '1 1 "Demo quiz" 1 1\n'
    )
    (data / "attempts.db").write_text(
        'QUIZ_ATTEMPTS_V1\n'
        '1 3 1 1 2 "Other learner quiz" "2026-10-20T10:00:00Z"\n'
        '2 2 0 0 1 "Unsubmitted quiz" "2026-10-20T11:00:00Z"\n'
    )

    output = run(
        [], "2\nalex\nlearner-test-password\n1\n1\nB\n3\n3\n"
    )
    assert "Available quizzes" in output and "Demo quiz" in output
    assert "Attempt saved." in output and "Score: 1/1" in output
    attempts = (data / "attempts.db").read_text()
    assert '"Demo quiz"' in attempts
    assert "learner-test-password" not in (data / "users.db").read_text()

    output = run(
        [], "2\nalex\nlearner-test-password\n2\n3\n3\n"
    )
    assert '"Demo quiz"' in output
    assert "Other learner quiz" not in output
    assert "Unsubmitted quiz" not in output

    output = run(
        [], "2\nowner\nsetup-test-password\n3\n4\n3\n"
    )
    assert '"alex" | "Demo quiz"' in output
    assert '"blair" | "Other learner quiz"' in output
    assert "Unsubmitted quiz" not in output

    for credentials in (
        "missing\nlearner-test-password\n",
        "alex\nwrong-test-password\n",
    ):
        output = run([], "2\n" + credentials + "3\n")
        assert "Invalid credentials." in output
        assert "Learner menu" not in output and "Administrator menu" not in output

    raw = (data / "users.db").read_text()
    assert "setup-test-password" not in raw
    assert "learner-test-password" not in raw
    assert raw.count("$argon2id$") == 3

print("PASS account restart, authenticated quiz discovery, completed submission, learner-owned history, administrator summary, and credential storage")
