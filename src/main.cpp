#include "quiz/console.h"
#include "quiz/question_catalog.h"
#include "quiz/result_service.h"
#include "quiz/answer_session.h"
#include <iostream>
#include <algorithm>
#include <vector>

int main(int argc, char** argv) {
    try {
        std::filesystem::path data = "data";
        bool provision = false;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--data" && i + 1 < argc) data = argv[++i];
            else if (arg == "--init-admin") provision = true;
            else { std::cerr << "Usage: quiz [--data DIRECTORY] [--init-admin]\n"; return 2; }
        }
        quiz::PasswordHasher hasher;
        quiz::FileAccountStore store(data);
        quiz::AuthService auth(store, hasher);
        if (provision) {
            std::cout << "Administrator username (1-128 characters): " << std::flush;
            const auto username = quiz::readLine(std::cin, 128);
            std::cout << "Administrator password (at least 8 characters, at most 1024 bytes): " << std::flush;
            auto password = quiz::readSecret(std::cin);
            if (username.state != quiz::InputState::Value || password.state != quiz::InputState::Value) {
                quiz::eraseSecret(password.value); return 1;
            }
            const auto created = auth.provisionFirstAdmin(username.value, password.value);
            quiz::eraseSecret(password.value);
            if (!created) { std::cerr << "Administrator setup rejected; use a valid empty data directory.\n"; return 1; }
            std::cout << "Administrator account saved.\n";
            return 0;
        }
        if (std::none_of(store.users().begin(), store.users().end(), [](const quiz::User& u) { return u.role == quiz::Role::Admin; })) {
            std::cerr << "Initialize the administrator first: quiz --init-admin [--data DIRECTORY]\n";
            return 1;
        }

        quiz::FileAttemptStore attempts(data);
        quiz::ResultService results(store, attempts);
        quiz::Console console(auth,
            [&](const quiz::Session& session, std::istream& input, std::ostream& output) {
                if (!session.authenticated() || session.isAdmin()) {
                    output << "Quiz access denied.\n";
                    return;
                }
                try {
                    quiz::QuestionCatalog catalog(data);
                    std::vector<quiz::QuestionPrompt> prompts;
                    for (quiz::Id id = 1; id <= static_cast<quiz::Id>(catalog.size()); ++id) {
                        const auto* question = catalog.find(id);
                        if (question) prompts.push_back(*question);
                    }
                    if (prompts.empty()) {
                        output << "No active questions are available.\n";
                        return;
                    }
                    quiz::AnswerSession answerSession(std::move(prompts));
                    output << "Starting quiz with " << answerSession.questionIds().size()
                           << " questions.\n";
                    while (!answerSession.complete()) {
                        if (!answerSession.readNext(input, output)) return;
                    }
                    output << "All answers recorded. Quiz input complete.\n";
                } catch (const std::exception&) {
                    output << "Questions unavailable; check the saved question data and retry.\n";
                }
            },
            [&](const quiz::Session& session, std::istream&, std::ostream& output) {
                try { attempts.reload(); (void)results.displaySummary(session, output); }
                catch (const std::exception&) { output << "Results unavailable; repair saved data.\n"; }
            });
        return console.run(std::cin, std::cout);
    } catch (const std::exception& e) {
        std::cerr << "Unable to open quiz data: " << e.what() << '\n';
        return 1;
    }
}
