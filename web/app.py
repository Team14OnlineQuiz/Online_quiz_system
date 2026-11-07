"""Quizora web application. Run with --demo to create local demonstration accounts."""
import argparse
from datetime import datetime, timedelta, timezone
from functools import wraps
import json
import os
from pathlib import Path
import re
import secrets
import sqlite3
import time
import unicodedata

from flask import Flask, g, jsonify, request, send_from_directory, session
from werkzeug.exceptions import HTTPException
from werkzeug.security import check_password_hash, generate_password_hash

from web.seed import SUBJECTS, seed_catalog


def utcnow():
    return datetime.now(timezone.utc)


def normalize(value):
    return ' '.join(unicodedata.normalize('NFKC', value).casefold().split())


def create_app(data_dir=None, testing=False):
    data = Path(data_dir or os.environ.get('QUIZ_WEB_DATA', 'data/web')).resolve()
    data.mkdir(parents=True, exist_ok=True)
    secret_file = data / '.session-key'
    if not secret_file.exists():
        try:
            with secret_file.open('x') as file:
                file.write(secrets.token_hex(32))
            secret_file.chmod(0o600)
        except FileExistsError:
            pass
    app = Flask(__name__, static_folder='static', static_url_path='/static')
    app.config.update(SECRET_KEY=os.environ.get('QUIZ_SECRET_KEY') or secret_file.read_text(),
                      TESTING=testing, MAX_CONTENT_LENGTH=256_000,
                      SESSION_COOKIE_HTTPONLY=True, SESSION_COOKIE_SAMESITE='Strict',
                      SESSION_COOKIE_SECURE=os.environ.get('QUIZ_HTTPS') == '1',
                      PERMANENT_SESSION_LIFETIME=timedelta(hours=8))

    def db():
        if 'db' not in g:
            g.db = sqlite3.connect(data / 'quiz.sqlite3', timeout=15)
            g.db.row_factory = sqlite3.Row
            g.db.execute('PRAGMA foreign_keys=ON')
        return g.db

    @app.teardown_appcontext
    def close_db(error=None):
        connection = g.pop('db', None)
        if connection:
            connection.close()

    with app.app_context():
        db().executescript('''
            CREATE TABLE IF NOT EXISTS users(
              id INTEGER PRIMARY KEY, name TEXT NOT NULL, email TEXT UNIQUE NOT NULL,
              password TEXT NOT NULL, role TEXT NOT NULL CHECK(role IN ('learner','admin')),
              grade INTEGER NOT NULL, demo INTEGER NOT NULL DEFAULT 0);
            CREATE TABLE IF NOT EXISTS quizzes(
              id INTEGER PRIMARY KEY, title TEXT NOT NULL, subject TEXT NOT NULL,
              grade INTEGER NOT NULL, description TEXT NOT NULL, questions TEXT NOT NULL,
              active INTEGER NOT NULL DEFAULT 1);
            CREATE TABLE IF NOT EXISTS runs(
              id TEXT PRIMARY KEY, user_id INTEGER NOT NULL REFERENCES users(id),
              snapshot TEXT NOT NULL, started REAL NOT NULL);
            CREATE TABLE IF NOT EXISTS attempts(
              id INTEGER PRIMARY KEY, run_id TEXT UNIQUE NOT NULL,
              user_id INTEGER NOT NULL REFERENCES users(id), title TEXT NOT NULL,
              subject TEXT NOT NULL, grade INTEGER NOT NULL, score INTEGER NOT NULL,
              total INTEGER NOT NULL, seconds INTEGER NOT NULL,
              submitted TEXT NOT NULL, review TEXT NOT NULL, demo INTEGER DEFAULT 0);
            CREATE TABLE IF NOT EXISTS login_failures(key TEXT NOT NULL, at REAL NOT NULL);
            CREATE INDEX IF NOT EXISTS attempts_owner ON attempts(user_id, submitted);
        ''')
        seed_catalog(db())

    def fail(message, status=400):
        return jsonify(error=message), status

    def payload():
        value = request.get_json(silent=True)
        return value if isinstance(value, dict) else {}

    def public_user(user):
        return {key: user[key] for key in ('id', 'name', 'email', 'role', 'grade', 'demo')}

    def require_user(admin=False):
        def decorate(function):
            @wraps(function)
            def wrapped(*args, **kwargs):
                user = db().execute('SELECT * FROM users WHERE id=?', (session.get('user_id'),)).fetchone()
                if not user:
                    return fail('Please sign in to continue.', 401)
                if admin and user['role'] != 'admin':
                    return fail('Administrator access is required.', 403)
                g.user = user
                return function(*args, **kwargs)
            return wrapped
        return decorate

    @app.before_request
    def csrf():
        if request.path.startswith('/api/') and request.method in ('POST', 'PUT', 'DELETE', 'PATCH'):
            expected = session.get('csrf')
            supplied = request.headers.get('X-CSRF-Token', '')
            if not expected or not secrets.compare_digest(expected, supplied):
                return fail('Your session expired. Refresh the page and try again.', 403)

    @app.after_request
    def headers(response):
        response.headers['X-Content-Type-Options'] = 'nosniff'
        response.headers['X-Frame-Options'] = 'DENY'
        response.headers['Referrer-Policy'] = 'same-origin'
        response.headers['Content-Security-Policy'] = "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; connect-src 'self'; frame-ancestors 'none'; base-uri 'self'; form-action 'self'"
        if request.path.startswith('/api/'):
            response.headers['Cache-Control'] = 'no-store'
        return response

    @app.errorhandler(HTTPException)
    def http_error(error):
        return fail(error.description, error.code)

    @app.get('/api/session')
    def get_session():
        session.setdefault('csrf', secrets.token_hex(24))
        user = db().execute('SELECT * FROM users WHERE id=?', (session.get('user_id'),)).fetchone()
        return jsonify(user=public_user(user) if user else None, csrf=session['csrf'])

    @app.post('/api/register')
    def register():
        values = payload()
        name, email, password = (values.get(key, '') for key in ('name', 'email', 'password'))
        grade = values.get('grade', 8)
        if not all(isinstance(v, str) for v in (name, email, password)):
            return fail('Enter valid account details.')
        if not 1 <= len(name.strip()) <= 80 or not re.fullmatch(r'[^\s@]+@[^\s@]+\.[^\s@]+', email) or len(email) > 160:
            return fail('Enter your name and a valid email address.')
        if not 8 <= len(password) <= 128:
            return fail('Use a password between 8 and 128 characters.')
        if type(grade) is not int or grade not in range(6, 13):
            return fail('Choose a grade between 6 and 12.')
        try:
            cursor = db().execute('INSERT INTO users(name,email,password,role,grade) VALUES(?,?,?,?,?)',
                                  (name.strip(), email.strip().lower(), generate_password_hash(password), 'learner', grade))
            db().commit()
        except sqlite3.IntegrityError:
            return fail('An account with this email already exists.', 409)
        session.clear()
        session.update(user_id=cursor.lastrowid, csrf=secrets.token_hex(24))
        return get_session()

    @app.post('/api/login')
    def login():
        values = payload()
        email, password = values.get('email', ''), values.get('password', '')
        if not isinstance(email, str) or not isinstance(password, str) or len(email) > 160 or len(password) > 128:
            return fail('Email or password is incorrect.', 401)
        key = (request.remote_addr or '') + ':' + email.lower()
        db().execute('DELETE FROM login_failures WHERE at<?', (time.time() - 600,))
        if db().execute('SELECT COUNT(*) FROM login_failures WHERE key=?', (key,)).fetchone()[0] >= 10:
            return fail('Too many sign-in attempts. Try again in 10 minutes.', 429)
        user = db().execute('SELECT * FROM users WHERE email=?', (email.strip().lower(),)).fetchone()
        # A dummy hash keeps unknown-account failures on the password verification path.
        valid = check_password_hash(user['password'] if user else app.config['DUMMY_HASH'], password)
        if not user or not valid:
            db().execute('INSERT INTO login_failures VALUES(?,?)', (key, time.time()))
            db().commit()
            return fail('Email or password is incorrect.', 401)
        db().execute('DELETE FROM login_failures WHERE key=?', (key,))
        db().commit()
        session.clear()
        session.update(user_id=user['id'], csrf=secrets.token_hex(24))
        session.permanent = True
        return get_session()

    app.config['DUMMY_HASH'] = generate_password_hash(secrets.token_hex(20))

    @app.post('/api/logout')
    def logout():
        session.clear()
        return jsonify(ok=True)

    @app.get('/api/quizzes')
    @require_user()
    def quizzes():
        rows = db().execute('SELECT * FROM quizzes WHERE active=1 ORDER BY grade,subject').fetchall()
        result = []
        for row in rows:
            item = dict(row)
            questions = json.loads(item.pop('questions'))
            item.update(count=len(questions), types=sorted({q['type'] for q in questions}), minutes=max(2, len(questions)*2))
            result.append(item)
        return jsonify(quizzes=result, subjects=SUBJECTS)

    @app.post('/api/quizzes/<int:quiz_id>/start')
    @require_user()
    def start_quiz(quiz_id):
        if g.user['role'] != 'learner':
            return fail('Use a learner account to take quizzes.', 403)
        row = db().execute('SELECT * FROM quizzes WHERE id=? AND active=1', (quiz_id,)).fetchone()
        if not row:
            return fail('This quiz is unavailable.', 404)
        snapshot = dict(row)
        snapshot['questions'] = json.loads(snapshot['questions'])
        run_id = secrets.token_hex(24)
        db().execute('INSERT INTO runs VALUES(?,?,?,?)', (run_id, g.user['id'], json.dumps(snapshot), time.time()))
        db().commit()
        safe = [{k: q[k] for k in ('type', 'prompt', 'options')} for q in snapshot['questions']]
        return jsonify(run_id=run_id, title=row['title'], subject=row['subject'], grade=row['grade'], questions=safe)

    @app.post('/api/runs/<run_id>/submit')
    @require_user()
    def submit(run_id):
        # Lock before reading so a retry or concurrent submit returns the same attempt.
        db().execute('BEGIN IMMEDIATE')
        run = db().execute('SELECT * FROM runs WHERE id=? AND user_id=?', (run_id, g.user['id'])).fetchone()
        if not run:
            return fail('Quiz attempt not found.', 404)
        existing = db().execute('SELECT id FROM attempts WHERE run_id=?', (run_id,)).fetchone()
        if existing:
            return jsonify(attempt_id=existing['id'])
        answers = payload().get('answers')
        quiz = json.loads(run['snapshot'])
        questions = quiz['questions']
        if not isinstance(answers, list) or len(answers) != len(questions) or any(not isinstance(a, str) or not a.strip() or len(a) > 300 for a in answers):
            return fail('Answer every question before submitting.')
        review = []
        for question, answer in zip(questions, answers):
            if question['type'] != 'short_answer' and answer not in question['options']:
                return fail('Choose a listed answer option.')
            review.append({**question, 'selected': answer, 'correct': normalize(answer) == normalize(question['answer'])})
        score = sum(q['correct'] for q in review)
        elapsed = max(1, min(86400, int(time.time() - run['started'])))
        cursor = db().execute('''INSERT INTO attempts(run_id,user_id,title,subject,grade,score,total,seconds,submitted,review)
                              VALUES(?,?,?,?,?,?,?,?,?,?)''', (run_id, g.user['id'], quiz['title'], quiz['subject'], quiz['grade'], score, len(review), elapsed, utcnow().isoformat(), json.dumps(review)))
        db().commit()
        return jsonify(attempt_id=cursor.lastrowid)

    @app.get('/api/attempts/<int:attempt_id>')
    @require_user()
    def attempt(attempt_id):
        row = db().execute('SELECT * FROM attempts WHERE id=?', (attempt_id,)).fetchone()
        if not row or (row['user_id'] != g.user['id'] and g.user['role'] != 'admin'):
            return fail('Result not found.', 404)
        result = dict(row)
        result['review'] = json.loads(result['review'])
        result.pop('run_id')
        if g.user['role'] == 'admin':
            learner = db().execute('SELECT id,name,email,grade FROM users WHERE id=?',
                                   (row['user_id'],)).fetchone()
            result['learner'] = dict(learner)
        return jsonify(result)

    @app.get('/api/progress')
    @require_user()
    def progress():
        admin = g.user['role'] == 'admin'
        where = '' if admin else 'WHERE a.user_id=?'
        args = () if admin else (g.user['id'],)
        rows = [dict(r) for r in db().execute(f'''SELECT a.id,a.title,a.subject,a.grade,a.score,a.total,a.seconds,a.submitted,a.demo,u.name
                    FROM attempts a JOIN users u ON a.user_id=u.id {where} ORDER BY a.submitted DESC''', args)]
        total = sum(a['total'] for a in rows)
        accuracy = round(100 * sum(a['score'] for a in rows) / total) if total else 0
        by_subject = []
        for subject in SUBJECTS:
            selected = [a for a in rows if a['subject'] == subject]
            count = sum(a['total'] for a in selected)
            by_subject.append(dict(subject=subject, attempts=len(selected), accuracy=round(100*sum(a['score'] for a in selected)/count) if count else 0))
        activity = []
        for days in reversed(range(14)):
            date = (utcnow() - timedelta(days=days)).date().isoformat()
            selected = [a for a in rows if a['submitted'].startswith(date)]
            count = sum(a['total'] for a in selected)
            activity.append(dict(date=date, attempts=len(selected), accuracy=round(100*sum(a['score'] for a in selected)/count) if count else None))
        active_dates = {a['submitted'][:10] for a in rows}
        day = utcnow().date()
        if day.isoformat() not in active_dates:
            day -= timedelta(days=1)
        streak = 0
        while day.isoformat() in active_dates:
            streak += 1
            day -= timedelta(days=1)
        return jsonify(attempts=rows, accuracy=accuracy, count=len(rows), minutes=round(sum(a['seconds'] for a in rows)/60),
                       subjects=by_subject, activity=activity, streak=streak,
                       learners=db().execute("SELECT COUNT(*) FROM users WHERE role='learner'").fetchone()[0] if admin else None)

    def valid_quiz(values):
        if not isinstance(values, dict):
            return False
        if not isinstance(values.get('title'), str) or not 1 <= len(values['title'].strip()) <= 120:
            return False
        if values.get('subject') not in SUBJECTS or type(values.get('grade')) is not int or values['grade'] not in range(6, 13):
            return False
        if not isinstance(values.get('description', ''), str) or len(values.get('description', '')) > 400:
            return False
        questions = values.get('questions')
        if not isinstance(questions, list) or not 1 <= len(questions) <= 30:
            return False
        for q in questions:
            if not isinstance(q, dict) or q.get('type') not in ('mcq', 'true_false', 'short_answer'):
                return False
            if any(not isinstance(q.get(k), str) or not 1 <= len(q[k].strip()) <= 600 for k in ('prompt', 'answer', 'explanation')):
                return False
            options = q.get('options')
            if not isinstance(options, list):
                return False
            if q['type'] == 'mcq' and (len(options) != 4 or len(set(str(o) for o in options)) != 4):
                return False
            if q['type'] == 'true_false' and options != ['True', 'False']:
                return False
            if q['type'] == 'short_answer' and options:
                return False
            if any(not isinstance(o, str) or not o.strip() or len(o) > 200 for o in options):
                return False
            if q['type'] != 'short_answer' and q['answer'] not in options:
                return False
        return True

    @app.get('/api/admin/quizzes/<int:quiz_id>')
    @require_user(admin=True)
    def admin_quiz(quiz_id):
        row = db().execute('SELECT * FROM quizzes WHERE id=?', (quiz_id,)).fetchone()
        if not row:
            return fail('Quiz not found.', 404)
        result = dict(row)
        result['questions'] = json.loads(result['questions'])
        return jsonify(result)

    @app.route('/api/admin/quizzes', methods=['POST'])
    @app.route('/api/admin/quizzes/<int:quiz_id>', methods=['PUT'])
    @require_user(admin=True)
    def save_quiz(quiz_id=None):
        values = payload()
        if not valid_quiz(values):
            return fail('Check the quiz details. Each question needs a prompt, a correct answer, and an explanation.')
        args = (values['title'].strip(), values['subject'], values['grade'], values.get('description', ''), json.dumps(values['questions']))
        if quiz_id is None:
            cursor = db().execute('INSERT INTO quizzes(title,subject,grade,description,questions) VALUES(?,?,?,?,?)', args)
            quiz_id = cursor.lastrowid
        else:
            if not db().execute('SELECT id FROM quizzes WHERE id=? AND active=1', (quiz_id,)).fetchone():
                return fail('Quiz not found.', 404)
            db().execute('UPDATE quizzes SET title=?,subject=?,grade=?,description=?,questions=? WHERE id=?', args + (quiz_id,))
        db().commit()
        return jsonify(id=quiz_id)

    @app.delete('/api/admin/quizzes/<int:quiz_id>')
    @require_user(admin=True)
    def archive_quiz(quiz_id):
        cursor = db().execute('UPDATE quizzes SET active=0 WHERE id=? AND active=1', (quiz_id,))
        db().commit()
        if cursor.rowcount == 0:
            return fail('Quiz not found.', 404)
        return jsonify(ok=True)

    @app.get('/')
    @app.get('/<path:path>')
    def page(path=''):
        if path.startswith('api/'):
            return fail('Endpoint not found.', 404)
        return send_from_directory(app.static_folder, 'index.html')

    app.db = db
    return app


