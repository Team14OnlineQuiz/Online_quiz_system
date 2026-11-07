import json
import sqlite3
from concurrent.futures import ThreadPoolExecutor

import pytest

from web.app import add_demo, create_app


@pytest.fixture
def app(tmp_path):
    application = create_app(tmp_path, testing=True)
    add_demo(application)
    return application


def token(client):
    return {'X-CSRF-Token': client.get('/api/session').json['csrf']}


def login(client, admin=False):
    response = client.post('/api/login', json={
        'email': 'admin@quizora.test' if admin else 'learner@quizora.test',
        'password': 'Teach@12345' if admin else 'Learn@12345'}, headers=token(client))
    assert response.status_code == 200
    return {'X-CSRF-Token': response.json['csrf']}


def register(client, email='new@example.test'):
    response = client.post('/api/register', json=dict(name='New learner', email=email,
                           password='Strong-pass-123', grade=10, role='admin'), headers=token(client))
    assert response.status_code == 200
    assert response.json['user']['role'] == 'learner'
    return {'X-CSRF-Token': response.json['csrf']}


def test_csrf_login_logout_and_roles(app):
    client = app.test_client()
    assert client.post('/api/login', json={}).status_code == 403
    assert client.get('/api/progress').status_code == 401
    headers = register(client)
    assert client.get('/api/admin/quizzes/1').status_code == 403
    assert client.post('/api/admin/quizzes', json={}, headers=headers).status_code == 403
    assert client.delete('/api/admin/quizzes/1', headers=headers).status_code == 403
    assert client.post('/api/logout', headers=headers).status_code == 200
    assert client.get('/api/progress').status_code == 401
    response = client.post('/api/login', json={'email':'new@example.test','password':'wrong'}, headers=token(client))
    assert response.status_code == 401
    cookie = app.test_client().get('/api/session').headers['Set-Cookie']
    assert 'SameSite=Strict' in cookie and 'HttpOnly' in cookie


def test_passwords_are_hashed_and_persist_on_restart(app):
    client = app.test_client()
    register(client)
    with app.app_context():
        row = app.db().execute("SELECT * FROM users WHERE email='new@example.test'").fetchone()
        assert row['password'].startswith('scrypt:')
        assert 'Strong-pass-123' not in row['password']
    # A fresh browser session can authenticate against the same durable store.
    fresh = app.test_client()
    assert fresh.post('/api/login', json={'email':'new@example.test','password':'Strong-pass-123'}, headers=token(fresh)).status_code == 200


def test_mixed_question_grading_idempotency_and_ownership(app):
    client = app.test_client()
    headers = register(client)
    catalogue = client.get('/api/quizzes').json['quizzes']
    assert {q['grade'] for q in catalogue} == set(range(6,13))
    assert len({q['subject'] for q in catalogue}) == 4
    assert 'questions' not in catalogue[0]
    run = client.post('/api/quizzes/1/start', headers=headers).json
    assert [q['type'] for q in run['questions']] == ['mcq','true_false','short_answer']
    assert all('answer' not in q and 'explanation' not in q for q in run['questions'])
    assert client.post(f"/api/runs/{run['run_id']}/submit", json={'answers':['0.75']}, headers=headers).status_code == 400
    submission = client.post(f"/api/runs/{run['run_id']}/submit", json={'answers':['0.75','True',' 40 ']}, headers=headers)
    assert submission.status_code == 200
    attempt_id = submission.json['attempt_id']
    retry = client.post(f"/api/runs/{run['run_id']}/submit", json={'answers':['0.25','False','9']}, headers=headers)
    assert retry.json['attempt_id'] == attempt_id
    result = client.get(f'/api/attempts/{attempt_id}').json
    assert result['score'] == result['total'] == 3
    assert all(q['correct'] for q in result['review'])
    stranger = app.test_client()
    other_headers = register(stranger, 'other@example.test')
    assert stranger.get(f'/api/attempts/{attempt_id}').status_code == 404
    assert stranger.post(f"/api/runs/{run['run_id']}/submit", json={'answers':['0.75','True','40']}, headers=other_headers).status_code == 404
    assert stranger.get('/api/progress').json['count'] == 0


