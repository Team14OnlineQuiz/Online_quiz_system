# Quizora presentation guide

Prepared for Akshay (`sickDoughnut-007`). Issue ownership and closed status checked on 17 November 2026.

## Opening: about 30 seconds

"Good morning, sir. We are Team 14, and our project is Quizora, an Online Quiz System. It helps students practise subject- and grade-based quizzes and understand their progress. Teachers can manage quizzes, monitor class performance, and review what each student answered alongside the correct answers. We developed the original console system in the first two iterations, then extended it into a website in Iteration 3. I will demonstrate the student and teacher workflows, then explain my contribution and the issues I completed."

## Before the presentation

Start the existing local environment from the repository root:

```powershell
.venv/Scripts/python -m web.app --demo
```

Open <http://127.0.0.1:5000>. If the environment needs setup, follow [web/README.md](../web/README.md). Keep the server terminal running.

| Account | Email | Password |
| --- | --- | --- |
| Demo learner | learner@quizora.test | Learn@12345 |
| Demo administrator | admin@quizora.test | Teach@12345 |

The demo learner includes explicitly marked sample history. A newly registered learner has empty history. Explain that distinction when showing analytics. A quiz submitted during the presentation creates a real saved attempt in the local demo database.

Keep tabs ready for the website, [Team14Project](https://github.com/orgs/Team14OnlineQuiz/projects/1), your [assigned issues](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues?q=is%3Aissue%20assignee%3AsickDoughnut-007), and [GitHub Actions](https://github.com/Team14OnlineQuiz/Online_quiz_system/actions). The original sprint dates are planning history; do not imply the commits were made on those dates.

## What to show: about five minutes

| Time | Show | What to say |
| --- | --- | --- |
| 0:00-0:30 | Login, then learner dashboard | "Students and teachers have separate workspaces. Registration creates a learner account, while the server controls the account's role." |
| 0:30-1:00 | Quiz catalogue; change subject and grade filters | "Students can find quizzes by subject, grade, and search. The starter catalogue covers Mathematics, Science, English, and Computing for grades 6 to 12." |
| 1:00-2:00 | Complete one short quiz | "The runner supports multiple-choice, true/false, and short-answer questions. The server calculates the score and saves the attempt." Leave one answer blank briefly to show validation, then complete it. |
| 2:00-2:30 | Result and answer review | "The review shows the submitted answers, correct answers, and explanations. Answer keys are withheld from the learner's quiz-start response." |
| 2:30-3:00 | Progress page and newest history entry | "Analytics come from saved attempts. This submission appears in history, while subject performance and activity summarize the account's attempts." |
| 3:00-4:00 | Log out; sign in as administrator; open class analytics and that learner's result | "Teachers can see which student attempted which quiz and compare every selected answer with the correct answer." Show the learner's name/email, score, and at least one answer comparison. |
| 4:00-4:30 | Manage quizzes; open the editor | "Only administrators can create, edit, or archive quizzes. The editor supports all three question formats and validates the content." Save a small demo edit only if time permits. |
| 4:30-5:00 | GitHub issues, one linked merged PR, and Actions | "We tracked work as issues with priorities and story points, implemented it through branches and pull requests, and ran automated checks." Show one specific issue and its code change. |

An easy controlled example is Mathematics, grade 6, **Number sense**. Its starter answer key is `0.75`, `True`, `40`. Deliberately select one wrong answer to demonstrate useful correction; two correct out of three displays **67% on the website**. The original console specification displays the same fraction to one decimal place, **66.7%**.

If sir wants sprint evidence, explain Iteration 1's account/security foundation, Iteration 2's reporting/performance/platform work, and Iteration 3's website. The previously recorded sprint videos can supplement the live demo.

## My contribution: about one minute

"My work was divided into three iterations. In Iteration 1, I implemented learner registration, authentication with role-specific access, recovery from invalid quiz answers, and secure password storage. In Iteration 2, I implemented the administrator results summary, measured response time with the required data volume, and set up documented Windows and Linux builds with automated checks. In Iteration 3, I implemented the web login and navigation, learner dashboard and quiz filters, mixed question types, server-side grading, progress analytics, and administrator quiz management. I also worked on integration and corrected the administrator result page so teachers see the student's identity, submitted answers, and correct answers. My GitHub account has 12 closed assigned issues, estimated at 56 story points."

Credit teammates for their original assigned question-management, quiz, scoring, persistence, and history modules. Describe the later integration as work connecting and validating those contributions rather than claiming every original feature as your individual work.

## My completed issues

All issues below are assigned to `sickDoughnut-007` and were closed when checked. Points are planning estimates, not hours or a measure of presentation length.

| Iteration | Issue | What I implemented or resolved | Points |
| --- | --- | --- | ---: |
| 1 | [#1 / FR-01](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/1) | Learner registration, input and duplicate checks, persistence before success | 3 |
| 1 | [#3 / FR-02](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/3) | Authentication, stored-role sessions, generic credential failure | 3 |
| 1 | [#7 / FR-07](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/7) | Invalid console answer repeats the current question without advancing | 1 |
| 1 | [#16 / SEC-01](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/16) | Vetted adaptive password hashing with an independent random salt per account | 5 |
| 2 | [#10 / FR-10](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/10) | Administrator-only completed-attempt summary across learners | 3 |
| 2 | [#12 / NFR-01](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/12) | Indexed question reads and p95 benchmark with 1,000 questions and 10,000 attempts | 5 |
| 2 | [#15 / NFR-04](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/15) | Documented C++17 builds and Windows/Linux automated checks | 5 |
| 3 | [#32 / IT3-01](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/32) | Secure web registration/login/logout, role navigation, CSRF checks | 5 |
| 3 | [#33 / IT3-02](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/33) | Learner dashboard and combined subject/grade/search discovery | 5 |
| 3 | [#34 / IT3-03](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/34) | Three question types, server grading, persisted attempt and result review | 8 |
| 3 | [#35 / IT3-04](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/35) | Personal analytics, subject performance, account-isolated history | 5 |
| 3 | [#36 / IT3-05](https://github.com/Team14OnlineQuiz/Online_quiz_system/issues/36) | Administrator management, class activity, web tests and release validation | 8 |
| **Total** | **12 closed issues** | **Iteration 1: 12 points; Iteration 2: 13; Iteration 3: 31** | **56** |

Useful merged PR evidence: [Iteration 1 #22](https://github.com/Team14OnlineQuiz/Online_quiz_system/pull/22), [Iteration 2 #23](https://github.com/Team14OnlineQuiz/Online_quiz_system/pull/23), [website #37](https://github.com/Team14OnlineQuiz/Online_quiz_system/pull/37), and [administrator answer-review correction #38](https://github.com/Team14OnlineQuiz/Online_quiz_system/pull/38).

## Problems I can explain clearly

| Problem | Resolution | Evidence to show |
| --- | --- | --- |
| Invalid console answers needed safe recovery | Reject them and repeat the same question; record only a valid choice | #7 and its test |
| Password records needed protection | C++ uses libsodium Argon2id with random salts; the web extension uses Werkzeug scrypt | #16, #32, and security tests |
| Results needed different permissions | Learners see their own attempts; administrators can inspect class results | #10, #35, server authorization tests |
| Administrator result review used learner-oriented wording and omitted useful student context | Added a teacher review with identity, submitted answers, correct answers, and a regression test | Merged PR #38 and the live admin result page |
| Repeated submission could create duplicate results | A unique run identifier and transaction return the same completed result | #34 and concurrent-submission test |
| Editing a quiz could change the basis of a result | Grade against the saved run snapshot and preserve completed review data | Snapshot tests in `web/test_app.py` |

## Likely questions from sir

**What is the architecture?** The current website is HTML/CSS/vanilla JavaScript in the browser, a Python Flask API, and SQLite persistence. The server authenticates sessions, checks permissions, validates input, grades answers, and queries analytics. The original application is a separate layered C++17 console program with file repositories.

**How is a score calculated?** Each question has equal weight: correct answers divided by total questions, multiplied by 100. Short answers use exact matching after Unicode normalization, case folding, and whitespace normalization. This is not AI or semantic grading.

**Why does the admin have different pages?** Administrators need class oversight and content management; learners need practice and personal feedback. The server enforces permissions, so hiding a menu alone is not the security control.

**Why not store plaintext passwords?** Password verification uses a salted adaptive hash. Salt makes identical passwords produce independent stored representations; the adaptive cost increases the effort required for offline guessing.

**How did you verify it?** Show passing Windows/Linux GitHub Actions jobs, the automated web authorization/grading/persistence tests, and the console build, requirement tests, executable smoke test, and benchmark. The latest implementation runs checked before this documentation change passed both web and C++ workflows: [web checks](https://github.com/Team14OnlineQuiz/Online_quiz_system/actions/runs/42352665836) and [C++ checks](https://github.com/Team14OnlineQuiz/Online_quiz_system/actions/runs/42352665788).

**Are the analytics real?** New submissions are stored and included in analytics. The demo account additionally contains marked sample attempts. New registrations have no fabricated history.

**Is this deployed online?** The presentation uses a local Flask server. Internet hosting needs production hosting and HTTPS configuration. The console and web application currently use separate data stores; there is no automatic credential/data migration.

**Why do the original documents mention C++ and MCQs?** Deliverable 1 specifies the original console release. Iteration 3 extends it with a browser application and more question formats. The split documents preserve that original record; the current website behavior and implementation are in `web/README.md`.

**What are the limits?** The catalogue is starter content grouped into grade bands, not complete curriculum coverage. Short answers are normalized exact matches. This release does not provide remote proctoring, email verification, password reset, or cross-device synchronization.

## Documentation to open

- [SRS](../Deliverable1/SRS.md): what the original system must do; FR, NFR, and SEC identifiers.
- [Test cases](../Deliverable1/Test_Cases.md): how the requirements are checked, with expected results and traceability.
- [SAD](../Deliverable1/SAD.md): how the original architecture, data, services, and interactions are designed.
- [Current web documentation](../web/README.md): actual website setup, behavior, security, and tests.

A simple closing line: "This demonstrates the complete practice-and-review workflow, from student submission to teacher review, supported by tracked issues, pull requests, and automated checks."
