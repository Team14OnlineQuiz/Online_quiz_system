#include "quiz/console.h"
#include "quiz/quiz_service.h"
#include "quiz/result_service.h"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace {
bool readValue(std::istream& in, std::ostream& out, const std::string& prompt,
               std::string& value, std::size_t limit = 4096) {
    while (true) {
        out << prompt << std::flush;
        const auto line = quiz::readLine(in, limit);
        if (line.state == quiz::InputState::End) return false;
        if (line.state == quiz::InputState::Invalid) {
            out << "Input is too long. Please retry within " << limit << " characters.\n";
            continue;
        }
        value = line.value;
        return true;
    }
}
bool parseId(const std::string& text, quiz::Id& id) {
    try {
        std::size_t used = 0;
        const auto value = std::stoull(text, &used);
        if (used != text.size() || value == 0) return false;
        id = static_cast<quiz::Id>(value);
        return true;
    } catch (...) { return false; }
}
void manageQuestions(const quiz::Session& session, std::istream& in, std::ostream& out,
                     quiz::QuizService& service) {
    while (true) {
        out << "\nQuestion maintenance\n1 Create question\n2 Edit question\n3 Deactivate question\n4 Back\n";
        std::string choice;
        if (!readValue(in, out, "Enter choice (1-4): ", choice, 16)) return;
        if (choice == "4") return;
        if (choice == "3") {
            std::string raw;
            quiz::Id id = 0;
            if (!readValue(in, out, "Question ID: ", raw, 32)) return;
            if (!parseId(raw, id)) { out << "Enter a positive numeric question ID.\n"; continue; }
            const auto result = service.deactivateQuestion(session, id);
            out << (result == quiz::Error::None ? "Question deactivated.\n" :
                    result == quiz::Error::AccessDenied ? "Access denied.\n" :
                    result == quiz::Error::Storage ? "Question could not be saved; retry.\n" :
                    "Question ID was not found.\n");
            continue;
        }
        if (choice != "1" && choice != "2") {
            out << "Invalid choice. Enter 1, 2, 3, or 4.\n"; continue;
        }
        std::optional<quiz::Id> editId;
        if (choice == "2") {
            std::string raw;
            quiz::Id id = 0;
            if (!readValue(in, out, "Question ID to edit: ", raw, 32)) return;
            if (!parseId(raw, id)) { out << "Enter a positive numeric question ID.\n"; continue; }
            editId = id;
        }
        quiz::QuestionDraft draft;
        if (!readValue(in, out, "Question prompt: ", draft.prompt)) return;
        bool complete = true;
        for (std::size_t i = 0; i < 4; ++i) {
            if (!readValue(in, out, std::string("Choice ") + static_cast<char>('A' + i) + ": ",
                           draft.choices[i])) { complete = false; break; }
        }
        if (!complete) return;
        std::string key;
        if (!readValue(in, out, "Correct choice (A-D): ", key, 8)) return;
        draft.correctChoice = key.size() == 1 ? key[0] : '?';
        const auto saved = service.saveQuestion(session, draft, editId);
        if (saved) out << "Question saved. ID: " << *saved.value << '\n';
        else if (saved.error == quiz::Error::AccessDenied) out << "Access denied.\n";
        else if (saved.error == quiz::Error::Storage) out << "Question could not be saved; retry.\n";
        else out << "Question rejected. Enter a nonempty prompt, four nonempty choices, and one correct letter A-D.\n";
    }
}
void manageQuizzes(const quiz::Session& session, std::istream& in, std::ostream& out,
                   quiz::QuizService& service) {
    while (true) {
        out << "\nQuiz configuration\n1 Create quiz\n2 Edit quiz\n3 Back\n";
        std::string choice;
        if (!readValue(in, out, "Enter choice (1-3): ", choice, 16)) return;
        if (choice == "3") return;
        if (choice != "1" && choice != "2") {
            out << "Invalid choice. Enter 1, 2, or 3.\n"; continue;
        }
        std::optional<quiz::Id> editId;
        if (choice == "2") {
            std::string raw;
            quiz::Id id = 0;
            if (!readValue(in, out, "Quiz ID to edit: ", raw, 32)) return;
            if (!parseId(raw, id)) { out << "Enter a positive numeric quiz ID.\n"; continue; }
            editId = id;
        }
        quiz::QuizDraft draft;
        if (!readValue(in, out, "Quiz title: ", draft.title, 512)) return;
        std::string countText;
        if (!readValue(in, out, "Number of active questions: ", countText, 16)) return;
        std::size_t count = 0;
        try {
            std::size_t used = 0;
            count = std::stoul(countText, &used);
            if (used != countText.size() || count == 0 || count > 10000) throw std::invalid_argument("count");
        } catch (...) { out << "Enter a number from 1 to 10000.\n"; continue; }
        bool complete = true;
        for (std::size_t i = 0; i < count; ++i) {
            std::string raw;
            quiz::Id id = 0;
            if (!readValue(in, out, "Question ID " + std::to_string(i + 1) + ": ", raw, 32)) {
                complete = false; break;
            }
            if (!parseId(raw, id)) { complete = false; break; }
            draft.questionIds.push_back(id);
        }
        if (!complete) { out << "Question IDs must be positive numbers; configuration cancelled.\n"; continue; }
        const auto saved = service.saveQuiz(session, draft, editId);
        if (saved) out << "Quiz saved. ID: " << *saved.value << '\n';
        else if (saved.error == quiz::Error::AccessDenied) out << "Access denied.\n";
        else if (saved.error == quiz::Error::UnknownQuestion)
            out << "Quiz rejected. Every question ID must refer to an existing active question.\n";
        else if (saved.error == quiz::Error::Storage) out << "Quiz could not be saved; retry.\n";
        else out << "Quiz rejected. Use a nonempty title, at least one question, and no duplicate question IDs.\n";
    }
}
void browseAndTakeQuiz(const quiz::Session& session, std::istream& in, std::ostream& out,
                       quiz::QuizService& service) {
    const auto quizzes = service.listQuizzes(session);
    if (!quizzes) { out << "Access denied.\n"; return; }
    if (quizzes.value->empty()) { out << "No quizzes available.\n"; return; }
    out << "Available quizzes\n";
    for (const auto& item : *quizzes.value)
        out << item.id << ". " << item.title << " (" << item.questionCount << " questions)\n";
    std::string raw;
    if (!readValue(in, out, "Enter quiz ID to start (or 0 to cancel): ", raw, 32)) return;
    if (raw == "0") return;
    quiz::Id id = 0;
    if (!parseId(raw, id)) { out << "Enter one of the displayed quiz IDs.\n"; return; }
    const auto result = service.takeQuiz(session, id, in, out);
    if (!result) {
        if (result.error == quiz::Error::Incomplete) out << "Quiz not submitted; your answers were not saved.\n";
        else if (result.error == quiz::Error::QuizUnavailable) out << "That quiz is unavailable.\n";
        else if (result.error == quiz::Error::Storage) out << "Quiz data or result storage is unavailable.\n";
        else if (result.error == quiz::Error::AccessDenied) out << "Access denied.\n";
    }
}
}
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
        if (std::none_of(store.users().begin(), store.users().end(), [](const quiz::User& u) {
                return u.role == quiz::Role::Admin;
            })) {
            std::cerr << "Initialize the administrator first: quiz --init-admin [--data DIRECTORY]\n";
            return 1;
        }
        quiz::QuizService quizzes(data);
        quiz::FileAttemptStore attempts(data);
        quiz::ResultService results(store, attempts);
        quiz::Console console(
            auth,
            [&](const quiz::Session& session, std::istream& input, std::ostream& output) {
                browseAndTakeQuiz(session, input, output, quizzes);
            },
            [&](const quiz::Session& session, std::istream&, std::ostream& output) {
                try { attempts.reload(); (void)results.displaySummary(session, output); }
                catch (const std::exception&) { output << "Results unavailable; repair saved data.\n"; }
            },
            [&](const quiz::Session& session, std::istream& input, std::ostream& output) {
                manageQuestions(session, input, output, quizzes);
            },
            [&](const quiz::Session& session, std::istream& input, std::ostream& output) {
                manageQuizzes(session, input, output, quizzes);
            });
        return console.run(std::cin, std::cout);
    } catch (const std::exception& e) {
        std::cerr << "Unable to open quiz data: " << e.what() << '\n';
        return 1;
    }
}
