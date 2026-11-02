#include "quiz/console.h"
#include "quiz/result_service.h"
#include "quiz/quiz_service.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

int main(int argc, char** argv) {
    try {
        std::filesystem::path data = "data";
        bool provision = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--data" && i + 1 < argc) data = argv[++i];
            else if (arg == "--init-admin") provision = true;
            else {
                std::cerr << "Usage: quiz [--data DIRECTORY] [--init-admin]\n";
                return 2;
            }
        }

        quiz::PasswordHasher hasher;
        quiz::FileAccountStore store(data);
        quiz::AuthService auth(store, hasher);

        if (provision) {
            std::cout << "Administrator username (1-128 characters): " << std::flush;
            const auto username = quiz::readLine(std::cin, 128);
            std::cout << "Administrator password (at least 8 characters, at most 1024 bytes): " << std::flush;
            auto password = quiz::readSecret(std::cin);

            if (username.state != quiz::InputState::Value ||
                password.state != quiz::InputState::Value) {
                quiz::eraseSecret(password.value);
                return 1;
            }

            const auto created = auth.provisionFirstAdmin(username.value, password.value);
            quiz::eraseSecret(password.value);

            if (!created) {
                std::cerr << "Administrator setup rejected; use a valid empty data directory.\n";
                return 1;
            }

            std::cout << "Administrator account saved.\n";
            return 0;
        }

        if (std::none_of(store.users().begin(), store.users().end(),
            [](const quiz::User& u) { return u.role == quiz::Role::Admin; })) {
            std::cerr << "Initialize the administrator first: quiz --init-admin [--data DIRECTORY]\n";
            return 1;
        }

        quiz::FileAttemptStore attempts(data);
        quiz::ResultService results(store, attempts);
        quiz::QuizService quizzes(data);

        quiz::Console console(
            auth,
            {},
            [&](const quiz::Session& session, std::istream&, std::ostream& output) {
                try {
                    attempts.reload();
                    (void)results.displaySummary(session, output);
                } catch (const std::exception&) {
                    output << "Results unavailable; repair saved data.\n";
                }
            },
            [&](const quiz::Session& session, std::istream& input, std::ostream& output) {
                auto listed = quizzes.listQuizzes(session);
                if (!listed) {
                    output << "Unable to list quizzes.\n";
                    return;
                }

                output << "\nSaved quizzes:\n";
                if (listed.value->empty()) output << "No quizzes created yet.\n";

                for (const auto& quiz : *listed.value) {
                    output << "ID " << quiz.id << ": " << quiz.title << " ("
                           << quiz.questionIds.size() << " questions)\n";
                }

                output << "\n1 Create quiz\n2 Edit quiz\n3 Return\nEnter choice (1-3): " << std::flush;
                const auto action = quiz::readLine(input, 16);

                if (action.state != quiz::InputState::Value ||
                    (action.value != "1" && action.value != "2")) {
                    if (action.state == quiz::InputState::Value && action.value == "3")
                        return;
                    output << "Invalid choice. Enter 1, 2, or 3.\n";
                    return;
                }

                std::optional<quiz::Id> editId;
                if (action.value == "2") {
                    output << "Quiz ID to edit: " << std::flush;
                    const auto idLine = quiz::readLine(input, 32);
                    if (idLine.state != quiz::InputState::Value) {
                        output << "Invalid quiz ID.\n";
                        return;
                    }

                    std::istringstream parser(idLine.value);
                    quiz::Id id = 0;
                    char extra = '\0';
                    if (!(parser >> id) || (parser >> extra) || id == 0) {
                        output << "Invalid quiz ID.\n";
                        return;
                    }
                    editId = id;
                }

                output << "Quiz title: " << std::flush;
                const auto title = quiz::readLine(input, 256);
                if (title.state != quiz::InputState::Value) {
                    output << "Invalid title. Please try again.\n";
                    return;
                }

                output << "Active question IDs separated by spaces: " << std::flush;
                const auto idsLine = quiz::readLine(input, 4096);
                if (idsLine.state != quiz::InputState::Value) {
                    output << "Invalid question list. Please try again.\n";
                    return;
                }

                std::istringstream parser(idsLine.value);
                std::vector<quiz::Id> questionIds;
                quiz::Id questionId = 0;
                while (parser >> questionId) questionIds.push_back(questionId);

                if (!parser.eof()) {
                    output << "Invalid question IDs. Use positive numbers separated by spaces.\n";
                    return;
                }

                const auto saved = quizzes.saveQuiz(
                    session, {title.value, questionIds}, editId);

                if (saved)
                    output << "Quiz saved successfully. Quiz ID: " << *saved.value << "\n";
                else if (saved.error == quiz::Error::AccessDenied)
                    output << "Only administrators can manage quizzes.\n";
                else if (saved.error == quiz::Error::Validation)
                    output << "Quiz rejected. Check the title and ensure every ID refers to a distinct active question.\n";
                else
                    output << "Quiz could not be saved. Check the data directory and try again.\n";
            }
        );

        return console.run(std::cin, std::cout);
    } catch (const std::exception& e) {
        std::cerr << "Unable to open quiz data: " << e.what() << '\n';
        return 1;
    }
}