def test_progress_uses_owned_persisted_results(app):
    client = app.test_client()
    headers = register(client)
    empty = client.get('/api/progress').json
    assert empty['count'] == empty['accuracy'] == empty['streak'] == 0
    run = client.post('/api/quizzes/1/start', headers=headers).json
    client.post(f"/api/runs/{run['run_id']}/submit", json={'answers':['0.75','False','40']}, headers=headers)
    progress = client.get('/api/progress').json
    assert progress['count'] == 1
    assert progress['accuracy'] == 67
    assert progress['streak'] == 1
    assert progress['subjects'][0]['accuracy'] == 67
    assert progress['activity'][-1]['attempts'] == 1
    admin = app.test_client()
    login(admin, True)
    assert admin.get('/api/progress').json['count'] == 13


def test_admin_attempt_review_identifies_learner_and_keeps_all_answer_keys(app):
    learner = app.test_client()
    headers = register(learner)
    run = learner.post('/api/quizzes/1/start', headers=headers).json
    submission = learner.post(f"/api/runs/{run['run_id']}/submit",
                              json={'answers':['0.75','False','40']}, headers=headers)
    url = '/api/attempts/' + str(submission.json['attempt_id'])
    assert 'learner' not in learner.get(url).json

    admin = app.test_client()
    login(admin, True)
    review = admin.get(url).json
    assert review['learner']['name'] == 'New learner'
    assert review['learner']['email'] == 'new@example.test'
    assert review['learner']['grade'] == 10
    assert 'password' not in review['learner']
    assert review['title'] == 'Number sense'
    assert review['grade'] == 6
    assert [q['selected'] for q in review['review']] == ['0.75','False','40']
    assert [q['answer'] for q in review['review']] == ['0.75','True','40']
    assert [q['correct'] for q in review['review']] == [True,False,True]

    stranger = app.test_client()
    register(stranger, 'stranger@example.test')
    assert stranger.get(url).status_code == 404


def test_admin_edits_and_archives_preserve_inflight_and_completed_snapshots(app):
    learner = app.test_client()
    headers = register(learner)
    run = learner.post('/api/quizzes/1/start', headers=headers).json
    admin = app.test_client()
    admin_headers = login(admin, True)
    quiz = admin.get('/api/admin/quizzes/1').json
    quiz['title'] = 'Renamed quiz'
    quiz['questions'][0]['answer'] = '0.25'
    assert admin.put('/api/admin/quizzes/1', json=quiz, headers=admin_headers).status_code == 200
    assert admin.delete('/api/admin/quizzes/1', headers=admin_headers).status_code == 200
    assert learner.post('/api/quizzes/1/start', headers=headers).status_code == 404
    result = learner.post(f"/api/runs/{run['run_id']}/submit", json={'answers':['0.75','True','40']}, headers=headers).json
    attempt = learner.get('/api/attempts/'+str(result['attempt_id'])).json
    assert attempt['score'] == 3
    assert attempt['title'] == 'Number sense'
    assert admin.put('/api/admin/quizzes/9999', json=quiz, headers=admin_headers).status_code == 404


@pytest.mark.parametrize('change', [
    {'grade':True}, {'grade':5}, {'subject':'Unknown'}, {'questions':[]},
    {'title':''}, {'description':123}, {'questions':[{'type':'short_answer'}]},
])
def test_invalid_admin_quizzes_do_not_write(app, change):
    client = app.test_client()
    headers = login(client, True)
    quiz = client.get('/api/admin/quizzes/1').json
    quiz.update(change)
    assert client.post('/api/admin/quizzes', json=quiz, headers=headers).status_code == 400
    assert len(client.get('/api/quizzes').json['quizzes']) == 28


def test_concurrent_submission_writes_one_attempt(app):
    client = app.test_client()
    headers = register(client)
    run = client.post('/api/quizzes/1/start', headers=headers).json
    cookie = client.get_cookie('session').value

    def submit():
        independent = app.test_client()
        independent.set_cookie('session', cookie)
        return independent.post(f"/api/runs/{run['run_id']}/submit", json={'answers':['0.75','True','40']}, headers=headers).json

    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(lambda _: submit(), range(2)))
    assert results[0]['attempt_id'] == results[1]['attempt_id']
    assert client.get('/api/progress').json['count'] == 1


def test_unknown_endpoints_and_security_headers(app):
    client = app.test_client()
    assert client.get('/api/nonexistent').status_code == 404
    response = client.get('/login')
    assert response.status_code == 200
    assert response.headers['X-Frame-Options'] == 'DENY'
    assert "script-src 'self'" in response.headers['Content-Security-Policy']
    assert client.get('/api/session').headers['Cache-Control'] == 'no-store'
