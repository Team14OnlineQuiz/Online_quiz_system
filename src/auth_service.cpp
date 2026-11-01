#include "quiz/auth_service.h"
#include <exception>
namespace quiz {
Result<Id> AuthService::create(const std::string& username, const std::string& password, Role role) {
    if (!validUsername(username) || password.size() < 8 || password.size() > 1024)
        return failure<Id>(Error::Validation);
    if (store_.find(username)) return failure<Id>(Error::DuplicateUser);
    try { return store_.insert(username, hasher_.hash(password), role); }
    catch (const std::exception&) { return failure<Id>(Error::Storage); }
}
Result<Id> AuthService::registerLearner(const std::string& username, const std::string& password) {
    return create(username, password, Role::Learner);
}
Result<Id> AuthService::provisionFirstAdmin(const std::string& username, const std::string& password) {
    if (!store_.users().empty()) return failure<Id>(Error::AccessDenied);
    return create(username, password, Role::Admin);
}
Result<Session> AuthService::signIn(const std::string& username, const std::string& password) const {
    const auto* user = store_.find(username);
    if (!user || !hasher_.verify(password, user->passwordHash))
        return failure<Session>(Error::Authentication);
    return success(Session(user->id, user->username, user->role));
}
}