def add_demo(app):
    with app.app_context():
        db = app.db()
        for name, email, password, role in [('Alex Morgan', 'learner@quizora.test', 'Learn@12345', 'learner'), ('Sam Rivera', 'admin@quizora.test', 'Teach@12345', 'admin')]:
            db.execute('INSERT OR IGNORE INTO users(name,email,password,role,grade,demo) VALUES(?,?,?,?,8,1)',
                       (name, email, generate_password_hash(password), role))
        owner = db.execute('SELECT id FROM users WHERE email=?', ('learner@quizora.test',)).fetchone()[0]
        for i in range(12):
            run_id = f'demo-{i}'
            quiz = db.execute('SELECT * FROM quizzes WHERE id=?', (1 + i % 8,)).fetchone()
            questions = json.loads(quiz['questions'])
            review = [{**q, 'selected': q['answer'] if j != i % 5 else 'Skipped', 'correct': j != i % 5} for j, q in enumerate(questions)]
            db.execute('''INSERT OR IGNORE INTO attempts(run_id,user_id,title,subject,grade,score,total,seconds,submitted,review,demo)
                        VALUES(?,?,?,?,?,?,?,?,?,?,1)''', (run_id, owner, quiz['title'], quiz['subject'], quiz['grade'], sum(q['correct'] for q in review), len(review), 90+i*15, (utcnow()-timedelta(days=11-i)).isoformat(), json.dumps(review)))
        db.commit()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--demo', action='store_true')
    parser.add_argument('--data')
    parser.add_argument('--port', type=int, default=5000)
    parser.add_argument('--init-admin', action='store_true')
    args = parser.parse_args()
    application = create_app(args.data)
    if args.demo:
        add_demo(application)
    if args.init_admin:
        import getpass
        email = input('Administrator email: ').strip().lower()
        password = getpass.getpass('Password (8–128 characters): ')
        if not re.fullmatch(r'[^\s@]+@[^\s@]+\.[^\s@]+', email) or not 8 <= len(password) <= 128:
            raise SystemExit('Invalid email or password length.')
        with application.app_context():
            application.db().execute('INSERT INTO users(name,email,password,role,grade) VALUES(?,?,?,\'admin\',8)',
                                     ('Administrator', email, generate_password_hash(password)))
            application.db().commit()
        print('Administrator created.')
    else:
        application.run(host='127.0.0.1', port=args.port, debug=False)
