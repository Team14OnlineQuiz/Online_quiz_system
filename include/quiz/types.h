#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace quiz {
using Id = std::uint64_t;
enum class Role { Learner, Admin };
enum class Error { None, Validation, DuplicateUser, Storage, Authentication, AccessDenied, Incomplete };
template<class T> struct Result {
    std::optional<T> value;
    Error error = Error::None;
    explicit operator bool() const { return value.has_value(); }
};
template<class T> Result<T> success(T value) { return {std::move(value), Error::None}; }
template<class T> Result<T> failure(Error error) { return {std::nullopt, error}; }
struct User { Id id; std::string username; std::string passwordHash; Role role; };
class AuthService;
class Session {
public:
    Session() = default;
    bool authenticated() const { return authenticated_; }
    bool isAdmin() const { return authenticated_ && role_ == Role::Admin; }
    Id userId() const { return id_; }
    const std::string& username() const { return username_; }
private:
    friend class AuthService;
    Session(Id id, std::string username, Role role)
        : id_(id), username_(std::move(username)), role_(role), authenticated_(true) {}
    Id id_ = 0;
    std::string username_;
    Role role_ = Role::Learner;
    bool authenticated_ = false;
};
}
