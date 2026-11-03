#pragma once

#include "auth_service.h"
#include "console_input.h"

#include <functional>

namespace quiz {

using ConsoleAction = std::function<void(
    const Session&,
    std::istream&,
    std::ostream&
)>;

class Console {
public:
    Console(
        AuthService& auth,
        ConsoleAction takeQuiz = {},
        ConsoleAction summary = {},
        ConsoleAction manageQuestions = {}
    )
        : auth_(auth),
          takeQuiz_(std::move(takeQuiz)),
          summary_(std::move(summary)),
          manageQuestions_(std::move(manageQuestions)) {}

    int run(std::istream& input, std::ostream& output);

private:
    bool authenticatedMenu(
        Session& session,
        std::istream& input,
        std::ostream& output
    );

    AuthService& auth_;
    ConsoleAction takeQuiz_;
    ConsoleAction summary_;
    ConsoleAction manageQuestions_;
};

}
