#include "test.h"
#include "temp_directory.h"
#include "quiz/auth_service.h"
#include "quiz/result_service.h"
#include <fstream>
#include <sstream>
struct MemoryAttempts : quiz::AttemptStore {
    std::vector<quiz::AttemptRecord> values;
    const std::vector<quiz::AttemptRecord>& attempts() const override { return values; }
};
TEST(administrator_summary_shows_all_completed_attempts_and_stable_scores) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "setup-password"));
    const auto alex = auth.registerLearner("alex", "learner-password");
    const auto blair = auth.registerLearner("blair", "learner-password");
    REQUIRE(alex && blair);
    MemoryAttempts attempts;
    attempts.values = {{1, *alex.value, "Original quiz title", "2026-10-20T10:00:00Z", 2, 3, true},
        {2, *blair.value, "Second quiz", "2026-10-20T11:00:00Z", 3, 3, true},
        {3, *alex.value, "Unsubmitted", "2026-10-20T12:00:00Z", 0, 3, false}};
    quiz::ResultService results(accounts, attempts);
    const auto admin = auth.signIn("owner", "setup-password");
    const auto summary = results.summary(*admin.value);
    REQUIRE(summary && summary.value->size() == 2);
    REQUIRE(summary.value->at(0).username == "alex");
    REQUIRE(summary.value->at(1).username == "blair");
    std::ostringstream output;
    REQUIRE(results.displaySummary(*admin.value, output) == quiz::Error::None);
    REQUIRE(output.str().find("Original quiz title") != std::string::npos);
    REQUIRE(output.str().find("2026-10-20T10:00:00Z") != std::string::npos);
    REQUIRE(output.str().find("2/3 | 66.7%") != std::string::npos);
    REQUIRE(output.str().find("Unsubmitted") == std::string::npos);
}
TEST(learner_guest_and_logged_out_admin_cannot_read_summary) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "setup-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    MemoryAttempts attempts;
    quiz::ResultService results(accounts, attempts);
    REQUIRE(results.summary(quiz::Session{}).error == quiz::Error::AccessDenied);
    auto learner = auth.signIn("alex", "learner-password");
    REQUIRE(results.summary(*learner.value).error == quiz::Error::AccessDenied);
    auto admin = auth.signIn("owner", "setup-password");
    auth.signOut(*admin.value);
    REQUIRE(results.summary(*admin.value).error == quiz::Error::AccessDenied);
}
TEST(attempt_adapter_rejects_invalid_scores_and_reads_completed_snapshots) {
    TempDirectory temp;
    const auto file = temp.path / "attempts.db";
    std::ofstream(file) << "QUIZ_ATTEMPTS_V1\n1 2 1 2 3 \"Sample quiz\" \"2026-10-20T10:00:00Z\"\n";
    quiz::FileAttemptStore attempts(temp.path);
    REQUIRE(attempts.attempts().size() == 1);
    REQUIRE(attempts.attempts()[0].correctCount == 2);
    std::ofstream(file) << "QUIZ_ATTEMPTS_V1\n1 2 1 4 3 \"Sample quiz\" \"2026-10-20T10:00:00Z\"\n";
    bool rejected = false;
    try { attempts.reload(); } catch (const std::runtime_error&) { rejected = true; }
    REQUIRE(rejected);
    REQUIRE(attempts.attempts()[0].correctCount == 2);
    REQUIRE(!quiz::validSubmissionDate("2026-02-30T10:00:00Z"));
    REQUIRE(quiz::validSubmissionDate("2028-02-29T10:00:00Z"));
}


TEST(result_summary_rejects_unknown_account_references) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "setup-password"));
    const auto admin = auth.signIn("owner", "setup-password");
    REQUIRE(admin);

    MemoryAttempts attempts;
    attempts.values = {
        {1, 999, "Unknown owner", "2026-10-20T10:00:00Z", 2, 3, true}
    };

    quiz::ResultService results(accounts, attempts);
    REQUIRE(results.summary(*admin.value).error == quiz::Error::Storage);

    // Incomplete records with unknown owners must also be rejected.
    attempts.values[0].completed = false;
    REQUIRE(results.summary(*admin.value).error == quiz::Error::Storage);
}
