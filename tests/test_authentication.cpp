#include "test.h"
#include "temp_directory.h"
#include "quiz/auth_service.h"
TEST(authentication_preserves_roles_and_uses_one_failure_type) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    const auto badName = auth.signIn("missing", "learner-password");
    const auto badPassword = auth.signIn("alex", "wrong-password");
    REQUIRE(badName.error == quiz::Error::Authentication);
    REQUIRE(badPassword.error == badName.error);
    REQUIRE(!badName.value.has_value());
    REQUIRE(!badPassword.value.has_value());
    auto learner = auth.signIn("alex", "learner-password");
    REQUIRE(learner);
    REQUIRE(learner.value->authenticated());
    REQUIRE(!learner.value->isAdmin());
    REQUIRE(learner.value->userId() == store.find("alex")->id);
    auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);
    REQUIRE(admin.value->isAdmin());
    auth.signOut(*admin.value);
    REQUIRE(!admin.value->authenticated());
    REQUIRE(!admin.value->isAdmin());
}
TEST(authentication_works_after_restart_without_plaintext_credentials) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    {
        quiz::FileAccountStore first(temp.path);
        quiz::AuthService auth(first, hasher);
        REQUIRE(auth.registerLearner("alex", "learner-password"));
    }
    quiz::FileAccountStore restarted(temp.path);
    quiz::AuthService auth(restarted, hasher);
    REQUIRE(auth.signIn("alex", "learner-password"));
    REQUIRE(!auth.signIn("alex", "wrong-password"));
}
