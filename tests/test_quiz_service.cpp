#include "test.h"
#include "temp_directory.h"
#include "quiz/quiz_catalog.h"
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

    quiz::QuestionCatalog questions(temp.path);
    quiz::QuizCatalog service(temp.path, questions);
    auto saved = service.add(*admin.value, questions, "Science", {2, 1});
    REQUIRE(saved);

    auto listed = service.list(*admin.value);
    REQUIRE(listed.size() == 1);
    REQUIRE(listed.front().title == "Science");
    REQUIRE(listed.front().questionIds.size() == 2);
    REQUIRE(listed.front().questionIds[0] == 2);
    REQUIRE(listed.front().questionIds[1] == 1);
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

    quiz::QuestionCatalog questions(temp.path);
    quiz::QuizCatalog service(temp.path, questions);
    auto saved = service.add(*admin.value, questions, "Old title", {1});
    REQUIRE(saved);

    auto edited = service.modify(*admin.value, questions, *saved.value, "New title", {2, 1});
    REQUIRE(edited == quiz::Error::None);

    auto listed = service.list(*admin.value);
    REQUIRE(listed.front().title == "New title");
    REQUIRE(listed.front().questionIds.size() == 2);
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

    quiz::QuestionCatalog questions(temp.path);
    quiz::QuizCatalog service(temp.path, questions);
    REQUIRE(service.add(*admin.value, questions, "", {1}).error == quiz::Error::Validation);
    REQUIRE(service.add(*admin.value, questions, "Empty", {}).error == quiz::Error::Validation);
    REQUIRE(service.add(*admin.value, questions, "Unknown", {999}).error == quiz::Error::Validation);
    REQUIRE(service.add(*admin.value, questions, "Inactive", {3}).error == quiz::Error::Validation);
    REQUIRE(service.add(*admin.value, questions, "Repeated", {1, 1}).error == quiz::Error::Validation);

    auto listed = service.list(*admin.value);
    REQUIRE(listed.empty());
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

    quiz::QuestionCatalog questions(temp.path);
    quiz::QuizCatalog service(temp.path, questions);
    REQUIRE(service.add(*learner.value, questions, "Science", {1}).error == quiz::Error::AccessDenied);

    auto listed = service.list(*learner.value);
    REQUIRE(listed.empty());
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
        quiz::QuestionCatalog questions(temp.path);
    quiz::QuizCatalog service(temp.path, questions);
        auto saved = service.add(*admin.value, questions, "Science", {2, 1});
        REQUIRE(saved);
        savedId = *saved.value;
    }

    quiz::QuestionCatalog questions(temp.path);
    quiz::QuizCatalog restarted(temp.path, questions);
    auto listed = restarted.list(*admin.value);
    REQUIRE(listed.size() == 1);
    REQUIRE(listed.front().id == savedId);
    REQUIRE(listed.front().questionIds[0] == 2);
}

TEST(quiz_service_rejects_saved_quiz_with_unknown_question) {
    TempDirectory temp;
    writeQuestions(temp.path);

    {
        std::ofstream output(temp.path / "quizzes.db");
        output << "QUIZ_QUIZZES_V1\n"
               << "1 1 \"Invalid saved quiz\" 1 999\n";
    }

    bool rejected = false;
    try {
        quiz::QuestionCatalog questions(temp.path);
    quiz::QuizCatalog service(temp.path, questions);
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    REQUIRE(rejected);
}
