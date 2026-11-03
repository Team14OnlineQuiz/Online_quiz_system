#include "quiz/admin_console.h"
#include "quiz/console_input.h"
#include <charconv>
#include <sstream>
namespace quiz {
namespace {
bool readId(std::istream& input, std::ostream& output, const char* prompt, Id& value) {
    output << prompt << std::flush;
    const auto line = readLine(input, 32);
    if (line.state != InputState::Value) return false;
    const auto parsed = std::from_chars(line.value.data(), line.value.data() + line.value.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == line.value.data() + line.value.size() && value != 0;
}
bool readQuestionIds(std::istream& input, std::ostream& output, std::vector<Id>& ids) {
    output << "Question identifiers separated by spaces (for example: 1 2 3): " << std::flush;
    const auto line = readLine(input, 512);
    if (line.state != InputState::Value) return false;
    std::istringstream values(line.value);
    long long id = 0;
    while (values >> id) {
        if (id <= 0) return false;
        ids.push_back(static_cast<Id>(id));
    }
    return !ids.empty() && values.eof();
}
void showError(std::ostream& output, Error error) {
    if (error == Error::None) output << "Changes saved.\n";
    else if (error == Error::AccessDenied) output << "Access denied.\n";
    else if (error == Error::Storage) output << "Changes could not be saved. Existing data was retained.\n";
    else output << "Invalid data or identifier.\n";
}
}
void runQuizConfiguration(const Session& session, QuizCatalog& quizzes,
    const QuestionCatalog& questions, std::istream& input, std::ostream& output) {
    if (!session.isAdmin()) { output << "Access denied.\n"; return; }
    while (true) {
        output << "Quiz configuration\n1 List quizzes\n2 Create quiz\n3 Modify quiz\n4 Archive quiz\n5 Back\n";
        output << "Enter choice (1-5): " << std::flush;
        const auto choice = readLine(input, 8);
        if (choice.state != InputState::Value || choice.value == "5") return;
        if (choice.value == "1") {
            const auto list = quizzes.list(session);
            if (list.empty()) output << "No quizzes configured.\n";
            for (const auto& quiz : list) {
                output << quiz.id << ": " << quiz.title << " (" << quiz.questionIds.size() << " questions)\n";
            }
            output << "Active question identifiers:";
            for (const auto& question : questions.list(session)) output << ' ' << question.id;
            output << '\n';
        } else if (choice.value == "2" || choice.value == "3") {
            Id id = 0;
            if (choice.value == "3" && !readId(input, output, "Quiz identifier: ", id)) {
                output << "Invalid quiz identifier.\n"; continue;
            }
            output << "Quiz title (1-120 characters): " << std::flush;
            const auto title = readLine(input, 120);
            std::vector<Id> questionIds;
            if (title.state != InputState::Value || !readQuestionIds(input, output, questionIds)) {
                output << "Quiz input cancelled or invalid.\n"; continue;
            }
            if (choice.value == "2") {
                const auto added = quizzes.add(session, questions, title.value, std::move(questionIds));
                showError(output, added ? Error::None : added.error);
                if (added) output << "Quiz identifier: " << *added.value << '\n';
            } else showError(output, quizzes.modify(session, questions, id, title.value, std::move(questionIds)));
        } else if (choice.value == "4") {
            Id id = 0;
            if (!readId(input, output, "Quiz identifier: ", id)) {
                output << "Invalid quiz identifier.\n"; continue;
            }
            output << "Archive quiz " << id << "? (y/N): " << std::flush;
            const auto confirm = readLine(input, 8);
            if (confirm.state == InputState::Value && confirm.value == "y")
                showError(output, quizzes.remove(session, id));
            else output << "Cancelled.\n";
        } else output << "Invalid choice.\n";
    }
}
}