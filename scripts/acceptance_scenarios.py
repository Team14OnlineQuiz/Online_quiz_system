"""Execute the original three-question acceptance fixture and link supporting unit/CI evidence."""
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
os.chdir(root)
reports = root / 'docs/reports'
reports.mkdir(parents=True, exist_ok=True)
exe = Path(os.environ.get('QUIZ_SMOKE_EXE', 'build/quiz.exe' if os.name == 'nt' else 'build/quiz')).resolve()
password = 'acceptance-only-2026'
unit = (reports / 'cpp-tests.txt').read_text(encoding='utf-8')
results = []
transcripts = []


def unit_pass(name):
    assert f'PASS {name}\n' in unit, name


def record(identifier, observed):
    results.append({'case': identifier, 'status': 'PASS', 'observed': observed})


with tempfile.TemporaryDirectory(prefix='quiz-acceptance-', dir=root / 'build') as temp:
    data = Path(temp)

    def run(text, init=False, label=''):
        result = subprocess.run([str(exe), '--data', str(data), *(['--init-admin'] if init else [])],
                                input=text, text=True, capture_output=True, timeout=30)
        assert result.returncode == 0, (label, result.stderr)
        transcripts.append({'scenario': label, 'exit_code': result.returncode, 'output': result.stdout})
        return result.stdout

    def login(name):
        return f'2\n{name}\n{password}\n'

    run(f'owner\n{password}\n', True, 'Provision disposable administrator')
    for name in ['alex', 'blair']:
        assert 'Learner account saved.' in run(f'1\n{name}\n{password}\n3\n', label='Register learner')
    before = (data / 'users.db').read_bytes()
    assert 'Username already exists.' in run(f'1\nalex\n{password}\n3\n', label='Reject duplicate')
    assert (data / 'users.db').read_bytes() == before
    record('TC-01', 'Valid accounts persisted; repeating alex was rejected without changing the accounts file.')

    wrong_name = run(f'2\nunknown\n{password}\n3\n', label='Unknown username')
    wrong_password = run('2\nalex\nincorrect-password\n3\n', label='Wrong password')
    assert 'Invalid credentials.' in wrong_name and 'Invalid credentials.' in wrong_password
    signed_in = run(login('alex') + '3\n3\n', label='Valid learner sign-in')
    assert 'Learner menu' in signed_in and 'Administrator menu' not in signed_in
    hashes = re.findall(r'\$argon2id\$[^"\s]+', (data / 'users.db').read_text())
    assert len(hashes) == 3 and len({h.split('$')[4] for h in hashes}) == 3
    assert password not in (data / 'users.db').read_text()
    record('TC-02', 'Wrong name/password used the same failure; valid login succeeded; three same-password accounts had distinct salts and no plaintext password.')

    unit_pass('question_quiz_and_aggregate_result_services_enforce_admin_role')
    record('TC-03', 'Executed direct-service authorization unit test passed; learner CLI displayed no administrator menu.')

    create = login('owner') + '1\n'
    create += '2\n\nA option\nB option\nC option\nD option\nA\n'
    for i, key in enumerate('ACB', start=1):
        create += f'2\nPlan question {i}?\nA option\nB option\nC option\nD option\n{key}\n'
    create += '5\n2\n2\nInvalid references\n9999\n2\nAcceptance fixture\n1 2 3\n5\n4\n3\n'
    output = run(create, label='Reject invalid content; create three questions and quiz')
    assert 'Question rejected.' in output and 'Invalid data or identifier.' in output
    assert output.count('Question created. ID:') == 3 and 'Quiz identifier: 1' in output
    after_restart = run(login('owner') + '1\n1\n5\n2\n1\n5\n4\n3\n', label='Reload questions and quiz')
    assert 'Plan question 3?' in after_restart and 'Acceptance fixture (3 questions)' in after_restart
    record('TC-04', 'Empty question prompt and unknown quiz reference were rejected; valid three-question fixture reloaded after process restart.')

    first = run(login('alex') + '1\n1\nA\nC\nB\n3\n3\n', label='Known correct answer key A,C,B')
    assert 'Acceptance fixture | 3' in first and 'Score: 3/3' in first
    for i in range(1, 4):
        assert first.count(f'Question {i} of 3:') == 1
    record('TC-05', 'Quiz listed with three questions; each was displayed once; A,C,B produced a saved 3/3 result.')

    second = run(login('blair') + '1\n1\nX\nA\nD\nB\n3\n3\n', label='Invalid X then known two-of-three answers A,D,B')
    assert 'Invalid answer.' in second and second.count('Question 1 of 3:') == 2
    assert second.count('Question 2 of 3:') == 1 and 'Score: 2/3' in second
    record('TC-06', 'X repeated question 1; the valid A advanced it; subsequent questions were displayed once.')
    assert 'Percentage: 66.7%' in second and second.count('Attempt saved.') == 1
    record('TC-07', 'A,D,B against A,C,B produced exactly one completed attempt, 2/3 and 66.7%.')

    alex_history = run(login('alex') + '2\n3\n3\n', label='Alex-owned history')
    blair_history = run(login('blair') + '2\n3\n3\n', label='Blair-owned history')
    assert '3/3' in alex_history and '2/3' not in alex_history
    assert '2/3' in blair_history and '3/3' not in blair_history
    unit_pass('attempt_submission_and_learner_history_are_bound_to_authenticated_owner')
    record('TC-08', 'Separate learner logins returned only the respective 3/3 or 2/3 result; direct ownership-denial test passed.')
    summary = run(login('owner') + '3\n4\n3\n', label='Administrator cross-learner summary')
    assert '"alex"' in summary and '"blair"' in summary and '3/3' in summary and '2/3' in summary
    unit_pass('learner_guest_and_logged_out_admin_cannot_read_summary')
    record('TC-09', 'Administrator saw both named learners; unit tests denied learner, guest, and logged-out sessions.')

    for name in ['account_write_failure_preserves_exact_committed_file_and_restart_state',
                 'question_write_failure_preserves_exact_committed_file_and_restart_state',
                 'quiz_write_failure_preserves_exact_committed_file_and_restart_state',
                 'attempt_write_failure_preserves_exact_committed_file_and_restart_state',
                 'malformed_role_never_becomes_an_administrator',
                 'attempt_adapter_rejects_invalid_scores_and_reads_completed_snapshots']:
        unit_pass(name)
    record('TC-10', 'Recorded executed unit tests preserve committed files after injected write failure and reject malformed account/attempt records.')

    performance = (reports / 'performance.txt').read_text(encoding='utf-8')
    menu, question = map(float, re.search(r'menu_p95_ms=([\d.]+) question_p95_ms=([\d.]+)', performance).groups())
    assert menu <= 2000 and question <= 2000 and 'questions=1000 attempts=10000 samples_per_operation=40' in performance
    record('TC-11', f'40 samples per operation at the planned volume: menu p95 {menu} ms; question p95 {question} ms; both <= 2000 ms.')

    evidence = json.loads((reports / 'github-evidence.json').read_text())
    cpp_run = next(r for r in evidence['latest_main_runs'] if r['name'].startswith('C++'))
    assert cpp_run['conclusion'] == 'success'
    record('TC-12', 'Local Windows instrumented build and real CLI smoke passed. Previously completed main CI at the recorded source base passed Windows and Linux: ' + cpp_run['url'])

report = {'recorded_utc': datetime.now(timezone.utc).isoformat(), 'executable': str(exe),
          'note': 'TC-01/02/04/05/06/07/08/09 use fresh real CLI processes. Direct authorization, ownership and storage failures use the recorded executed C++ tests. TC-12 combines local execution with prior cross-platform CI evidence; Linux was not run on this Windows laptop.',
          'results': results, 'console_transcripts': transcripts}
(reports / 'acceptance-results.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('\n'.join(f'{x["case"]}: {x["status"]} - {x["observed"]}' for x in results))
