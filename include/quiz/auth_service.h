#pragma once
#include "account_store.h"
#include "password_hasher.h"
namespace quiz {
class AuthService {
public:
    AuthService(AccountStore& store, const PasswordHasher& hasher) : store_(store), hasher_(hasher) {}
    Result<Id> registerLearner(const std::string& username, const std::string& password);
    Result<Id> provisionFirstAdmin(const std::string& username, const std::string& password);
private:
    Result<Id> create(const std::string& username, const std::string& password, Role role);
    AccountStore& store_;
    const PasswordHasher& hasher_;
};
}
