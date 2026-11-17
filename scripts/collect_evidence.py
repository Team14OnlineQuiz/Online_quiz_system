"""Generate real test and coverage evidence without replacing the normal application build.

Install web/requirements.txt and scripts/requirements-evidence.txt first.
Windows: set SODIUM_ROOT to the official libsodium development directory.
Linux: install g++, gcov, pkg-config and libsodium-dev.
"""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--skip-web', action='store_true', help='Reuse web reports from a separately completed run.')
    options = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    os.chdir(root)
    reports = root / 'docs/reports'
    reports.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    build = root / 'build' / f'coverage-{stamp}'
    build.mkdir(parents=True)
    commands = []

    def run(args, logfile, env=None):
        print(f'Running: {args[0]} ... -> {logfile}', flush=True)
        result = subprocess.run([str(a) for a in args], text=True, encoding='utf-8', errors='replace',
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
        (reports / logfile).write_text(result.stdout, encoding='utf-8')
        commands.append({'argv': [str(a) for a in args], 'log': logfile, 'exit_code': result.returncode})
        if result.returncode:
            print(result.stdout[-5000:])
            raise RuntimeError(f'{logfile} failed with exit code {result.returncode}')
        return result.stdout

    if not options.skip_web:
        run([sys.executable, '-m', 'pytest', 'web', '-q', '--cov=web.app', '--cov=web.seed', '--cov-branch',
             '--cov-report=term-missing', '--cov-report=html:docs/reports/web',
             '--cov-report=json:docs/reports/web-coverage.json', '--cov-report=xml:docs/reports/web-coverage.xml',
             '--junitxml=docs/reports/web-junit.xml'], 'web-tests.txt')
        # The evaluator's checked-in HTML evidence must remain discoverable by Git.
        (reports / 'web/.gitignore').unlink(missing_ok=True)
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=c++17', '-O0', '-g', '--coverage', '-fprofile-abs-path', '-Wall', '-Wextra', '-Werror',
             '-pedantic', f'-I{root / "include"}']
    if os.name == 'nt' and os.environ.get('SODIUM_ROOT'):
        sodium = Path(os.environ['SODIUM_ROOT'])
        flags += ['-DSODIUM_STATIC', f'-I{sodium / "include"}']
        libs = [str(sodium / 'lib/libsodium.a'), '-lws2_32', '-ladvapi32']
    elif os.name == 'nt':
        libs = ['-lsodium']
    else:
        libs = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'libsodium'], text=True))
    sources = sorted(p for p in (root / 'src').glob('*.cpp') if p.name != 'main.cpp')
    suffix = '.exe' if os.name == 'nt' else ''
    tests = build / f'quiz_tests{suffix}'
    application = build / f'quiz{suffix}'
    run([compiler, *flags, *sources, *sorted((root / 'tests').glob('*.cpp')), *libs, '-o', tests], 'cpp-test-build.txt')
    run([compiler, *flags, *sources, root / 'src/main.cpp', *libs, '-o', application], 'cpp-app-build.txt')
    run([tests], 'cpp-tests.txt')
    smoke_env = dict(os.environ, QUIZ_SMOKE_EXE=str(application))
    run([sys.executable, 'scripts/smoke_test.py'], 'cpp-smoke.txt', env=smoke_env)
    run([sys.executable, '-m', 'gcovr', '--root', '.', '--gcov-executable', os.environ.get('GCOV', 'gcov'),
         '--filter', 'src/', '--filter', 'include/quiz/', '--exclude-unreachable-branches',
         '--exclude-throw-branches', '--html', str(reports / 'cpp-coverage.html'), '--html-self-contained',
         '--json-summary', str(reports / 'cpp-coverage.json'), '--json-summary-pretty',
         '--xml', str(reports / 'cpp-coverage.xml'), '--print-summary', str(build)], 'cpp-coverage.txt')
    metadata = {
        'recorded_utc': datetime.now(timezone.utc).isoformat(), 'source_base_commit': subprocess.check_output(
            ['git', 'rev-parse', 'HEAD'], text=True).strip(), 'working_tree_note':
            'Evidence preparation adds documents, reporting scripts, and an optional smoke executable path; production application source is unchanged.',
        'platform': platform.platform(), 'python': sys.version, 'compiler': subprocess.check_output(
            [compiler, '--version'], text=True).splitlines()[0], 'gcov': subprocess.check_output(
            [os.environ.get('GCOV', 'gcov'), '--version'], text=True).splitlines()[0],
        'cpp_scope': 'Instrumented src/*.cpp and include/quiz headers executed by the unit suite and real CLI smoke. Tests, tools, standard library and libsodium excluded. Compiler-generated exception and unreachable branches excluded.',
        'web_scope': 'web/app.py and web/seed.py, exercised by the Flask test client. Excludes test code and browser JavaScript/CSS. Not browser end-to-end coverage.',
        'web_reports_reused': options.skip_web, 'commands': commands,
    }
    (reports / 'environment.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
    print('Evidence generated in docs/reports', flush=True)


if __name__ == '__main__':
    main()
