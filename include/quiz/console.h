#pragma once
#include "auth_service.h"
#include "console_input.h"
#include <functional>
namespace quiz {
using ConsoleAction = std::function<void(const Session&, std::istream&, std::ostream&)>;
class Console {
public:
    Console(AuthService& auth, ConsoleAction takeQuiz = {}, ConsoleAction summary = {},
        ConsoleAction questionMaintenance = {}, ConsoleAction quizConfiguration = {}, ConsoleAction history = {})
        : auth_(auth), takeQuiz_(std::move(takeQuiz)), summary_(std::move(summary)),
          questionMaintenance_(std::move(questionMaintenance)), quizConfiguration_(std::move(quizConfiguration)),
          history_(std::move(history)) {}
    int run(std::istream& input, std::ostream& output);
private:
    bool authenticatedMenu(Session& session, std::istream& input, std::ostream& output);
    AuthService& auth_;
    ConsoleAction takeQuiz_;
    ConsoleAction summary_;
    ConsoleAction questionMaintenance_;
    ConsoleAction quizConfiguration_;
    ConsoleAction history_;
};
}