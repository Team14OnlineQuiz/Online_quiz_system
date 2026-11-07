'use strict';
const app = document.querySelector('#app');
const paths = {
  book: '<path d="M4 4h6a3 3 0 0 1 3 3v14a4 4 0 0 0-4-3H4zM13 7a3 3 0 0 1 3-3h5v14h-4a4 4 0 0 0-4 3"/>',
  home: '<path d="m3 10 9-7 9 7v10H3zM9 20v-7h6v7"/>',
  grid: '<rect x="3" y="3" width="7" height="7" rx="2"/><rect x="14" y="3" width="7" height="7" rx="2"/><rect x="3" y="14" width="7" height="7" rx="2"/><rect x="14" y="14" width="7" height="7" rx="2"/>',
  chart: '<path d="M4 3v17h17M8 15v-4m5 4V7m5 8V4"/>',
  clock: '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',
  check: '<path d="m5 12 4 4L19 6"/>',
  arrow: '<path d="M4 12h15m-5-5 5 5-5 5"/>',
  back: '<path d="M20 12H5m5-5-5 5 5 5"/>',
  search: '<circle cx="10.5" cy="10.5" r="6.5"/><path d="m16 16 5 5"/>',
  logout: '<path d="M9 4H4v16h5m6-13 5 5-5 5m-7-5h12"/>',
  calendar: '<rect x="3" y="5" width="18" height="16" rx="3"/><path d="M7 3v4m10-4v4M3 11h18"/>',
  flame: '<path d="M13 2c2 7-5 7-3 12 2-1 4-3 5-5 7 7 4 13-3 13-8 0-11-10-5-15 0 3 1 4 2 5-2-5 3-6 4-10z"/>',
  target: '<circle cx="12" cy="12" r="9"/><circle cx="12" cy="12" r="5"/><circle cx="12" cy="12" r="1"/>',
  math: '<rect x="4" y="3" width="16" height="18" rx="3"/><path d="M8 8h8M8 13h1m6 0h1m-8 4h1m6 0h1"/>',
  science: '<path d="M9 3h6m-5 0v7l-6 9c-1 2 1 2 2 2h12c1 0 3 0 2-2l-6-9V3M7 15h10"/><circle cx="11" cy="17" r=".5"/>',
  english: '<path d="M4 4h15v17H4zM8 8h7m-7 4h7m-7 4h4"/>',
  computing: '<rect x="3" y="4" width="18" height="13" rx="2"/><path d="M8 21h8m-4-4v4m-3-9-2-2 2-2m6 4 2-2-2-2"/>',
  trophy: '<path d="M8 3h8v6c0 6-8 6-8 0zM8 5H4v3c0 3 4 4 4 4m8-7h4v3c0 3-4 4-4 4m-4 2v5m-4 2h8"/>',
  users: '<circle cx="9" cy="7" r="3"/><path d="M3 21v-4c0-5 12-5 12 0v4m2-17c5 0 5 7 0 7m2 3c3 0 3 4 3 7"/>',
  plus: '<path d="M12 4v16M4 12h16"/>',
  eye: '<path d="M2 12c5-9 15-9 20 0-5 9-15 9-20 0z"/><circle cx="12" cy="12" r="3"/>',
  menu: '<path d="M4 6h16M4 12h16M4 18h16"/>',
  close: '<path d="m6 6 12 12M6 18 18 6"/>',
  spark: '<path d="m12 2 2.5 7.5L22 12l-7.5 2.5L12 22l-2.5-7.5L2 12l7.5-2.5z"/>'
};
const icon = name => `<svg class="icon" aria-hidden="true" viewBox="0 0 24 24">${paths[name] || paths.book}</svg>`;
const escapeHTML = value => String(value ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const E = escapeHTML;
const brand = () => `<a class="brand" href="/dashboard"><span class="brand-mark">${icon('book')}</span>quizora<span class="brand-dot">.</span></a>`;
const subjectKey = subject => ({Mathematics:'math',Science:'science',English:'english',Computing:'computing'}[subject] || 'book');
const subjectClass = subject => subject === 'Mathematics' ? '' : subject.toLowerCase();
const subjectIcon = subject => `<span class="subject-icon ${subjectClass(subject)}">${icon(subjectKey(subject))}</span>`;
const typeName = type => ({mcq:'Multiple choice',true_false:'True or false',short_answer:'Short answer'}[type]);
const dateText = date => new Date(date).toLocaleDateString('en-US',{month:'short',day:'numeric'});
let user = null;
let csrfToken = '';
let quizzes = [];
let progress = null;
let run = null;
let questionIndex = 0;
let answers = [];
let editor = null;
let filters = {subject:'All subjects',grade:'',search:''};
let renderNumber = 0;

function confirmAction(message) {
  return new Promise(resolve => {
    const dialog = document.createElement('dialog');
    dialog.className = 'confirm-dialog';
    dialog.innerHTML = `<h2>Please confirm</h2><p>${E(message)}</p><div class="confirm-actions"><button class="btn secondary" data-choice="cancel">Cancel</button><button class="btn primary" data-choice="confirm">Continue</button></div>`;
    document.body.append(dialog);
    const finish = value => { dialog.close(); dialog.remove(); resolve(value); };
    dialog.addEventListener('click', event => {
      const choice = event.target.closest('[data-choice]')?.dataset.choice;
      if (choice) finish(choice === 'confirm');
    });
    dialog.addEventListener('cancel', event => { event.preventDefault(); finish(false); });
    dialog.showModal();
  });
}

async function api(path, options={}) {
  const response = await fetch('/api/' + path, {
    ...options,
    headers: {'Content-Type':'application/json','X-CSRF-Token':csrfToken,...options.headers},
    body: options.body === undefined ? undefined : JSON.stringify(options.body)
  });
  const data = await response.json();
  if (!response.ok) {
    if (response.status === 401 && user) {
      user = null;
      history.replaceState({},'', '/login');
      authPage();
    }
    throw new Error(data.error || 'Something went wrong. Please try again.');
  }
  return data;
}

function toast(message) {
  const box = document.querySelector('#toast');
  box.textContent = message;
  box.style.display = 'block';
  clearTimeout(toast.timer);
  toast.timer = setTimeout(() => box.style.display='none', 4200);
}

async function go(path) {
  if (run && !path.startsWith('/quiz/')) {
    if (!await confirmAction('Leave this quiz? Your unfinished answers will be discarded.')) return;
    run = null;
  }
  history.pushState({},'',path);
  window.scrollTo(0,0);
  render().catch(showPageError);
}

function authPage() {
  const signup = location.pathname === '/register';
  document.title = `${signup?'Create account':'Welcome back'} · Quizora`;
  app.innerHTML = `<main class="auth"><section class="auth-left">${brand()}
    <div class="auth-copy"><span class="eyebrow">A little practice. A lot of possibility.</span><h1>Your next<br>“I get it” moment<br>starts here<span class="brand-dot">.</span></h1><p>Explore your curiosity. Build your confidence. Make progress, one question at a time.</p></div>
    <div class="auth-art" aria-hidden="true"><div class="art-orbit"></div><div class="art-card art-quiz"><span class="pill">YOUR LEARNING JOURNEY</span><h3 style="margin-top:17px">Small steps. Big ideas.</h3><div class="bar"></div><div class="bar"></div><div class="bar"></div></div><div class="art-card art-score">${icon('target')}<div><strong>You’ve got this!</strong><div class="muted">Keep your curiosity growing</div></div></div><div class="art-card art-streak"><strong>✦ A little better, every day</strong>Learn at your own pace.</div><div class="art-symbol">${icon('spark')}</div></div>
    <div class="auth-note">Made for curious minds · Grades 6–12 · Team 14</div></section>
    <section class="auth-right"><div class="auth-form"><div class="auth-mobile-brand">${brand()}</div><span class="eyebrow">YOUR SPACE TO GROW</span><h1>${signup?'Let’s get you started.':'Welcome back.'}</h1><p>${signup?'A new chapter of learning starts with you.':'Pick up where your curiosity left off.'}</p>
    <form id="auth-form"><div id="form-error" class="error hidden" role="alert"></div>
    ${signup?'<label class="field">Your name<input name="name" autocomplete="name" placeholder="Alex Morgan" required maxlength="80"></label>':''}
    <label class="field">Email address<input name="email" type="email" autocomplete="email" placeholder="you@example.com" required maxlength="160"></label>
    <label class="field">Password<div class="input-wrap"><input id="password" name="password" type="password" autocomplete="${signup?'new-password':'current-password'}" placeholder="${signup?'At least 8 characters':'Enter your password'}" required minlength="8" maxlength="128"><button type="button" data-action="password" aria-label="Show password">${icon('eye')}</button></div></label>
    ${signup?`<label class="field">Your grade<select name="grade">${Array.from({length:7},(_,i)=>`<option value="${i+6}" ${i===2?'selected':''}>Grade ${i+6}</option>`).join('')}</select></label>`:''}
    <button class="btn primary wide" type="submit">${signup?'Create your account':'Sign in'} ${icon('arrow')}</button></form>
    <div class="auth-footer">${signup?'Already part of Quizora? <a href="/login">Sign in</a>':'New around here? <a href="/register">Create an account</a>'}</div>
    <div class="demo-box"><strong>Take a look around</strong><br>Local demo accounts, available when the server runs in demo mode.<br><button data-action="demo" data-role="learner">Explore as a learner ${icon('arrow')}</button><button data-action="demo" data-role="admin">Administrator</button><br><span>Demo passwords: Learn@12345 / Teach@12345</span></div>
    </div></section></main>`;
}

function shell(title, content) {
  const admin = user.role === 'admin';
  const current = location.pathname;
  const nav = admin ? [['/dashboard','home','Overview'],['/quizzes','grid','Manage quizzes'],['/progress','chart','Class analytics']] : [['/dashboard','home','Overview'],['/quizzes','grid','Explore quizzes'],['/progress','chart','My progress']];
  const initials = user.name.split(' ').map(n=>n[0]).slice(0,2).join('').toUpperCase();
  document.title = `${title} · Quizora`;
  app.innerHTML = `<div class="shell"><aside class="sidebar">${brand()}<div class="nav-label">${admin?'TEACHING SPACE':'YOUR LEARNING SPACE'}</div><nav class="nav" aria-label="Main navigation">${nav.map(([url,i,label])=>`<a href="${url}" class="${current===url?'active':''}" ${current===url?'aria-current="page"':''}>${icon(i)}${label}${url==='/quizzes'?`<span class="nav-count">${quizzes.length}</span>`:''}</a>`).join('')}</nav><div class="sidebar-bottom"><div class="study-note"><div class="spark">✦</div><b>${admin?'Great learning starts with you.':'Progress over perfection.'}</b>${admin?'Create a little challenge. Inspire a big discovery.':'Every question is a chance to learn something new.'}</div><div class="profile"><span class="avatar">${E(initials)}</span><div><div class="profile-name">${E(user.name)}</div><div class="profile-role">${admin?'Administrator':`Grade ${user.grade} learner`}</div></div><button data-action="logout" aria-label="Sign out">${icon('logout')}</button></div></div></aside>
  <div class="main"><header class="topbar"><button class="icon-button mobile-toggle" data-action="menu" aria-label="Toggle navigation">${icon('menu')}</button><div class="breadcrumbs">My workspace <span>/</span> <b>${E(title)}</b></div><div class="topbar-right"><form id="top-search" class="top-search">${icon('search')}<input name="search" placeholder="Find your next quiz…" aria-label="Search all quizzes"></form><span class="pill">${admin?'TEACHER SPACE':`GRADE ${user.grade}`}</span><span class="avatar" aria-hidden="true">${E(initials)}</span></div></header>
  <main class="content" id="main-content">${user.demo?'<div class="demo-notice">Demo workspace · Sample history is included for exploration. New quiz attempts are saved to this account.</div>':''}${content}<footer class="page-footer"><span>© ${new Date().getFullYear()} Quizora · Team 14 Online Quiz System</span><span>A little practice. A lot of progress.</span></footer></main></div></div>`;
}

function heading(title, subtitle, action='') {
  return `<div class="page-heading"><div><h1>${title}</h1><p>${subtitle}</p></div>${action || `<span class="date-label">${icon('calendar')}${new Date().toLocaleDateString('en-US',{weekday:'short',month:'long',day:'numeric'})}</span>`}</div>`;
}

function statsCards() {
  const admin = user.role==='admin';
  const stats = [[admin?'Class attempts':'Quizzes completed',progress.count,'book','Completed & saved'],['Overall accuracy',progress.accuracy+'%','target','Across all answered questions'],[admin?'Registered learners':'Learning streak',admin?progress.learners:progress.streak,'flame',admin?'Learner accounts':'Consecutive active days'],['Time spent',progress.minutes,'clock','Minutes of quiz practice']];
  return `<div class="stats">${stats.map(([label,value,i,foot])=>`<div class="stat"><div class="stat-top">${label}<span class="stat-icon">${icon(i)}</span></div><div class="stat-value">${value}${label==='Learning streak'?'<small>days</small>':label==='Time spent'?'<small>min</small>':''}</div><div class="stat-foot">${foot}</div></div>`).join('')}</div>`;
}

function recentAttempts(limit=3) {
  if (!progress.attempts.length) return emptyState('Your first quiz is waiting','Complete a quiz to start building your learning story.',true);
  return `<div class="attempt-list">${progress.attempts.slice(0,limit).map(a=>`<a class="attempt-row" href="/results/${a.id}">${subjectIcon(a.subject)}<div><h3>${E(a.title)}</h3><p>${user.role==='admin'?E(a.name)+' · ':''}${E(a.subject)} · Grade ${a.grade} · ${dateText(a.submitted)}</p></div><div class="score">${Math.round(100*a.score/a.total)}%<small>${a.score} of ${a.total} correct</small></div></a>`).join('')}</div>`;
}

function activityBars(large=false) {
  const days = progress.activity.slice(large?0:7);
  const maximum = Math.max(1,...days.map(a=>a.attempts));
  if (!large) return `<div class="mini-activity">${days.map(a=>`<div title="${dateText(a.date)}: ${a.attempts} attempts"><i style="height:${Math.max(4,a.attempts/maximum*85)}%"></i><small>${new Date(a.date+'T12:00:00').toLocaleDateString('en-US',{weekday:'narrow'})}</small></div>`).join('')}</div>`;
  return `<div class="chart" role="img" aria-label="Completed quizzes by day over the last fourteen days"><div class="chart-grid">${[maximum,Math.round(maximum/2),0].map(n=>`<div><small>${n}</small></div>`).join('')}</div><div class="chart-bars">${days.map((a,i)=>`<div class="chart-bar" title="${dateText(a.date)}: ${a.attempts} completed quizzes${a.accuracy===null?'':`, ${a.accuracy}% accuracy`}"><i style="height:${Math.max(1,a.attempts/maximum*100)}%"></i><small>${i%2===0?dateText(a.date):''}</small></div>`).join('')}</div></div>`;
}

function emptyState(title, description, action=false) {
  return `<div class="empty">${icon('book')}<h3>${E(title)}</h3><p>${E(description)}</p>${action?`<a class="btn subtle" href="/quizzes">Explore quizzes ${icon('arrow')}</a>`:''}</div>`;
}

function dashboard() {
  const admin = user.role==='admin';
  const firstName = E(user.name.split(' ')[0]);
  const intro = admin ? heading(`Welcome back, ${firstName} <span style="font-size:22px">✦</span>`,'A little inspiration for your classroom, all in one place.',`<a class="btn primary" href="/admin/new">${icon('plus')} Create a quiz</a>`) : heading(`Hello, ${firstName} <span style="font-size:22px">☀</span>`,'Ready to turn a little curiosity into a little progress?');
  const hero = `<section class="welcome"><div><div class="eyebrow">${admin?'MAKE ROOM FOR DISCOVERY':'YOUR NEXT CHAPTER STARTS HERE'}</div><h2>${admin?'Good questions spark<br>great discoveries.':'Big ideas start with<br>one small question.'}</h2><p>${admin?'Build thoughtful quizzes for every subject and grade. See where your learners shine.':'Find your next challenge, explore something new, and build confidence along the way.'}</p><a class="btn primary" href="${admin?'/admin/new':'/quizzes'}">${admin?'Create a quiz':'Find a quiz'} ${icon('arrow')}</a></div><div class="welcome-art" aria-hidden="true"><div class="orb"></div><span class="welcome-star">✦</span><div class="mini-card">${icon('trophy')}<strong>You’ve got this!</strong><span>Keep going. Keep growing.</span></div><div class="welcome-dot"></div></div></section>`;
  const subjects = `<div class="section-head"><h2>${admin?'Your subject library':'What will you explore today?'}</h2><a class="text-btn" href="/quizzes">View all quizzes ${icon('arrow')}</a></div><div class="subjects">${['Mathematics','Science','English','Computing'].map(s=>`<button class="subject-card" data-action="subject" data-subject="${s}">${subjectIcon(s)}<h3>${s}</h3><p>${quizzes.filter(q=>q.subject===s).length} quizzes to explore</p><span class="arrow">${icon('arrow')}</span></button>`).join('')}</div>`;
  const lower = `<div class="dashboard-lower"><section class="panel"><div class="section-head"><h2>${admin?'Latest class activity':'Your recent activity'}</h2><a class="text-btn" href="/progress">View all ${icon('arrow')}</a></div>${recentAttempts()}</section><section class="panel"><div class="section-head"><h2>This week’s rhythm</h2><span class="pill">LAST 7 DAYS</span></div>${activityBars()}<div class="weekly-summary">${icon('spark')} ${progress.activity.slice(7).reduce((s,a)=>s+a.attempts,0)} quizzes completed this week</div></section></div>`;
  shell('Overview',intro+hero+statsCards()+subjects+lower);
}

function quizCard(q) {
  const admin = user.role==='admin';
  return `<article class="quiz-card"><div class="quiz-cover ${subjectClass(q.subject)}"><span class="pill">${E(q.subject)}</span>${icon(subjectKey(q.subject))}</div><div class="quiz-card-body"><h3>${E(q.title)}</h3><p>${E(q.description)}</p><div class="quiz-meta"><span>${icon('book')}${q.count} questions</span><span>${icon('clock')}${q.minutes} min</span><span>Grade ${q.grade}</span></div><div class="card-bottom"><small>${q.types.length} question format${q.types.length===1?'':'s'}</small>${admin?`<div class="admin-actions"><a class="btn subtle" href="/admin/edit/${q.id}">Edit quiz</a><button data-action="archive" data-id="${q.id}" aria-label="Archive ${E(q.title)} grade ${q.grade}">Archive</button></div>`:`<button class="btn subtle" data-action="start" data-id="${q.id}">Start quiz ${icon('arrow')}</button>`}</div></div></article>`;
}

function catalogue() {
  const admin = user.role==='admin';
  const results = quizzes.filter(q=>(filters.subject==='All subjects'||q.subject===filters.subject)&&(!filters.grade||q.grade===Number(filters.grade))&&(`${q.title} ${q.subject} ${q.description}`.toLowerCase().includes(filters.search.toLowerCase())));
  shell(admin?'Manage quizzes':'Explore quizzes',heading(admin?'Your quiz library':'Follow your curiosity.',admin?'Create, refine, and organise your learning experiences.':'A new skill, a fresh perspective, a little more confidence.',admin?`<a class="btn primary" href="/admin/new">${icon('plus')} Create a quiz</a>`:'')+
    `<section class="catalog-hero"><div><span class="eyebrow">SOMETHING FOR EVERY CURIOUS MIND</span><h2>Find your next “aha!” moment.</h2><p>Four subjects. Seven grades. Endless possibilities to grow.</p></div><div class="catalog-emblem">✦</div></section>
    <div class="filters"><label class="search-filter">${icon('search')}<input id="quiz-search" aria-label="Search quiz library" placeholder="Search a topic or quiz…" value="${E(filters.search)}"></label><select id="grade-filter" aria-label="Filter by grade"><option value="">All grades</option>${Array.from({length:7},(_,i)=>`<option value="${i+6}" ${filters.grade==i+6?'selected':''}>Grade ${i+6}</option>`).join('')}</select><button class="text-btn" data-action="reset-filters">Reset filters</button></div>
    <div class="subject-tabs" role="group" aria-label="Filter by subject">${['All subjects','Mathematics','Science','English','Computing'].map(s=>`<button class="${filters.subject===s?'active':''}" aria-pressed="${filters.subject===s}" data-action="filter-subject" data-subject="${s}">${s}</button>`).join('')}</div>
    <div class="section-head"><h2>${filters.subject==='All subjects'?'Explore the collection':E(filters.subject)}</h2><span class="muted" id="results-count">${results.length} quiz${results.length===1?'':'zes'}</span></div><div class="quiz-grid" id="quiz-grid">${results.map(quizCard).join('')||emptyState('No quizzes found','Try another subject, grade, or search term.')}</div>`);
}

function filterResults() {
  const selected = quizzes.filter(q=>(filters.subject==='All subjects'||q.subject===filters.subject)&&(!filters.grade||q.grade===Number(filters.grade))&&(`${q.title} ${q.subject} ${q.description}`.toLowerCase().includes(filters.search.toLowerCase())));
  document.querySelector('#quiz-grid').innerHTML = selected.map(quizCard).join('')||emptyState('No quizzes found','Try another subject, grade, or search term.');
  document.querySelector('#results-count').textContent = selected.length+(selected.length===1?' quiz':' quizzes');
}

function progressPage() {
  const admin = user.role==='admin';
  const subjects = `<section class="panel"><h2>Performance by subject</h2><p class="muted">Accuracy across completed questions</p><div class="subject-performance">${progress.subjects.map(s=>`<div class="performance-row"><div><span>${E(s.subject)}</span><b>${s.attempts?s.accuracy+'%':'Not started'}</b></div><div class="track"><i style="width:${s.accuracy}%"></i></div></div>`).join('')}</div></section>`;
  const history = progress.attempts.length ? `<div class="table-wrap"><table class="history-table"><thead><tr><th>QUIZ</th>${admin?'<th>LEARNER</th>':''}<th>SUBJECT</th><th>GRADE</th><th>SCORE</th><th>DATE</th><th></th></tr></thead><tbody>${progress.attempts.map(a=>`<tr><td>${E(a.title)}</td>${admin?`<td>${E(a.name)}</td>`:''}<td>${E(a.subject)}</td><td>Grade ${a.grade}</td><td><span class="pill ${a.score/a.total>=.7?'green':'orange'}">${Math.round(100*a.score/a.total)}%</span></td><td>${dateText(a.submitted)}</td><td><a class="text-btn" href="/results/${a.id}">Review ${icon('arrow')}</a></td></tr>`).join('')}</tbody></table></div>`:emptyState('A fresh start','Your completed quizzes and insights will appear here.',true);
  shell(admin?'Class analytics':'My progress',heading(admin?'Every learner’s journey.':'Look how far you’ve come.','Small steps add up. Here’s your learning story so far.')+statsCards()+
    `<div class="progress-grid"><section class="panel"><div class="section-head"><div><h2>Learning activity</h2><p>Completed quizzes over the last 14 days · UTC</p></div><span class="pill">14 DAYS</span></div>${activityBars(true)}</section>${subjects}</div><section class="panel"><div class="section-head"><div><h2>${admin?'Class attempt history':'Your completed quizzes'}</h2><p>Revisit your answers and discover what to practise next.</p></div><span class="pill">${progress.count} COMPLETED</span></div>${history}</section>`);
}

function quizRunner() {
  const q = run.questions[questionIndex];
  const options = q.type === 'short_answer' ? `<label class="field">Your answer<input id="short-answer" placeholder="Type your answer here…" value="${E(answers[questionIndex]||'')}" maxlength="300" autocomplete="off"></label><p class="muted">Answers are checked without case or extra-space differences.</p>` : `<div class="answer-options">${q.options.map((answer,i)=>`<button class="answer-option ${answers[questionIndex]===answer?'selected':''}" data-action="answer" data-index="${i}" aria-pressed="${answers[questionIndex]===answer}"><span class="letter">${String.fromCharCode(65+i)}</span>${E(answer)}${answers[questionIndex]===answer?icon('check'):''}</button>`).join('')}</div>`;
  shell('Quiz in progress',`<div class="runner"><div class="runner-top"><div><span class="eyebrow">${E(run.subject)} · GRADE ${run.grade}</span><h2>${E(run.title)}</h2></div><button class="text-btn" data-action="exit-quiz">${icon('close')} Exit quiz</button></div><div class="runner-progress"><div class="track"><i style="width:${questionIndex/run.questions.length*100}%"></i></div><span>${questionIndex+1} of ${run.questions.length}</span></div><section class="question-panel"><div class="question-types"><span class="pill">${typeName(q.type)}</span><span>QUESTION ${String(questionIndex+1).padStart(2,'0')}</span></div><h1>${E(q.prompt)}</h1>${options}<div id="question-error" class="error hidden" role="alert"></div><div class="runner-controls"><button class="btn secondary" data-action="previous" ${questionIndex===0?'disabled':''}>${icon('back')} Previous</button><div class="question-dots">${run.questions.map((_,i)=>`<button class="question-dot ${i===questionIndex?'current':answers[i]?'answered':''}" data-action="jump" data-index="${i}" aria-label="Go to question ${i+1}">${i+1}</button>`).join('')}</div><button class="btn primary" data-action="next">${questionIndex===run.questions.length-1?'Finish quiz':'Next question'} ${icon('arrow')}</button></div></section><p class="runner-note">Take your time. Every question is a chance to learn.</p></div>`);
}

function resultsPage(result) {
  const percent = Math.round(100*result.score/result.total);
  if (user.role === 'admin') {
    const learner = result.learner;
    const submitted = new Date(result.submitted).toLocaleString('en-US', {
      dateStyle:'medium', timeStyle:'short', timeZone:'UTC'
    });
    const review = result.review.map((q, i) => `
      <article class="review-card">
        <span class="pill ${q.correct?'green':'orange'}">${icon(q.correct?'check':'close')}${q.correct?'Correct':'Incorrect'} · Question ${i+1}</span>
        <h3>${E(q.prompt)}</h3>
        <div class="answer-comparison">
          <div class="review-answer"><span>Student’s answer</span><strong>${E(q.selected)}</strong></div>
          <div class="review-answer correct-answer"><span>Correct answer</span><strong>${E(q.answer)}</strong></div>
        </div>
        <p>${E(q.explanation)}</p>
      </article>`).join('');
    shell('Student attempt review', `<div class="runner">
      ${heading('Student attempt review','See who attempted the quiz and how each answer was graded.',
        `<a class="text-btn" href="/progress">${icon('back')} Back to class analytics</a>`)}
      <section class="panel admin-attempt-summary">
        <div class="section-head"><div><span class="eyebrow">COMPLETED ATTEMPT #${result.id}</span><h2>${E(learner.name)}</h2><p>${E(learner.email)} · Learner grade ${learner.grade}</p></div><span class="pill ${percent>=70?'green':'orange'}">${percent}% · ${result.score}/${result.total} correct</span></div>
        <dl class="attempt-details">
          <div><dt>Quiz attempted</dt><dd>${E(result.title)}</dd></div>
          <div><dt>Subject & quiz grade</dt><dd>${E(result.subject)} · Grade ${result.grade}</dd></div>
          <div><dt>Submitted</dt><dd>${E(submitted)} UTC</dd></div>
          <div><dt>Time spent</dt><dd>${Math.max(1,Math.ceil(result.seconds/60))} min</dd></div>
        </dl>
      </section>
      <div class="section-head"><h2>Question-by-question review</h2><span class="muted">${result.total} answers</span></div>
      ${review}
    </div>`);
    return;
  }
  shell('Quiz results',`<div class="runner"><section class="result-summary"><span class="eyebrow">ANOTHER STEP FORWARD</span><div class="score-ring" style="--score:${percent}%"><strong>${percent}%</strong></div><h1>${percent>=70?'Look at you go!':'Keep growing. You’re getting there.'}</h1><p>${E(result.title)} · ${E(result.subject)} · Grade ${result.grade}<br>${result.score} out of ${result.total} correct · ${Math.max(1,Math.ceil(result.seconds/60))} min · ${dateText(result.submitted)}</p><div class="result-actions"><a class="btn primary" href="/quizzes">Explore more quizzes ${icon('arrow')}</a><a class="btn secondary" href="/progress">${icon('chart')} View progress</a></div></section><div class="section-head"><h2>A little reflection goes a long way.</h2><span class="muted">Your answer review</span></div>${result.review.map((q,i)=>`<article class="review-card"><span class="pill ${q.correct?'green':'orange'}">${icon(q.correct?'check':'spark')}${q.correct?'Correct':'Keep practising'} · Question ${i+1}</span><h3>${E(q.prompt)}</h3><div class="review-answer">Your answer <strong>${E(q.selected)}</strong>${q.correct?'':`<br>Correct answer <strong>${E(q.answer)}</strong>`}</div><p>${E(q.explanation)}</p></article>`).join('')}</div>`);
}

const newQuestion = () => ({type:'mcq',prompt:'',options:['','','',''],answer:'',explanation:''});
function quizEditor() {
  const q = editor;
  shell(q.id?'Edit quiz':'Create quiz',`<div class="editor">${heading(q.id?'A little refinement.':'Create a little challenge.','Bring a subject to life with questions that spark curiosity.',`<a class="text-btn" href="/quizzes">${icon('back')} Back to library</a>`)}<form id="editor-form"><div id="form-error" class="error hidden" role="alert"></div><section class="panel"><div class="form-grid"><label class="field">Quiz title<input name="title" value="${E(q.title)}" maxlength="120" required placeholder="e.g. The wonder of fractions"></label><label class="field">Subject<select name="subject">${['Mathematics','Science','English','Computing'].map(s=>`<option ${s===q.subject?'selected':''}>${s}</option>`).join('')}</select></label><label class="field">Grade<select name="grade">${Array.from({length:7},(_,i)=>`<option value="${i+6}" ${i+6===q.grade?'selected':''}>Grade ${i+6}</option>`).join('')}</select></label><label class="field">Short description<input name="description" value="${E(q.description)}" maxlength="400" placeholder="What will learners discover?"></label></div></section>
    ${q.questions.map((question,i)=>`<section class="editor-question" data-question="${i}"><div class="editor-question-head"><b>Question ${i+1}</b>${q.questions.length>1?`<button type="button" data-action="remove-question" data-index="${i}">Remove question</button>`:''}</div><label class="field">Question format<select name="type" data-question-type="${i}">${['mcq','true_false','short_answer'].map(t=>`<option value="${t}" ${question.type===t?'selected':''}>${typeName(t)}</option>`).join('')}</select></label><label class="field">Question<input name="prompt" value="${E(question.prompt)}" maxlength="600" required placeholder="What would you like to ask?"></label>${question.type==='mcq'?`<label class="field">Four answer options, one per line<textarea name="options" required>${E(question.options.join('\n'))}</textarea></label>`:''}<div class="form-grid"><label class="field">Correct answer${question.type==='true_false'?`<select name="answer"><option ${question.answer==='True'?'selected':''}>True</option><option ${question.answer==='False'?'selected':''}>False</option></select>`:`<input name="answer" value="${E(question.answer)}" maxlength="600" required placeholder="${question.type==='mcq'?'Copy one option exactly':'Expected short answer'}">`}</label><label class="field">Explanation<input name="explanation" value="${E(question.explanation)}" maxlength="600" required placeholder="Help learners understand why"></label></div></section>`).join('')}
    <div class="editor-actions"><button class="btn secondary" type="button" data-action="add-question">${icon('plus')} Add question</button><button class="btn primary" type="submit">${icon('check')} ${q.id?'Save changes':'Publish quiz'}</button></div></form></div>`);
}

function readEditor() {
  const form = document.querySelector('#editor-form');
  const questions = [...form.querySelectorAll('[data-question]')].map(row=>{
    const type = row.querySelector('[name="type"]').value;
    const value = name => row.querySelector(`[name="${name}"]`)?.value || '';
    return {type,prompt:value('prompt'),options:type==='mcq'?value('options').split('\n'):type==='true_false'?['True','False']:[],answer:value('answer'),explanation:value('explanation')};
  });
  return {...editor,title:form.elements.title.value,subject:form.elements.subject.value,grade:Number(form.elements.grade.value),description:form.elements.description.value,questions};
}

async function render() {
  const sequence = ++renderNumber;
  if (!user) {
    if (!['/login','/register'].includes(location.pathname)) history.replaceState({},'', '/login');
    authPage();
    return;
  }
  const path = location.pathname;
  if (path==='/login'||path==='/register'||path==='/') {
    history.replaceState({},'', '/dashboard');
    return render();
  }
  if (path.startsWith('/quiz/')) {
    if (!run) { history.replaceState({},'', '/quizzes'); return render(); }
    quizRunner();
    return;
  }
  const [catalogueData, progressData] = await Promise.all([api('quizzes'),api('progress')]);
  if (sequence!==renderNumber) return;
  quizzes = catalogueData.quizzes;
  progress = progressData;
  if (path.startsWith('/results/')) return resultsPage(await api('attempts/'+path.split('/').pop()));
  if (path.startsWith('/admin/')) {
    if (user.role!=='admin') { history.replaceState({},'', '/dashboard'); return dashboard(); }
    editor = path==='/admin/new'?{title:'',subject:'Mathematics',grade:8,description:'',questions:[newQuestion()]}:await api('admin/quizzes/'+path.split('/').pop());
    return quizEditor();
  }
  if (path==='/quizzes') return catalogue();
  if (path==='/progress') return progressPage();
  if (path!=='/dashboard') history.replaceState({},'', '/dashboard');
  dashboard();
}

function showPageError(error) {
  if (!user) return toast(error.message);
  shell('Something went wrong',emptyState('We couldn’t load this page',error.message)+`<button class="btn primary" data-action="retry">Try again</button>`);
}

app.addEventListener('click', async event=>{
  const link = event.target.closest('a');
  if (link && new URL(link.href).origin===location.origin && !event.ctrlKey && !event.metaKey) {
    event.preventDefault();
    go(new URL(link.href).pathname);
    return;
  }
  const button = event.target.closest('[data-action]');
  if (!button || button.disabled) return;
  const action = button.dataset.action;
  try {
    if (action==='password') {
      const input=document.querySelector('#password');
      input.type=input.type==='password'?'text':'password';
      button.setAttribute('aria-label', input.type==='password'?'Show password':'Hide password');
    } else if (action==='demo') {
      const admin=button.dataset.role==='admin';
      document.querySelector('[name="email"]').value=admin?'admin@quizora.test':'learner@quizora.test';
      document.querySelector('#password').value=admin?'Teach@12345':'Learn@12345';
      document.querySelector('#auth-form').requestSubmit();
    } else if (action==='menu') {
      document.querySelector('.sidebar').classList.toggle('open');
    } else if (action==='logout') {
      await api('logout',{method:'POST'});
      user=null; run=null;
      const fresh=await api('session'); csrfToken=fresh.csrf;
      go('/login');
    } else if (action==='subject') {
      filters={subject:button.dataset.subject,grade:String(user.grade),search:''}; go('/quizzes');
    } else if (action==='filter-subject') {
      filters.subject=button.dataset.subject; catalogue();
    } else if (action==='reset-filters') {
      filters={subject:'All subjects',grade:'',search:''}; catalogue();
    } else if (action==='start') {
      button.disabled=true;
      run=await api(`quizzes/${button.dataset.id}/start`,{method:'POST',body:{}});
      answers=Array(run.questions.length).fill(''); questionIndex=0;
      history.pushState({},'', '/quiz/'+run.run_id); quizRunner();
    } else if (action==='answer') {
      answers[questionIndex]=run.questions[questionIndex].options[Number(button.dataset.index)]; quizRunner();
    } else if (action==='previous'||action==='jump') {
      saveShortAnswer();
      questionIndex=action==='previous'?Math.max(0,questionIndex-1):Number(button.dataset.index); quizRunner();
    } else if (action==='next') {
      saveShortAnswer();
      if (!answers[questionIndex]?.trim()) { const box=document.querySelector('#question-error'); box.textContent='Choose or enter an answer to continue.'; box.classList.remove('hidden'); return; }
      if (questionIndex<run.questions.length-1) { questionIndex++; quizRunner(); }
      else {
        const missing=answers.findIndex(a=>!a.trim());
        if (missing!==-1) { questionIndex=missing; quizRunner(); toast('Answer every question before finishing.'); return; }
        button.disabled=true;
        const result=await api(`runs/${run.run_id}/submit`,{method:'POST',body:{answers}});
        run=null; go('/results/'+result.attempt_id);
      }
    } else if (action==='exit-quiz') {
      go('/quizzes');
    } else if (action==='archive') {
      if (!await confirmAction('Archive this quiz? Existing attempt results will remain available.')) return;
      await api('admin/quizzes/'+button.dataset.id,{method:'DELETE'});
      toast('Quiz archived.'); await render();
    } else if (action==='add-question') {
      editor=readEditor();
      if (editor.questions.length>=30) return toast('A quiz can have up to 30 questions.');
      editor.questions.push(newQuestion()); quizEditor();
      document.querySelector('.editor-question:last-of-type')?.scrollIntoView({behavior:'smooth',block:'center'});
    } else if (action==='remove-question') {
      editor=readEditor(); editor.questions.splice(Number(button.dataset.index),1); quizEditor();
    } else if (action==='retry') await render();
  } catch(error) { toast(error.message); if(button.isConnected) button.disabled=false; }
});

function saveShortAnswer() {
  const field=document.querySelector('#short-answer');
  if(field) answers[questionIndex]=field.value.trim();
}

app.addEventListener('input',event=>{
  if(event.target.id==='quiz-search') { filters.search=event.target.value; filterResults(); }
  if(event.target.id==='short-answer') answers[questionIndex]=event.target.value;
});
app.addEventListener('change',event=>{
  if(event.target.id==='grade-filter') { filters.grade=event.target.value; filterResults(); }
  if(event.target.matches('[data-question-type]')) {
    editor=readEditor();
    const q=editor.questions[Number(event.target.dataset.questionType)];
    q.options=q.type==='mcq'?['','','','']:q.type==='true_false'?['True','False']:[];
    q.answer=q.type==='true_false'?'True':'';
    quizEditor();
  }
});

app.addEventListener('submit',async event=>{
  event.preventDefault();
  const form=event.target;
  if(form.id==='top-search') { filters={subject:'All subjects',grade:'',search:form.elements.search.value}; go('/quizzes'); return; }
  const button=form.querySelector('[type="submit"]');
  const errorBox=form.querySelector('#form-error');
  if(errorBox) errorBox.classList.add('hidden');
  if(button) button.disabled=true;
  try {
    if(form.id==='auth-form') {
      const signup=location.pathname==='/register';
      const data=Object.fromEntries(new FormData(form));
      if(signup) data.grade=Number(data.grade);
      const result=await api(signup?'register':'login',{method:'POST',body:data});
      user=result.user; csrfToken=result.csrf; go('/dashboard');
    } else if(form.id==='editor-form') {
      editor=readEditor();
      await api('admin/quizzes'+(editor.id?'/'+editor.id:''),{method:editor.id?'PUT':'POST',body:editor});
      toast('Your quiz is ready for learners.'); go('/quizzes');
    }
  } catch(error) {
    if(errorBox) { errorBox.textContent=error.message; errorBox.classList.remove('hidden'); errorBox.scrollIntoView({block:'center'}); }
    else toast(error.message);
  } finally { if(button?.isConnected) button.disabled=false; }
});

window.addEventListener('popstate',()=>{ if(run&&!location.pathname.startsWith('/quiz/')) run=null; render().catch(showPageError); });
window.addEventListener('beforeunload',event=>{ if(run) { event.preventDefault(); event.returnValue=''; } });
async function init() {
  const data=await api('session');
  user=data.user; csrfToken=data.csrf;
  await render();
}
init().catch(error=>{app.innerHTML=emptyState('Unable to connect',error.message);});
