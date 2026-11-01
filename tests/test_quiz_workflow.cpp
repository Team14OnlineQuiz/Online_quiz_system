#include "test.h"
#include "temp_directory.h"
#include "quiz/auth_service.h"
#include "quiz/quiz_service.h"
#include <sstream>
namespace {
quiz::QuestionPrompt makePrompt(const std::string& text) {
    return {0, text, {"Option A", "Option B", "Option C", "Option D"}};
}
}
TEST(learner_discovers_selects_and_completes_a_persisted_quiz) {
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
    const auto q1 = questions.add(*admin.value, makePrompt("First question?"), 'A');
    const auto q2 = questions.add(*admin.value, makePrompt("Second question?"), 'D');
    REQUIRE(q1 && q2);
    quiz::QuizCatalog catalog(temp.path, questions);
    const auto created = catalog.add(*admin.value, questions, "Starter quiz", {*q1.value, *q2.value});
    REQUIRE(created);
    quiz::QuizService service(catalog, questions);
    REQUIRE(service.discover(quiz::Session{}).error == quiz::Error::AccessDenied);
    REQUIRE(service.discover(*admin.value).error == quiz::Error::AccessDenied);
    const auto listings = service.discover(*learner.value);
    REQUIRE(listings && listings.value->size() == 1);
    REQUIRE(listings.value->at(0).id == *created.value);
    REQUIRE(listings.value->at(0).title == "Starter quiz");
    REQUIRE(listings.value->at(0).questionCount == 2);
    const auto selected = service.select(*learner.value, *created.value);
    REQUIRE(selected && selected.value->questions.size() == 2);
    const auto grade = service.grade(*learner.value, *selected.value, {'A', 'D'});
    REQUIRE(grade && *grade.value == 2);
    const auto partialGrade = service.grade(*learner.value, *selected.value, {'A', 'A'});
    REQUIRE(partialGrade && *partialGrade.value == 1);
    REQUIRE(service.select(*learner.value, 9999).error == quiz::Error::Validation);
    quiz::FileAttemptStore attempts(temp.path);
    quiz::AttemptService submission(attempts);
    std::istringstream answers(std::to_string(*created.value) + "\nA\nD\n");
    std::ostringstream output;
    REQUIRE(quiz::runLearnerQuiz(*learner.value, answers, output, service, submission) == quiz::Error::None);
    REQUIRE(output.str().find("Available quizzes") != std::string::npos);
    REQUIRE(output.str().find("Attempt saved.") != std::string::npos);
    REQUIRE(output.str().find("Score: 2/2") != std::string::npos);
    quiz::QuestionCatalog reloadedQuestions(temp.path);
    quiz::QuizCatalog reloadedQuizzes(temp.path, reloadedQuestions);
    quiz::FileAttemptStore reloadedAttempts(temp.path);
    REQUIRE(reloadedQuestions.size() == 2);
    REQUIRE(reloadedQuizzes.list().size() == 1);
    REQUIRE(reloadedAttempts.attempts().size() == 1);
    REQUIRE(reloadedAttempts.attempts().front().userId == learner.value->userId());
    REQUIRE(reloadedAttempts.attempts().front().completed);
    REQUIRE(reloadedAttempts.attempts().front().correctCount == 2);
}
TEST(incomplete_quiz_is_not_saved_or_reported_as_submitted) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    auto admin = auth.signIn("owner", "admin-password");
    auto learner = auth.signIn("alex", "learner-password");
    quiz::QuestionCatalog questions(temp.path);
    const auto q1 = questions.add(*admin.value, makePrompt("Question?"), 'A');
    quiz::QuizCatalog catalog(temp.path, questions);
    const auto created = catalog.add(*admin.value, questions, "One question", {*q1.value});
    quiz::QuizService service(catalog, questions);
    quiz::FileAttemptStore attempts(temp.path);
    quiz::AttemptService submission(attempts);
    std::istringstream answers(std::to_string(*created.value) + "\n");
    std::ostringstream output;
    REQUIRE(quiz::runLearnerQuiz(*learner.value, answers, output, service, submission)
        == quiz::Error::Incomplete);
    REQUIRE(attempts.attempts().empty());
    REQUIRE(output.str().find("No completed attempt was saved.") != std::string::npos);
}
