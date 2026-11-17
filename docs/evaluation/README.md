# Evaluation checklist: evidence index

Prepared 17 November 2026 against the supplied `FOR-EVALUATORS-TheCheckList.pdf` (three pages). The faculty document is the primary checklist; the team's reported viva experience adds atomic wording, before/after presentation, and coverage discussion. "Evidence" means supporting material is present, not that a faculty member has accepted it. Personal explanations for schedule changes are left to the team.

## Part 1: requirements and design

| Evaluator item | Evidence / status |
| --- | --- |
| SRS in standard IEEE format | [User-story SRS](../../Deliverable1/SRS.md), [original baseline](../../Deliverable1/SRS_v1_Baseline.md), and [structured atomic revision](../../Deliverable1/SRS_Atomic_Revision.md). Requested content exists; exact institutional-format acceptance remains a review item. |
| SRS introductory/descriptive sections | Original sections 1-3; revision sections 1-3 |
| Clear measurable FRs/NFRs | Original 11 FRs/four NFRs; atomic child statements preserve IDs; NFR-01 has workload, p95 and two-second limit |
| At least one UML use-case diagram | SRS section 7; original diagram preserved |
| At least two security objectives and requirements | SRS section 6 contains three objectives and four parent security requirements |
| Test plan in standard IEEE format | [Before](../../Deliverable1/Test_Plan_v1_Before.md) and [after](../../Deliverable1/Test_Plan_v2_After.md). Updated plan supplies numbered plan sections; exact institutional-format acceptance remains a review item. |
| Test-plan sections 3, 4 and 5 complete | Feature scope, exclusions, approach, criteria and environment are populated in both versions |
| Section 5.1 dedicated to security | Both plans explicitly contain security validation; updated plan maps checks to evidence |
| Test traceability to SRS | Original TC trace column; updated results; [backlog traceability](backlog-traceability.md) |
| At least 7-10 FR/NFR cases | Twelve original cases plus 15 executed web cases; [actual results](../reports/acceptance-results.json) |
| SAD in standard IEEE format | [Original SAD](../../Deliverable1/SAD.md) and [web addendum](../../Deliverable1/SAD_Web_Addendum.md). Required design content exists; exact institutional-format acceptance remains a review item. |
| Pattern, component view, traceability, security architecture | Original SAD sections 2/3/5/9; current web addendum sections 2/4/7 |
| Two sequence diagrams, API and error handling | Original SAD figures 3/4 and sections 7/8; web addendum adds two current web sequences and HTTP/error tables |

## Part 2: planning and repository

| Evaluator item | Evidence / status |
| --- | --- |
| Project linked to repository | [Team14Project](https://github.com/orgs/Team14OnlineQuiz/projects/1). Existing project evidence was captured earlier; current browser verification is blocked by the saved GitHub browser permission. Open it manually before the demo. |
| SRS-based backlog | All 19 original parents mapped to issue IDs; five web-extension stories documented separately |
| Story points for every item | All 24 issue descriptions contain an estimate; [mapping](backlog-traceability.md) lists each value. Live Project numeric fields require manual verification. |
| Individual assignment | GitHub snapshot records the actual owner of every issue; three implementation accounts are present |
| Two planned sprints | Issue planning text assigns Iteration 1/2; current Project iteration fields require manual verification |
| Actions for PRs/commits | Existing C++ and web workflows trigger on main pushes and PRs; [recorded successful runs](../reports/github-evidence.json) |
| Implementation matches constraints | Original C++ baseline retained; later Flask/SQLite extension and display/storage changes explicitly documented as deviations |
| Assigned members raised PRs | [PR/issue snapshot](../reports/github-evidence.json) records authors and owners |
| Owner formally reviewed and merged PRs | Merges exist. No submitted formal GitHub review records were found on the 12 merged implementation PRs. This remains an evidence gap. |
| One-minute Sprint 1 working-product video uploaded | [Sprint 1 video](../videos/sprint-1.mp4), exactly 60 seconds; historical source `ad5cf8b` |
| Sprint 2 completed and development frozen | Integrated console checkpoint `e08a033`; later web work continued. No claim of an October freeze. See deviations. |
| Deviations documented | [Schedule/scope/evidence deviations](deviations.md); personal reasons excluded from the repository |
| Documentation finalized | Prepared evidence packet, separate baseline documents, updated test plan, architecture addendum and guide; team/faculty review remains |
| Two-minute Sprint 2 working-product video uploaded | [Sprint 2 video](../videos/sprint-2.mp4), exactly 120 seconds; historical source `e08a033` |

## Part 3: tests and reports

| Evaluator item | Evidence / status |
| --- | --- |
| Original tests updated for sprint changes | Baseline version 1.0 retained; version 1.1 adds integration/web scope and observed outcomes |
| Tests executed according to plan | 51 C++ tests, real CLI smoke, original three-question acceptance fixture, benchmark, and 15 web tests. TC-12 explicitly combines local Windows execution with prior Windows/Linux CI. |
| Generated reports in docs | [Report index](../reports/README.md): line/branch coverage HTML/XML/JSON, JUnit XML, functional logs, acceptance observations, environment and benchmark CSV |

## Part 4: final demo and viva

| Evaluator item | Evidence / preparation |
| --- | --- |
| Application covers core use cases | Demonstrate learner registration/login, catalogue, quiz, review/progress, administrator authoring and learner answer review |
| Explain permitted deviations | Explain separate console and web implementations; original MCQ versus mixed formats; local file versus SQLite storage; actual dates. Faculty permission is not presumed. |
| Readable accessible version-controlled code | Root/web READMEs, named source modules, branches/merged PRs and main history |
| Individual contributions and assigned PRs | [Presentation guide](../presentation-guide.md) and [full walkthrough](walkthrough.md); fourth member presents design/testing concepts without an invented coding contribution |
| Agile/SDLC: estimates, planning, blocks | Explain requirement -> issue -> points/owner/iteration -> implementation -> checks -> PR -> integration -> verification. Actual date deviations stay visible. |
| Justify architecture, API and tests | Walkthrough gives precise answers and pointers; current architecture is documented separately from the console SAD |

## Before entering the room

Open the Project manually and verify its link, estimates, assignments, Iteration 1/2 and status values. Keep the before/after plans, SAD, report index, videos, PRs and live website ready. Rehearse the [40-45 minute walkthrough](walkthrough.md). The team supplies the oral explanation for late dates. The fourth presenter should prepare the architecture and testing section.
