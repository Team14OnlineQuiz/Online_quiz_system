#include "quiz/quiz_service.h"
#include "quiz/answer_session.h"
#include "quiz/console_input.h"
#include <charconv>
#include <ctime>
#include <iomanip>
#include <sstream>
namespace quiz {
Result<std::vector<QuizListing>> QuizService::discover(const Session& session) const {
    if (!session.authenticated() || session.isAdmin())
        return failure<std::vector<QuizListing>>(Error::AccessDenied);
    std::vector<QuizListing> result;
    for (const auto& quiz : quizzes_.list())
        result.push_back({quiz.id, quiz.title, quiz.questionIds.size()});
    return success(std::move(result));
}
Result<QuizSnapshot> QuizService::select(const Session& session, Id quizId) const {
    if (!session.authenticated() || session.isAdmin()) return failure<QuizSnapshot>(Error::AccessDenied);
    const auto* quiz = quizzes_.find(quizId);
    if (!quiz) return failure<QuizSnapshot>(Error::Validation);
    QuizSnapshot snapshot{quiz->id, quiz->title, {}};
    snapshot.questions.reserve(quiz->questionIds.size());
    for (Id questionId : quiz->questionIds) {
        const auto* prompt = questions_.find(questionId);
        if (!prompt) return failure<QuizSnapshot>(Error::Validation);
        snapshot.questions.push_back(*prompt);
    }
    return success(std::move(snapshot));
}
Result<std::uint32_t> QuizService::grade(const Session& session, const QuizSnapshot& quiz,
    const std::vector<char>& answers) const {
    if (!session.authenticated() || session.isAdmin())
        return failure<std::uint32_t>(Error::AccessDenied);
    if (quiz.questions.empty() || answers.size() != quiz.questions.size())
        return failure<std::uint32_t>(Error::Validation);
    std::uint32_t correct = 0;
    for (std::size_t i = 0; i < answers.size(); ++i) {
        if (answers[i] < 'A' || answers[i] > 'D')
            return failure<std::uint32_t>(Error::Validation);
        const auto key = questions_.answerForGrading(quiz.questions[i].id);
        if (!key) return failure<std::uint32_t>(Error::Validation);
        if (answers[i] == *key) ++correct;
    }
    return success(correct);
}
std::string currentUtcTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    if (gmtime_s(&utc, &now) != 0) return {};
#else
    if (gmtime_r(&now, &utc) == nullptr) return {};
#endif
    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}
Error runLearnerQuiz(const Session& session, std::istream& input, std::ostream& output,
    const QuizService& quizzes, AttemptService& attempts) {
    const auto available = quizzes.discover(session);
    if (!available) { output << "Access denied.\n"; return available.error; }
    if (available.value->empty()) { output << "No quizzes available.\n"; return Error::None; }
    output << "Available quizzes\n";
    output << "ID | Title | Questions\n";
    for (const auto& quiz : *available.value)
        output << quiz.id << " | " << quiz.title << " | " << quiz.questionCount << '\n';
    while (true) {
        output << "Enter quiz identifier (or press Ctrl+D/Ctrl+Z to cancel): " << std::flush;
        const auto line = readLine(input, 32);
        if (line.state == InputState::End) return Error::Incomplete;
        Id quizId = 0;
        const char* begin = line.value.data();
        const char* end = begin + line.value.size();
        const auto parsed = std::from_chars(begin, end, quizId);
        if (line.state != InputState::Value || parsed.ec != std::errc{} || parsed.ptr != end || quizId == 0) {
            output << "Invalid quiz identifier.\n";
            continue;
        }
        const auto selected = quizzes.select(session, quizId);
        if (!selected) {
            output << "Invalid quiz identifier.\n";
            continue;
        }
        AnswerSession attempt(selected.value->questions);
        while (!attempt.complete()) {
            if (!attempt.readNext(input, output)) {
                output << "Attempt not submitted. No completed attempt was saved.\n";
                return Error::Incomplete;
            }
        }
        const auto graded = quizzes.grade(session, *selected.value, attempt.answers());
        if (!graded) return graded.error;
        const std::uint32_t correct = *graded.value;
        const auto saved = attempts.submit(session, selected.value->title, currentUtcTimestamp(),
            correct, static_cast<std::uint32_t>(selected.value->questions.size()));
        if (!saved) {
            output << "Attempt could not be saved. Please retry; submission was not confirmed.\n";
            return saved.error;
        }
        output << "Attempt saved.\nQuiz: " << selected.value->title << "\nScore: "
            << correct << '/' << selected.value->questions.size() << '\n';
        return Error::None;
    }
}
}