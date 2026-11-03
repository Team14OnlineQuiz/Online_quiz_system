#include "test.h"
#include "temp_directory.h"
#include "quiz/account_store.h"
#include "quiz/password_hasher.h"
#include <fstream>
#include <iterator>
TEST(accounts_persist_hashes_and_roles_across_restart) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    {
        quiz::FileAccountStore store(temp.path);
        REQUIRE(store.insert("alex", hasher.hash("test-password"), quiz::Role::Learner));
        REQUIRE(store.insert("owner", hasher.hash("admin-password"), quiz::Role::Admin));
        REQUIRE(store.insert("alex", hasher.hash("test-password"), quiz::Role::Learner).error == quiz::Error::DuplicateUser);
    }
    quiz::FileAccountStore reloaded(temp.path);
    REQUIRE(reloaded.users().size() == 2);
    REQUIRE(reloaded.find("alex")->role == quiz::Role::Learner);
    REQUIRE(reloaded.find("owner")->role == quiz::Role::Admin);
    std::ifstream file(temp.path / "users.db");
    std::string text((std::istreambuf_iterator<char>(file)), {});
    REQUIRE(text.find("test-password") == std::string::npos);
    REQUIRE(text.find("admin-password") == std::string::npos);
    REQUIRE(text.find("$argon2id$") != std::string::npos);
}
TEST(account_write_failure_does_not_create_an_account) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    REQUIRE(store.insert("alex", hasher.hash("test-password"), quiz::Role::Learner));
    // A directory at the temporary filename reliably injects a write failure on both OSes.
    std::filesystem::create_directory(temp.path / "users.db.tmp");
    REQUIRE(store.insert("second", hasher.hash("test-password"), quiz::Role::Learner).error == quiz::Error::Storage);
    REQUIRE(store.users().size() == 1);
    REQUIRE(store.find("second") == nullptr);
    quiz::FileAccountStore reload(temp.path);
    REQUIRE(reload.users().size() == 1);
}
TEST(malformed_role_never_becomes_an_administrator) {
    TempDirectory temp;
    std::ofstream(temp.path / "users.db") << "QUIZ_USERS_V1\n1 99 \"alex\" \"plaintext\"\n";
    bool rejected = false;
    try { quiz::FileAccountStore bad(temp.path); } catch (const std::runtime_error&) { rejected = true; }
    REQUIRE(rejected);
}


TEST(account_store_rejects_unknown_role_values) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    const auto invalidRole = static_cast<quiz::Role>(99);
    const auto result = store.insert("intruder", hasher.hash("test-password"), invalidRole);
    REQUIRE(!result);
    REQUIRE(result.error == quiz::Error::Validation);
    REQUIRE(store.users().empty());
    REQUIRE(store.find("intruder") == nullptr);
}
