"""Run the real console executable with disposable, synthetic credentials."""
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
            input=text, text=True, capture_output=True, timeout=20,
        )
        assert result.returncode == expected, result.stderr
        return result.stdout

    assert "Administrator account saved." in run(["--init-admin"], "owner\nsetup-test-password\n")
    before = (data / "users.db").read_bytes()
    run(["--init-admin"], "second\nsecond-test-password\n", expected=1)
    assert before == (data / "users.db").read_bytes(), "Repeated setup changed credentials"
    assert "Learner account saved." in run([], "1\nalex\nlearner-test-password\n3\n")
    output = run([], "2\nalex\nlearner-test-password\n2\n3\n")
    assert "Learner menu" in output and "Administrator menu" not in output
    assert "Administrator menu" in run([], "2\nowner\nsetup-test-password\n2\n3\n")
    for credentials in ("missing\nlearner-test-password\n", "alex\nwrong-test-password\n"):
        output = run([], "2\n" + credentials + "3\n")
        assert "Invalid credentials." in output
        assert "Learner menu" not in output and "Administrator menu" not in output
    raw = (data / "users.db").read_text()
    assert "setup-test-password" not in raw and "learner-test-password" not in raw
    assert raw.count("$argon2id$") == 2
    (data / "attempts.db").write_text(
        'QUIZ_ATTEMPTS_V1\n'
        '1 2 1 2 3 "Original quiz" "2026-10-20T10:00:00Z"\n'
        '2 2 0 0 3 "Unsubmitted quiz" "2026-10-20T11:00:00Z"\n'
    )
    output = run([], "2\nowner\nsetup-test-password\n1\n2\n3\n")
    assert '"alex" | "Original quiz" | 2026-10-20T10:00:00Z | 2/3 | 66.7%' in output
    assert "Unsubmitted quiz" not in output
    output = run([], "2\nalex\nlearner-test-password\n1\n2\n3\n")
    assert "Results summary" not in output and "Original quiz" not in output
print("PASS executable setup, registration, restart, role menus, administrator summary, and credential storage")
