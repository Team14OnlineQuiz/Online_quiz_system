#include "quiz/password_hasher.h"
#include <sodium.h>
#include <stdexcept>

namespace quiz {
PasswordHasher::PasswordHasher() {
    if (sodium_init() < 0) throw std::runtime_error("Password hashing initialization failed");
}
std::string PasswordHasher::hash(std::string_view password) const {
    if (password.size() < 8 || password.size() > 1024)
        throw std::invalid_argument("Password must contain 8 to 1024 bytes");
    char encoded[crypto_pwhash_STRBYTES]{};
    if (crypto_pwhash_str_alg(encoded, password.data(), password.size(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_ARGON2ID13) != 0)
        throw std::runtime_error("Not enough resources to protect password");
    return encoded;
}
bool PasswordHasher::verify(std::string_view password, const std::string& encoded) const {
    if (password.size() > 1024 || !validEncoding(encoded)) return false;
    return crypto_pwhash_str_verify(encoded.c_str(), password.data(), password.size()) == 0;
}
bool PasswordHasher::validEncoding(const std::string& encoded) {
    return encoded.size() < crypto_pwhash_STRBYTES && encoded.find('\0') == std::string::npos
        && encoded.rfind("$argon2id$", 0) == 0
        && crypto_pwhash_str_needs_rehash(encoded.c_str(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != -1;
}
}
