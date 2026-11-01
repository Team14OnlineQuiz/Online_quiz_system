#include "quiz/account_store.h"
#include "quiz/password_hasher.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace quiz {
bool validUsername(const std::string& value) {
    return !value.empty() && value.size() <= 128
        && value.find_first_not_of(' ') != std::string::npos
        && std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 32 || c == 127; });
}
void replaceFile(const std::filesystem::path& staged, const std::filesystem::path& destination) {
#ifdef _WIN32
    if (!MoveFileExW(staged.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Unable to commit data file");
#else
    std::filesystem::rename(staged, destination);
#endif
}
FileAccountStore::FileAccountStore(std::filesystem::path directory) {
    std::filesystem::create_directories(directory);
    file_ = std::move(directory) / "users.db";
    if (!std::filesystem::exists(file_)) return;
    std::ifstream input(file_);
    if (!input) throw std::runtime_error("Cannot read users.db; repair the data directory");
    std::string line;
    if (!std::getline(input, line) || line != "QUIZ_USERS_V1")
        throw std::runtime_error("Invalid users.db header; restore a valid backup");
    std::unordered_set<Id> ids;
    std::unordered_set<std::string> names;
    while (std::getline(input, line)) {
        if (line.size() > 2048) throw std::runtime_error("Invalid account record");
        User user{};
        int role = -1;
        std::istringstream record(line);
        if (!(record >> user.id >> role >> std::quoted(user.username) >> std::quoted(user.passwordHash)))
            throw std::runtime_error("Malformed account record; restore a valid backup");
        record >> std::ws;
        if (!record.eof() || user.id == 0 || (role != 0 && role != 1) || !validUsername(user.username)
            || !PasswordHasher::validEncoding(user.passwordHash) || !ids.insert(user.id).second
            || !names.insert(user.username).second)
            throw std::runtime_error("Invalid account record; restore a valid backup");
        user.role = role == 1 ? Role::Admin : Role::Learner;
        users_.push_back(std::move(user));
    }
    if (input.bad()) throw std::runtime_error("Account read failed");
}
const User* FileAccountStore::find(const std::string& username) const {
    const auto it = std::find_if(users_.begin(), users_.end(), [&](const User& u) { return u.username == username; });
    return it == users_.end() ? nullptr : &*it;
}
Result<Id> FileAccountStore::insert(std::string username, std::string hash, Role role) {
    if (!validUsername(username) || !PasswordHasher::validEncoding(hash)) return failure<Id>(Error::Validation);
    if (find(username)) return failure<Id>(Error::DuplicateUser);
    Id id = 1;
    for (const auto& user : users_) {
        if (user.id == std::numeric_limits<Id>::max()) return failure<Id>(Error::Storage);
        id = std::max(id, user.id + 1);
    }
    auto stagedUsers = users_;
    stagedUsers.push_back({id, std::move(username), std::move(hash), role});
    auto stagedFile = file_;
    stagedFile += ".tmp";
    try {
        std::ofstream output(stagedFile, std::ios::trunc);
        if (!output) return failure<Id>(Error::Storage);
#ifndef _WIN32
        std::filesystem::permissions(stagedFile, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
#endif
        output << "QUIZ_USERS_V1\n";
        for (const auto& user : stagedUsers)
            output << user.id << ' ' << (user.role == Role::Admin ? 1 : 0) << ' '
                << std::quoted(user.username) << ' ' << std::quoted(user.passwordHash) << '\n';
        output.flush();
        if (!output) return failure<Id>(Error::Storage);
        output.close();
        if (!output) return failure<Id>(Error::Storage);
        replaceFile(stagedFile, file_);
        users_.swap(stagedUsers);
        return success(id);
    } catch (const std::exception&) { return failure<Id>(Error::Storage); }
}
}
