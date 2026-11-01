#include "quiz/admin_console.h"
#include "quiz/console.h"
#include "quiz/quiz_service.h"
#include "quiz/result_service.h"
#include <algorithm>
#include <iostream>
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
        if (std::none_of(store.users().begin(), store.users().end(),
            [](const quiz::User& user) { return user.role == quiz::Role::Admin; })) {
            std::cerr << "Initialize the administrator first: quiz --init-admin [--data DIRECTORY]\n";
            return 1;
        }
        quiz::QuestionCatalog questions(data);
        quiz::QuizCatalog quizCatalog(data, questions);
        quiz::FileAttemptStore attempts(data);
        quiz::AttemptService attemptService(attempts);
        quiz::QuizService quizService(quizCatalog, questions);
        quiz::ResultService results(store, attempts);
        quiz::LearnerHistoryService history(store, attempts);
        quiz::Console console(auth,
            [&](const quiz::Session& session, std::istream& input, std::ostream& output) {
                (void)quiz::runLearnerQuiz(session, input, output, quizService, attemptService);
            },
            [&](const quiz::Session& session, std::istream&, std::ostream& output) {
                try { attempts.reload(); (void)results.displaySummary(session, output); }
                catch (const std::exception&) { output << "Results unavailable; repair saved data.\n"; }
            },
            [&](const quiz::Session& session, std::istream& input, std::ostream& output) {
                quiz::runQuestionMaintenance(session, questions, input, output);
            },
            [&](const quiz::Session& session, std::istream& input, std::ostream& output) {
                quiz::runQuizConfiguration(session, quizCatalog, questions, input, output);
            },
            [&](const quiz::Session& session, std::istream&, std::ostream& output) {
                try { attempts.reload(); (void)history.displayHistory(session, output); }
                catch (const std::exception&) { output << "History unavailable; repair saved data.\n"; }
            });
        return console.run(std::cin, std::cout);
    } catch (const std::exception& e) {
        std::cerr << "Unable to open quiz data: " << e.what() << '\n';
        return 1;
    }
}