#include "test.h"
#include "temp_directory.h"
#include "quiz/auth_service.h"
TEST(registration_validates_boundaries_and_duplicate_names) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);
    REQUIRE(auth.registerLearner("", "12345678").error == quiz::Error::Validation);
    REQUIRE(auth.registerLearner("   ", "12345678").error == quiz::Error::Validation);
    REQUIRE(auth.registerLearner("alex\nadmin", "12345678").error == quiz::Error::Validation);
    REQUIRE(auth.registerLearner("alex", "1234567").error == quiz::Error::Validation);
    REQUIRE(auth.registerLearner("unicode", u8"ééééééé").error == quiz::Error::Validation);
    REQUIRE(auth.registerLearner("alex", "12345678"));
    REQUIRE(auth.registerLearner("alex", "different-password").error == quiz::Error::DuplicateUser);
    REQUIRE(store.users().size() == 1);
    REQUIRE(store.find("alex")->role == quiz::Role::Learner);
    quiz::FileAccountStore reload(temp.path);
    REQUIRE(reload.find("alex") != nullptr);
}
TEST(first_admin_is_provisioned_only_once_and_never_by_registration) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "setup-password"));
    REQUIRE(auth.provisionFirstAdmin("second", "setup-password").error == quiz::Error::AccessDenied);
    REQUIRE(auth.registerLearner("admin", "test-password"));
    REQUIRE(store.find("admin")->role == quiz::Role::Learner);
}
TEST(registration_does_not_claim_success_when_storage_fails) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);
    std::filesystem::create_directory(temp.path / "users.db.tmp");
    REQUIRE(auth.registerLearner("alex", "test-password").error == quiz::Error::Storage);
    REQUIRE(store.find("alex") == nullptr);
}
