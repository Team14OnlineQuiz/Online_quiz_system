#include "quiz/console.h"

namespace quiz {

bool Console::authenticatedMenu(
    Session& session,
    std::istream& input,
    std::ostream& output
) {
    while (session.authenticated()) {
        if (session.isAdmin()) {
            output << "Administrator menu\n"
                   << "1 Manage questions\n"
                   << "2 Results summary\n"
                   << "3 Logout\n";
            output << "Enter choice (1-3): " << std::flush;
        } else {
            output << "Learner menu\n"
                   << "1 Take quiz\n"
                   << "2 Logout\n";
            output << "Enter choice (1-2): " << std::flush;
        }

        const auto choice = readLine(input, 16);

        if (choice.state == InputState::End) {
            auth_.signOut(session);
            return false;
        }

        if (session.isAdmin()) {
            if (choice.state == InputState::Value && choice.value == "3") {
                auth_.signOut(session);
                break;
            }

            if (choice.state != InputState::Value
                || (choice.value != "1" && choice.value != "2")) {
                output << "Invalid choice. Enter 1, 2, or 3.\n";
                continue;
            }

            if (choice.value == "1") {
                if (manageQuestions_) {
                    manageQuestions_(session, input, output);
                } else {
                    output << "Question management unavailable.\n";
                }
            } else {
                if (summary_) {
                    summary_(session, input, output);
                } else {
                    output << "No completed attempts.\n";
                }
            }
        } else {
            if (choice.state == InputState::Value && choice.value == "2") {
                auth_.signOut(session);
                break;
            }

            if (choice.state != InputState::Value || choice.value != "1") {
                output << "Invalid choice. Enter 1 or 2.\n";
                continue;
            }

            if (takeQuiz_) {
                takeQuiz_(session, input, output);
            } else {
                output << "No quizzes available.\n";
            }
        }
    }

    return true;
}

int Console::run(std::istream& input, std::ostream& output) {
    while (true) {
        output << "Online Quiz System\n"
               << "1 Register learner\n"
               << "2 Sign in\n"
               << "3 Exit\n"
               << "Enter choice (1-3): " << std::flush;

        const auto choice = readLine(input, 16);

        if (choice.state == InputState::End
            || (choice.state == InputState::Value && choice.value == "3")) {
            return 0;
        }

        if (choice.state != InputState::Value
            || (choice.value != "1" && choice.value != "2")) {
            output << "Invalid choice. Enter 1, 2, or 3.\n";
            continue;
        }

        output << "Username (1-128 characters): " << std::flush;
        const auto username = readLine(input, 128);

        output << "Password (at least 8 characters, at most 1024 bytes): "
               << std::flush;
        auto password = readSecret(input);

        if (password.state == InputState::End) {
            return 0;
        }

        if (username.state == InputState::Invalid
            || password.state == InputState::Invalid) {
            eraseSecret(password.value);
            output << (choice.value == "2"
                ? "Invalid credentials.\n"
                : "Invalid username or password length.\n");
            continue;
        }

        if (choice.value == "1") {
            const auto registered = auth_.registerLearner(
                username.value,
                password.value
            );

            eraseSecret(password.value);

            if (registered) {
                output << "Learner account saved.\n";
            } else if (registered.error == Error::DuplicateUser) {
                output << "Username already exists.\n";
            } else if (registered.error == Error::Storage) {
                output << "Account could not be saved. Please retry.\n";
            } else {
                output << "Use a nonempty username and a password of at least 8 characters.\n";
            }
        } else {
            auto signedIn = auth_.signIn(username.value, password.value);
            eraseSecret(password.value);

            if (!signedIn) {
                output << "Invalid credentials.\n";
                continue;
            }

            if (!authenticatedMenu(*signedIn.value, input, output)) {
                return 0;
            }
        }
    }
}

}
