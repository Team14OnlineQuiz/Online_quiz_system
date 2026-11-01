#include "test.h"
#include "temp_directory.h"
#include "quiz/auth_service.h"
#include "quiz/quiz_catalog.h"
#include "quiz/result_service.h"
TEST(question_quiz_and_aggregate_result_services_enforce_admin_role) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    auto admin = auth.signIn("owner", "admin-password");
    auto learner = auth.signIn("alex", "learner-password");
    REQUIRE(admin && learner);

    quiz::QuestionCatalog questions(temp.path);
    quiz::QuestionPrompt prompt{0, "Question?", {"A", "B", "C", "D"}};
    REQUIRE(questions.add(quiz::Session{}, prompt, 'A').error == quiz::Error::AccessDenied);
    REQUIRE(questions.add(*learner.value, prompt, 'A').error == quiz::Error::AccessDenied);
    REQUIRE(questions.size() == 0);
    const auto question = questions.add(*admin.value, prompt, 'A');
    REQUIRE(question && questions.size() == 1);
    REQUIRE(questions.modify(*learner.value, *question.value, prompt, 'B') == quiz::Error::AccessDenied);
    REQUIRE(questions.remove(quiz::Session{}, *question.value) == quiz::Error::AccessDenied);
    REQUIRE(questions.size() == 1);

    quiz::QuizCatalog quizzes(temp.path, questions);
    REQUIRE(quizzes.add(quiz::Session{}, questions, "Quiz", {*question.value}).error == quiz::Error::AccessDenied);
    REQUIRE(quizzes.add(*learner.value, questions, "Quiz", {*question.value}).error == quiz::Error::AccessDenied);
    REQUIRE(quizzes.list().empty());
    const auto createdQuiz = quizzes.add(*admin.value, questions, "Quiz", {*question.value});
    REQUIRE(createdQuiz && quizzes.list().size() == 1);
    REQUIRE(quizzes.modify(*learner.value, questions, *createdQuiz.value, "Changed", {*question.value})
        == quiz::Error::AccessDenied);
    REQUIRE(quizzes.remove(quiz::Session{}, *quiz.value) == quiz::Error::AccessDenied);
    REQUIRE(quizzes.find(*quiz.value)->title == "Quiz");

    quiz::FileAttemptStore attempts(temp.path);
    quiz::ResultService results(accounts, attempts);
    REQUIRE(results.summary(quiz::Session{}).error == quiz::Error::AccessDenied);
    REQUIRE(results.summary(*learner.value).error == quiz::Error::AccessDenied);
    REQUIRE(results.summary(*admin.value));
}
TEST(attempt_submission_and_learner_history_are_bound_to_authenticated_owner) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    const auto alexId = auth.registerLearner("alex", "learner-password");
    const auto blairId = auth.registerLearner("blair", "learner-password");
    REQUIRE(alexId && blairId);
    auto admin = auth.signIn("owner", "admin-password");
    auto alex = auth.signIn("alex", "learner-password");
    auto blair = auth.signIn("blair", "learner-password");
    REQUIRE(admin && alex && blair);
    quiz::FileAttemptStore attempts(temp.path);
    quiz::AttemptService submission(attempts);
    REQUIRE(submission.submit(quiz::Session{}, "Quiz A", "2026-10-20T10:00:00Z", 2, 3).error
        == quiz::Error::AccessDenied);
    REQUIRE(submission.submit(*admin.value, "Quiz A", "2026-10-20T10:00:00Z", 2, 3).error
        == quiz::Error::AccessDenied);
    REQUIRE(submission.submit(*alex.value, "Quiz A", "2026-10-20T10:00:00Z", 2, 3));
    REQUIRE(submission.submit(*blair.value, "Quiz B", "2026-10-20T11:00:00Z", 1, 3));
    quiz::LearnerHistoryService history(accounts, attempts);
    const auto alexHistory = history.history(*alex.value);
    REQUIRE(alexHistory && alexHistory.value->size() == 1);
    REQUIRE(alexHistory.value->front().quizTitle == "Quiz A");
    REQUIRE(history.historyFor(*alex.value, *blairId.value).error == quiz::Error::AccessDenied);
    REQUIRE(history.history(*admin.value).error == quiz::Error::AccessDenied);
    REQUIRE(history.history(quiz::Session{}).error == quiz::Error::AccessDenied);
}
TEST(question_quiz_and_attempt_file_failures_keep_last_committed_data) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    auto admin = auth.signIn("owner", "admin-password");
    quiz::QuestionCatalog questions(temp.path);
    quiz::QuestionPrompt first{0, "First?", {"A", "B", "C", "D"}};
    quiz::QuestionPrompt second{0, "Second?", {"A", "B", "C", "D"}};
    const auto q1 = questions.add(*admin.value, first, 'A');
    REQUIRE(q1);
    std::filesystem::create_directory(temp.path / "questions.db.tmp");
    REQUIRE(questions.add(*admin.value, second, 'B').error == quiz::Error::Storage);
    REQUIRE(questions.size() == 1);
    quiz::QuestionCatalog questionReload(temp.path);
    REQUIRE(questionReload.size() == 1);

    quiz::QuizCatalog quizzes(temp.path, questionReload);
    REQUIRE(quizzes.add(*admin.value, questionReload, "Original", {*q1.value}));
    std::filesystem::create_directory(temp.path / "quizzes.db.tmp");
    REQUIRE(quizzes.add(*admin.value, questionReload, "Rejected", {*q1.value}).error == quiz::Error::Storage);
    REQUIRE(quizzes.list().size() == 1);
    quiz::QuizCatalog quizReload(temp.path, questionReload);
    REQUIRE(quizReload.list().size() == 1);

    REQUIRE(auth.registerLearner("alex", "learner-password"));
    auto learner = auth.signIn("alex", "learner-password");
    quiz::FileAttemptStore attempts(temp.path);
    quiz::AttemptService submit(attempts);
    REQUIRE(submit.submit(*learner.value, "Original", "2026-10-20T10:00:00Z", 1, 1));
    std::filesystem::create_directory(temp.path / "attempts.db.tmp");
    REQUIRE(submit.submit(*learner.value, "Rejected", "2026-10-20T11:00:00Z", 0, 1).error
        == quiz::Error::Storage);
    REQUIRE(attempts.attempts().size() == 1);
    quiz::FileAttemptStore attemptReload(temp.path);
    REQUIRE(attemptReload.attempts().size() == 1);
    REQUIRE(attemptReload.attempts().front().quizTitle == "Original");
}