#include "test.h"
#include "temp_directory.h"
#include "quiz/quiz_service.h"
#include "quiz/auth_service.h"

#include <fstream>

namespace {
void writeQuestions(const std::filesystem::path& directory) {
    std::ofstream output(directory / "questions.db");
    output << "QUIZ_QUESTIONS_V1\n"
           << "1 1 A \"First question?\" \"one\" \"two\" \"three\" \"four\"\n"
           << "2 1 B \"Second question?\" \"one\" \"two\" \"three\" \"four\"\n"
           << "3 0 C \"Inactive question?\" \"one\" \"two\" \"three\" \"four\"\n";
}
}

TEST(quiz_service_admin_creates_and_lists_quiz_in_question_order) {
    TempDirectory temp;
    writeQuestions(temp.path);

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);

    quiz::QuizService service(temp.path);
    quiz::QuizDraft draft{"Science", {2, 1}};
    auto saved = service.saveQuiz(*admin.value, draft);
    REQUIRE(saved);

    auto listed = service.listQuizzes(*admin.value);
    REQUIRE(listed);
    REQUIRE(listed.value->size() == 1);
    REQUIRE(listed.value->front().title == "Science");
    REQUIRE(listed.value->front().questionIds.size() == 2);
    REQUIRE(listed.value->front().questionIds[0] == 2);
    REQUIRE(listed.value->front().questionIds[1] == 1);
}

TEST(quiz_service_admin_edits_existing_quiz) {
    TempDirectory temp;
    writeQuestions(temp.path);

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);

    quiz::QuizService service(temp.path);
    auto saved = service.saveQuiz(*admin.value, {"Old title", {1}});
    REQUIRE(saved);

    auto edited = service.saveQuiz(*admin.value, {"New title", {2, 1}}, *saved.value);
    REQUIRE(edited);
    REQUIRE(*edited.value == *saved.value);

    auto listed = service.listQuizzes(*admin.value);
    REQUIRE(listed);
    REQUIRE(listed.value->front().title == "New title");
    REQUIRE(listed.value->front().questionIds.size() == 2);
}

TEST(quiz_service_rejects_invalid_quiz_drafts_and_unknown_questions) {
    TempDirectory temp;
    writeQuestions(temp.path);

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);

    quiz::QuizService service(temp.path);
    REQUIRE(service.saveQuiz(*admin.value, {"", {1}}).error == quiz::Error::Validation);
    REQUIRE(service.saveQuiz(*admin.value, {"Empty", {}}).error == quiz::Error::Validation);
    REQUIRE(service.saveQuiz(*admin.value, {"Unknown", {999}}).error == quiz::Error::Validation);
    REQUIRE(service.saveQuiz(*admin.value, {"Inactive", {3}}).error == quiz::Error::Validation);
    REQUIRE(service.saveQuiz(*admin.value, {"Repeated", {1, 1}}).error == quiz::Error::Validation);

    auto listed = service.listQuizzes(*admin.value);
    REQUIRE(listed);
    REQUIRE(listed.value->empty());
}

TEST(quiz_service_denies_learner_changes) {
    TempDirectory temp;
    writeQuestions(temp.path);

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));

    auto learner = auth.signIn("alex", "learner-password");
    REQUIRE(learner);

    quiz::QuizService service(temp.path);
    REQUIRE(service.saveQuiz(*learner.value, {"Science", {1}}).error == quiz::Error::AccessDenied);

    auto listed = service.listQuizzes(*learner.value);
    REQUIRE(listed);
    REQUIRE(listed.value->empty());
}

TEST(quiz_service_persists_quiz_after_restart) {
    TempDirectory temp;
    writeQuestions(temp.path);

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);

    quiz::Id savedId = 0;
    {
        quiz::QuizService service(temp.path);
        auto saved = service.saveQuiz(*admin.value, {"Science", {2, 1}});
        REQUIRE(saved);
        savedId = *saved.value;
    }

    quiz::QuizService restarted(temp.path);
    auto listed = restarted.listQuizzes(*admin.value);
    REQUIRE(listed);
    REQUIRE(listed.value->size() == 1);
    REQUIRE(listed.value->front().id == savedId);
    REQUIRE(listed.value->front().questionIds[0] == 2);
}

TEST(quiz_service_rejects_saved_quiz_with_inactive_question) {
    TempDirectory temp;
    writeQuestions(temp.path);

    {
        std::ofstream output(temp.path / "quizzes.db");
        output << "QUIZ_DEFINITIONS_V1\n"
               << "1 \"Invalid saved quiz\" 1 3\n";
    }

    bool rejected = false;
    try {
        quiz::QuizService service(temp.path);
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    REQUIRE(rejected);
}
