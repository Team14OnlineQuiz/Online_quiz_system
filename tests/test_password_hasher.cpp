#include "test.h"
#include "quiz/password_hasher.h"
TEST(argon2id_hashes_include_unique_salts) {
    quiz::PasswordHasher hasher;
    const std::string password = "test-only-password";
    const auto first = hasher.hash(password);
    const auto second = hasher.hash(password);
    REQUIRE(first != second);
    REQUIRE(first.rfind("$argon2id$", 0) == 0);
    REQUIRE(first.find(password) == std::string::npos);
    REQUIRE(hasher.verify(password, first));
    REQUIRE(hasher.verify(password, second));
    REQUIRE(!hasher.verify("wrong-password", first));
    // Compare encoded salts, not just the overall hash strings.
    const auto saltStart = first.find('$', first.find("m=")) + 1;
    const auto saltEnd = first.find('$', saltStart);
    const auto saltStart2 = second.find('$', second.find("m=")) + 1;
    REQUIRE(first.substr(saltStart, saltEnd - saltStart)
        != second.substr(saltStart2, second.find('$', saltStart2) - saltStart2));
}
TEST(malformed_or_unbounded_hash_input_fails_closed) {
    quiz::PasswordHasher hasher;
    REQUIRE(!hasher.verify("password", "plaintext"));
    REQUIRE(!hasher.verify("password", "$argon2id$invalid"));
    REQUIRE(!hasher.verify(std::string(1025, 'x'), "$argon2id$invalid"));
    bool rejected = false;
    try { (void)hasher.hash("short"); } catch (const std::invalid_argument&) { rejected = true; }
    REQUIRE(rejected);
}
