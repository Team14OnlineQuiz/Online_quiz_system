#include "quiz/console.h"
#include "quiz/question_service.h"
#include "quiz/result_service.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

std::optional<quiz::Id> parseQuestionId(const std::string& value) {
    if (value.empty()
        || !std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return c >= '0' && c <= '9';
        })) {
        return std::nullopt;
    }

    try {
        std::size_t consumed = 0;
        const auto id = std::stoull(value, &consumed);

        if (consumed != value.size() || id == 0) {
            return std::nullopt;
        }

        return static_cast<quiz::Id>(id);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

bool readQuestionDraft(
    std::istream& input,
    std::ostream& output,
    quiz::QuestionDraft& draft
) {
    output << "Question prompt: " << std::flush;
    const auto prompt = quiz::readLine(input, 4096);

    if (prompt.state != quiz::InputState::Value) {
        output << "Invalid or missing prompt. Please try again.\n";
        return false;
    }

    draft.prompt = prompt.value;

    for (std::size_t i = 0; i < draft.choices.size(); ++i) {
        output << "Choice " << static_cast<char>('A' + i)
               << ": " << std::flush;

        const auto choice = quiz::readLine(input, 4096);

        if (choice.state != quiz::InputState::Value) {
            output << "Invalid or missing choice. Please try again.\n";
            return false;
        }

        draft.choices[i] = choice.value;
    }

    output << "Correct choice (A-D): " << std::flush;
    const auto correct = quiz::readLine(input, 16);

    if (correct.state != quiz::InputState::Value
        || correct.value.size() != 1
        || correct.value[0] < 'A'
        || correct.value[0] > 'D') {
        output << "Invalid answer key. Enter A, B, C, or D.\n";
        return false;
    }

    draft.correctChoice = correct.value[0];
    return true;
}

void displayQuestions(
    const std::vector<quiz::AdminQuestion>& questions,
    std::ostream& output
) {
    if (questions.empty()) {
        output << "No questions have been created.\n";
        return;
    }

    for (const auto& question : questions) {
        output << "\nQuestion ID: " << question.id
               << "\nStatus: " << (question.active ? "Active" : "Inactive")
               << "\nPrompt: " << question.data.prompt << '\n';

        for (std::size_t i = 0; i < question.data.choices.size(); ++i) {
            output << static_cast<char>('A' + i) << ". "
                   << question.data.choices[i] << '\n';
        }

        output << "Correct answer: " << question.data.correctChoice << "\n";
    }
}

void manageQuestions(
    quiz::QuestionService& service,
    const quiz::Session& session,
    std::istream& input,
    std::ostream& output
) {
    while (true) {
        output << "\nQuestion management\n"
               << "1 List questions\n"
               << "2 Create question\n"
               << "3 Edit question\n"
               << "4 Deactivate question\n"
               << "5 Back\n"
               << "Enter choice (1-5): " << std::flush;

        const auto choice = quiz::readLine(input, 16);

        if (choice.state == quiz::InputState::End) {
            return;
        }

        if (choice.state != quiz::InputState::Value
            || choice.value.size() != 1
            || choice.value < "1"
            || choice.value > "5") {
            output << "Invalid choice. Enter a number from 1 to 5.\n";
            continue;
        }

        if (choice.value == "5") {
            return;
        }

        if (choice.value == "1") {
            const auto questions = service.listQuestions(session);

            if (!questions) {
                output << "Unable to list questions.\n";
            } else {
                displayQuestions(*questions.value, output);
            }

            continue;
        }

        if (choice.value == "2") {
            quiz::QuestionDraft draft{};

            if (!readQuestionDraft(input, output, draft)) {
                if (!input) return;
                continue;
            }

            const auto saved = service.saveQuestion(session, draft);

            if (saved) {
                output << "Question created. ID: " << *saved.value << '\n';
            } else if (saved.error == quiz::Error::Validation) {
                output << "Question rejected. Use a nonempty prompt, "
                          "four nonempty choices, and an answer from A-D.\n";
            } else {
                output << "Question could not be saved. Please retry.\n";
            }

            continue;
        }

        output << "Question ID to "
               << (choice.value == "3" ? "edit" : "deactivate")
               << ": " << std::flush;

        const auto idText = quiz::readLine(input, 32);

        if (idText.state == quiz::InputState::End) {
            return;
        }

        const auto id = idText.state == quiz::InputState::Value
            ? parseQuestionId(idText.value)
            : std::nullopt;

        if (!id) {
            output << "Invalid question ID. Enter a positive numeric ID.\n";
            continue;
        }

        if (choice.value == "3") {
            quiz::QuestionDraft draft{};

            if (!readQuestionDraft(input, output, draft)) {
                if (!input) return;
                continue;
            }

            const auto saved = service.saveQuestion(session, draft, *id);

            if (saved) {
                output << "Question updated.\n";
            } else if (saved.error == quiz::Error::Validation) {
                output << "Question not found or question data is invalid.\n";
            } else {
                output << "Question could not be saved. Please retry.\n";
            }
        } else {
            const auto result = service.deactivateQuestion(session, *id);

            if (result == quiz::Error::None) {
                output << "Question deactivated.\n";
            } else if (result == quiz::Error::Validation) {
                output << "Question ID not found.\n";
            } else {
                output << "Question could not be deactivated.\n";
            }
        }
    }
}

}

int main(int argc, char** argv) {
    try {
        std::filesystem::path data = "data";
        bool provision = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];

            if (arg == "--data" && i + 1 < argc) {
                data = argv[++i];
            } else if (arg == "--init-admin") {
                provision = true;
            } else {
                std::cerr
                    << "Usage: quiz [--data DIRECTORY] [--init-admin]\n";
                return 2;
            }
        }

        quiz::PasswordHasher hasher;
        quiz::FileAccountStore store(data);
        quiz::AuthService auth(store, hasher);

        if (provision) {
            std::cout
                << "Administrator username (1-128 characters): "
                << std::flush;

            const auto username = quiz::readLine(std::cin, 128);

            std::cout
                << "Administrator password (at least 8 characters, "
                   "at most 1024 bytes): "
                << std::flush;

            auto password = quiz::readSecret(std::cin);

            if (username.state != quiz::InputState::Value
                || password.state != quiz::InputState::Value) {
                quiz::eraseSecret(password.value);
                return 1;
            }

            const auto created = auth.provisionFirstAdmin(
                username.value,
                password.value
            );

            quiz::eraseSecret(password.value);

            if (!created) {
                std::cerr
                    << "Administrator setup rejected; use a valid "
                       "empty data directory.\n";
                return 1;
            }

            std::cout << "Administrator account saved.\n";
            return 0;
        }

        if (std::none_of(
            store.users().begin(),
            store.users().end(),
            [](const quiz::User& user) {
                return user.role == quiz::Role::Admin;
            })) {
            std::cerr
                << "Initialize the administrator first: "
                   "quiz --init-admin [--data DIRECTORY]\n";
            return 1;
        }

        quiz::FileAttemptStore attempts(data);
        quiz::ResultService results(store, attempts);
        quiz::QuestionService questions(data);

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
                manageQuestions(questions, session, input, output);
            }
        );

        return console.run(std::cin, std::cout);

    } catch (const std::exception& e) {
        std::cerr << "Unable to open quiz data: " << e.what() << '\n';
        return 1;
    }
}
