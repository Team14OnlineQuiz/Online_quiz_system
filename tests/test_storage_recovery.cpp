#include "test.h"
#include "temp_directory.h"
#include "quiz/account_store.h"
#include "quiz/auth_service.h"
#include "quiz/question_catalog.h"
#include "quiz/quiz_catalog.h"
#include "quiz/result_service.h"
#include <fstream>
#include <iterator>
#include <string>

namespace {
std::string readBytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}
quiz::QuestionPrompt sampleQuestion(const std::string& text) {
    return {0, text, {"Choice A", "Choice B", "Choice C", "Choice D"}};
}
}

TEST(account_write_failure_preserves_exact_committed_file_and_restart_state) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    {
        quiz::FileAccountStore accounts(temp.path);
        REQUIRE(accounts.insert("alex", hasher.hash("learner-password"), quiz::Role::Learner));
        const auto committed = readBytes(temp.path / "users.db");
        REQUIRE(!committed.empty());
        std::filesystem::create_directory(temp.path / "users.db.tmp");
        const auto failed = accounts.insert("blair", hasher.hash("second-password"), quiz::Role::Learner);
        REQUIRE(failed.error == quiz::Error::Storage);
        REQUIRE(readBytes(temp.path / "users.db") == committed);
        REQUIRE(accounts.find("alex") != nullptr);
        REQUIRE(accounts.find("blair") == nullptr);
    }
    quiz::FileAccountStore restarted(temp.path);
    REQUIRE(restarted.find("alex") != nullptr);
    REQUIRE(restarted.find("blair") == nullptr);
}

TEST(question_write_failure_preserves_exact_committed_file_and_restart_state) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    const auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);
    {
        quiz::QuestionCatalog questions(temp.path);
        REQUIRE(questions.add(*admin.value, sampleQuestion("First?"), 'A'));
        const auto committed = readBytes(temp.path / "questions.db");
        REQUIRE(!committed.empty());
        std::filesystem::create_directory(temp.path / "questions.db.tmp");
        const auto failed = questions.add(*admin.value, sampleQuestion("Second?"), 'B');
        REQUIRE(failed.error == quiz::Error::Storage);
        REQUIRE(readBytes(temp.path / "questions.db") == committed);
        REQUIRE(questions.size() == 1);
    }
    quiz::QuestionCatalog restarted(temp.path);
    REQUIRE(restarted.size() == 1);
    REQUIRE(restarted.find(1) != nullptr);
    REQUIRE(restarted.find(2) == nullptr);
}

TEST(quiz_write_failure_preserves_exact_committed_file_and_restart_state) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    const auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);
    quiz::QuestionCatalog questions(temp.path);
    const auto question = questions.add(*admin.value, sampleQuestion("First?"), 'A');
    REQUIRE(question);
    {
        quiz::QuizCatalog quizzes(temp.path, questions);
        REQUIRE(quizzes.add(*admin.value, questions, "Original quiz", {*question.value}));
        const auto committed = readBytes(temp.path / "quizzes.db");
        REQUIRE(!committed.empty());
        std::filesystem::create_directory(temp.path / "quizzes.db.tmp");
        const auto failed = quizzes.add(*admin.value, questions, "Rejected quiz", {*question.value});
        REQUIRE(failed.error == quiz::Error::Storage);
        REQUIRE(readBytes(temp.path / "quizzes.db") == committed);
        REQUIRE(quizzes.list(*admin.value).size() == 1);
    }
    quiz::QuizCatalog restarted(temp.path, questions);
    REQUIRE(restarted.list(*admin.value).size() == 1);
    REQUIRE(restarted.find(*admin.value, 1) != nullptr);
    REQUIRE(restarted.find(*admin.value, 2) == nullptr);
}

TEST(attempt_write_failure_preserves_exact_committed_file_and_restart_state) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    const auto learnerId = auth.registerLearner("alex", "learner-password");
    REQUIRE(learnerId);
    const auto learner = auth.signIn("alex", "learner-password");
    REQUIRE(learner);
    {
        quiz::FileAttemptStore attempts(temp.path);
        quiz::AttemptService submission(attempts);
        REQUIRE(submission.submit(*learner.value, "Original quiz", "2026-10-20T10:00:00Z", 2, 3));
        const auto committed = readBytes(temp.path / "attempts.db");
        REQUIRE(!committed.empty());
        std::filesystem::create_directory(temp.path / "attempts.db.tmp");
        const auto failed = submission.submit(*learner.value, "Rejected quiz", "2026-10-20T11:00:00Z", 1, 3);
        REQUIRE(failed.error == quiz::Error::Storage);
        REQUIRE(readBytes(temp.path / "attempts.db") == committed);
        REQUIRE(attempts.attempts().size() == 1);
    }
    quiz::FileAttemptStore restarted(temp.path);
    REQUIRE(restarted.attempts().size() == 1);
    REQUIRE(restarted.attempts().front().userId == *learnerId.value);
    REQUIRE(restarted.attempts().front().quizTitle == "Original quiz");
}
