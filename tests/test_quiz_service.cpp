#include "test.h"
#include "temp_directory.h"
#include "quiz/auth_service.h"
#include "quiz/quiz_service.h"
#include "quiz/result_service.h"
#include <sstream>
#include <fstream>

TEST(question_and_quiz_management_require_admin_and_persist_records) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    auto admin = auth.signIn("owner", "admin-password");
    auto learner = auth.signIn("alex", "learner-password");
    quiz::QuizService service(temp.path);
    quiz::QuestionDraft q{"Capital of France?", {"Paris", "Rome", "Oslo", "Lima"}, 'A'};
    REQUIRE(service.saveQuestion(*admin.value, q));
    REQUIRE(service.saveQuestion(*learner.value, q).error == quiz::Error::AccessDenied);
    quiz::QuizDraft draft{"Geography", {1}};
    REQUIRE(service.saveQuiz(*admin.value, draft));
    REQUIRE(service.saveQuiz(*admin.value, {"Bad quiz", {999}}).error == quiz::Error::UnknownQuestion);
    auto listed = service.listQuizzes(*learner.value);
    REQUIRE(listed && listed.value->size() == 1);
    REQUIRE(listed.value->at(0).title == "Geography");
    REQUIRE(service.listQuizzes(*admin.value).error == quiz::Error::AccessDenied);
    quiz::QuizService restarted(temp.path);
    listed = restarted.listQuizzes(*learner.value);
    REQUIRE(listed && listed.value->size() == 1);
}
TEST(quiz_attempt_scores_and_persists_only_after_all_answers) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    auto admin = auth.signIn("owner", "admin-password");
    auto learner = auth.signIn("alex", "learner-password");
    quiz::QuizService service(temp.path);
    REQUIRE(service.saveQuestion(*admin.value, {"First?", {"a","b","c","d"}, 'A'}));
    REQUIRE(service.saveQuestion(*admin.value, {"Second?", {"a","b","c","d"}, 'C'}));
    REQUIRE(service.saveQuestion(*admin.value, {"Third?", {"a","b","c","d"}, 'B'}));
    auto quizId = service.saveQuiz(*admin.value, {"Sample quiz", {1,2,3}});
    REQUIRE(quizId);
    std::istringstream input("A\nD\nB\n");
    std::ostringstream output;
    const auto score = service.takeQuiz(*learner.value, *quizId.value, input, output);
    REQUIRE(score);
    REQUIRE(score.value->correctCount == 2);
    REQUIRE(score.value->totalQuestions == 3);
    REQUIRE(score.value->percentage > 66.69 && score.value->percentage < 66.71);
    REQUIRE(output.str().find("66.7%") != std::string::npos);
    quiz::FileAttemptStore attempts(temp.path);
    REQUIRE(attempts.attempts().size() == 1);
    REQUIRE(attempts.attempts()[0].correctCount == 2);
    REQUIRE(attempts.attempts()[0].completed);
}
TEST(incomplete_attempt_does_not_create_a_saved_score) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    auto admin = auth.signIn("owner", "admin-password");
    auto learner = auth.signIn("alex", "learner-password");
    quiz::QuizService service(temp.path);
    REQUIRE(service.saveQuestion(*admin.value, {"Only?", {"a","b","c","d"}, 'A'}));
    auto quizId = service.saveQuiz(*admin.value, {"One", {1}});
    REQUIRE(quizId);
    std::istringstream input("");
    std::ostringstream output;
    REQUIRE(service.takeQuiz(*learner.value, *quizId.value, input, output).error == quiz::Error::Incomplete);
    quiz::FileAttemptStore attempts(temp.path);
    REQUIRE(attempts.attempts().empty());
}
TEST(deactivated_question_is_not_available_to_learners) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    auto admin = auth.signIn("owner", "admin-password");
    auto learner = auth.signIn("alex", "learner-password");
    quiz::QuizService service(temp.path);
    REQUIRE(service.saveQuestion(*admin.value, {"Question?", {"a","b","c","d"}, 'A'}));
    REQUIRE(service.saveQuiz(*admin.value, {"Quiz", {1}}));
    REQUIRE(service.deactivateQuestion(*admin.value, 1) == quiz::Error::None);
    REQUIRE(service.listQuizzes(*learner.value).value->empty());
    quiz::QuizService restarted(temp.path);
    REQUIRE(restarted.listQuizzes(*learner.value).value->empty());
}

TEST(quiz_repository_rejects_malformed_question_and_quiz_records) {
    TempDirectory temp;
    {
        std::ofstream questions(temp.path / "questions.db");
        questions << "QUIZ_QUESTIONS_V1\n"
                  << "1 1 Z \"Prompt\" \"A\" \"B\" \"C\" \"D\"\n";
    }
    bool rejected = false;
    try {
        quiz::QuizService service(temp.path);
    }
    catch (const std::runtime_error&) {
        rejected = true;
    }
    REQUIRE(rejected);
}
TEST(quiz_repository_rejects_unknown_question_references) {
    TempDirectory temp;
    {
        std::ofstream questions(temp.path / "questions.db");
        questions << "QUIZ_QUESTIONS_V1\n"
                  << "1 1 A \"Prompt\" \"A\" \"B\" \"C\" \"D\"\n";
    }
    {
        std::ofstream quizzes(temp.path / "quizzes.db");
        quizzes << "QUIZ_QUIZZES_V1\n"
                << "1 \"Broken quiz\" 1 999\n";
    }
    bool rejected = false;
    try {
        quiz::QuizService service(temp.path);
    }
    catch (const std::runtime_error&) {
        rejected = true;
    }
    REQUIRE(rejected);
}
