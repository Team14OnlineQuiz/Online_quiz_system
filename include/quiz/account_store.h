#pragma once
#include "types.h"
#include <filesystem>
#include <vector>
namespace quiz {
class AccountStore {
public:
    virtual ~AccountStore() = default;
    virtual const User* find(const std::string& username) const = 0;
    virtual const std::vector<User>& users() const = 0;
    virtual Result<Id> insert(std::string username, std::string hash, Role role) = 0;
};
class FileAccountStore final : public AccountStore {
public:
    explicit FileAccountStore(std::filesystem::path directory);
    const User* find(const std::string& username) const override;
    const std::vector<User>& users() const override { return users_; }
    Result<Id> insert(std::string username, std::string hash, Role role) override;
private:
    std::filesystem::path file_;
    std::vector<User> users_;
};
bool validUsername(const std::string& value);
// Platform-dependent replacement belongs in the repository adapter only.
void replaceFile(const std::filesystem::path& staged, const std::filesystem::path& destination);
}
