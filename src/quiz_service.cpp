#include "quiz/quiz_service.h"
#include "quiz/account_store.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace quiz {
namespace {
template<class T> Id nextIdentifier(const std::vector<T>& records) {
    Id next = 1;
    for (const auto& record : records) {
        if (record.id >= static_cast<Id>(std::numeric_limits<long long>::max())) return 0;
        next = std::max(next, record.id + 1);
    }
    return next;
}
Error writeCommitted(const std::filesystem::path& file, const std::string& contents) {
    auto staged = file;
    staged += ".tmp";
    try {
        {
            std::ofstream out(staged, std::ios::binary | std::ios::trunc);
            if (!out) return Error::Storage;
            out << contents;
            out.flush();
            if (!out) return Error::Storage;
        }
        replaceFile(staged, file);
        return Error::None;
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(staged, ignored);
        return Error::Storage;
    }
}
std::string utcNow() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t value = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &value);
#else
    gmtime_r(&value, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}
}
bool QuizService::validText(const std::string& value, std::size_t limit) {
    return !value.empty() && value.size() <= limit
        && value.find_first_not_of(" \t\r\n") != std::string::npos
        && std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 32 || c == 127; });
}
Id QuizService::nextId(const std::vector<Question>& values) { return nextIdentifier(values); }
Id QuizService::nextQuizId(const std::vector<Quiz>& values) { return nextIdentifier(values); }

QuizService::QuizService(std::filesystem::path directory)
    : directory_(std::move(directory)), questionsFile_(directory_ / "questions.db"),
      quizzesFile_(directory_ / "quizzes.db"), attemptsFile_(directory_ / "attempts.db") {
    std::filesystem::create_directories(directory_);
    loadQuestions();
    loadQuizzes();
}
void QuizService::loadQuestions() {
    if (!std::filesystem::exists(questionsFile_)) return;
    std::ifstream in(questionsFile_);
    std::string line;
    if (!in || !std::getline(in, line) || line != "QUIZ_QUESTIONS_V1")
        throw std::runtime_error("Invalid questions.db; restore a valid backup");
    std::vector<Question> staged;
    std::unordered_set<Id> ids;
    while (std::getline(in, line)) {
        if (line.size() > 21000) throw std::runtime_error("Oversized question record");
        long long rawId = 0;
        int active = -1;
        Question q{};
        std::istringstream row(line);
        if (!(row >> rawId >> active >> q.data.correctChoice >> std::quoted(q.data.prompt)))
            throw std::runtime_error("Malformed question record");
        for (auto& choice : q.data.choices)
            if (!(row >> std::quoted(choice))) throw std::runtime_error("Malformed question choices");
        row >> std::ws;
        if (!row.eof() || rawId <= 0 || (active != 0 && active != 1)
            || q.data.correctChoice < 'A' || q.data.correctChoice > 'D'
            || !validText(q.data.prompt, 4096)
            || !std::all_of(q.data.choices.begin(), q.data.choices.end(),
                            [](const std::string& s) { return validText(s, 4096); })
            || !ids.insert(static_cast<Id>(rawId)).second)
            throw std::runtime_error("Invalid question record");
        q.id = static_cast<Id>(rawId);
        q.active = active == 1;
        staged.push_back(std::move(q));
    }
    if (in.bad()) throw std::runtime_error("Question read failed");
    questions_.swap(staged);
}
void QuizService::loadQuizzes() {
    if (!std::filesystem::exists(quizzesFile_)) return;
    std::ifstream in(quizzesFile_);
    std::string line;
    if (!in || !std::getline(in, line) || line != "QUIZ_QUIZZES_V1")
        throw std::runtime_error("Invalid quizzes.db; restore a valid backup");
    std::vector<Quiz> staged;
    std::unordered_set<Id> ids;
    while (std::getline(in, line)) {
        if (line.size() > 65536) throw std::runtime_error("Oversized quiz record");
        long long rawId = 0, count = 0;
        Quiz q{};
        std::istringstream row(line);
        if (!(row >> rawId >> std::quoted(q.title) >> count) || rawId <= 0 || count <= 0 || count > 10000)
            throw std::runtime_error("Malformed quiz record");
        q.id = static_cast<Id>(rawId);
        if (!validText(q.title, 512) || !ids.insert(q.id).second)
            throw std::runtime_error("Invalid quiz record");
        std::unordered_set<Id> questionIds;
        for (long long i = 0, questionId = 0; i < count; ++i) {
            if (!(row >> questionId) || questionId <= 0
                || !questionIds.insert(static_cast<Id>(questionId)).second)
                throw std::runtime_error("Invalid quiz question reference");
            const auto found = std::find_if(questions_.begin(), questions_.end(), [&](const Question& qn) {
                return qn.id == static_cast<Id>(questionId);
            });
            if (found == questions_.end()) throw std::runtime_error("Quiz references an unknown question");
            q.questionIds.push_back(static_cast<Id>(questionId));
        }
        row >> std::ws;
        if (!row.eof()) throw std::runtime_error("Unexpected quiz record data");
        staged.push_back(std::move(q));
    }
    if (in.bad()) throw std::runtime_error("Quiz read failed");
    quizzes_.swap(staged);
}
Error QuizService::saveQuestions(const std::vector<Question>& values) const {
    std::ostringstream out;
    out << "QUIZ_QUESTIONS_V1\n";
    for (const auto& q : values) {
        out << q.id << ' ' << (q.active ? 1 : 0) << ' ' << q.data.correctChoice << ' '
            << std::quoted(q.data.prompt);
        for (const auto& choice : q.data.choices) out << ' ' << std::quoted(choice);
        out << '\n';
    }
    return writeCommitted(questionsFile_, out.str());
}
Error QuizService::saveQuizzes(const std::vector<Quiz>& values) const {
    std::ostringstream out;
    out << "QUIZ_QUIZZES_V1\n";
    for (const auto& q : values) {
        out << q.id << ' ' << std::quoted(q.title) << ' ' << q.questionIds.size();
        for (const Id id : q.questionIds) out << ' ' << id;
        out << '\n';
    }
    return writeCommitted(quizzesFile_, out.str());
}
Result<Id> QuizService::saveQuestion(const Session& session, const QuestionDraft& draft, std::optional<Id> id) {
    if (!session.isAdmin()) return failure<Id>(Error::AccessDenied);
    if (!validText(draft.prompt, 4096) || draft.correctChoice < 'A' || draft.correctChoice > 'D'
        || !std::all_of(draft.choices.begin(), draft.choices.end(),
                        [](const std::string& s) { return validText(s, 4096); }))
        return failure<Id>(Error::Validation);
    auto staged = questions_;
    Id target = id.value_or(nextId(staged));
    if (target == 0) return failure<Id>(Error::Storage);
    auto found = std::find_if(staged.begin(), staged.end(), [&](const Question& q) { return q.id == target; });
    if (id && found == staged.end()) return failure<Id>(Error::Validation);
    if (found == staged.end()) staged.push_back({target, true, draft});
    else { found->data = draft; found->active = true; }
    if (saveQuestions(staged) != Error::None) return failure<Id>(Error::Storage);
    questions_.swap(staged);
    return success(target);
}
Error QuizService::deactivateQuestion(const Session& session, Id id) {
    if (!session.isAdmin()) return Error::AccessDenied;
    auto staged = questions_;
    const auto found = std::find_if(staged.begin(), staged.end(), [&](const Question& q) { return q.id == id; });
    if (found == staged.end()) return Error::Validation;
    found->active = false;
    if (saveQuestions(staged) != Error::None) return Error::Storage;
    questions_.swap(staged);
    return Error::None;
}
Result<Id> QuizService::saveQuiz(const Session& session, const QuizDraft& draft, std::optional<Id> id) {
    if (!session.isAdmin()) return failure<Id>(Error::AccessDenied);
    if (!validText(draft.title, 512) || draft.questionIds.empty() || draft.questionIds.size() > 10000)
        return failure<Id>(Error::Validation);
    std::unordered_set<Id> seen;
    for (Id questionId : draft.questionIds) {
        const auto q = std::find_if(questions_.begin(), questions_.end(), [&](const Question& item) {
            return item.id == questionId && item.active;
        });
        if (q == questions_.end()) return failure<Id>(Error::UnknownQuestion);
        if (!seen.insert(questionId).second) return failure<Id>(Error::Validation);
    }
    auto staged = quizzes_;
    Id target = id.value_or(nextQuizId(staged));
    if (target == 0) return failure<Id>(Error::Storage);
    auto found = std::find_if(staged.begin(), staged.end(), [&](const Quiz& q) { return q.id == target; });
    if (id && found == staged.end()) return failure<Id>(Error::Validation);
    if (found == staged.end()) staged.push_back({target, draft.title, draft.questionIds});
    else { found->title = draft.title; found->questionIds = draft.questionIds; }
    if (saveQuizzes(staged) != Error::None) return failure<Id>(Error::Storage);
    quizzes_.swap(staged);
    return success(target);
}
Result<std::vector<QuizSummary>> QuizService::listQuizzes(const Session& session) const {
    if (!session.authenticated() || session.isAdmin()) return failure<std::vector<QuizSummary>>(Error::AccessDenied);
    std::vector<QuizSummary> rows;
    for (const auto& q : quizzes_) {
        const bool available = std::all_of(q.questionIds.begin(), q.questionIds.end(), [&](Id id) {
            return std::any_of(questions_.begin(), questions_.end(), [&](const Question& item) {
                return item.id == id && item.active;
            });
        });
        if (available) rows.push_back({q.id, q.title, q.questionIds.size()});
    }
    return success(std::move(rows));
}
Result<QuizScore> QuizService::takeQuiz(const Session& session, Id quizId, std::istream& input, std::ostream& output) {
    if (!session.authenticated() || session.isAdmin()) return failure<QuizScore>(Error::AccessDenied);
    const auto quiz = std::find_if(quizzes_.begin(), quizzes_.end(), [&](const Quiz& q) { return q.id == quizId; });
    if (quiz == quizzes_.end()) return failure<QuizScore>(Error::QuizUnavailable);
    std::vector<QuestionPrompt> prompts;
    std::vector<char> keys;
    for (Id id : quiz->questionIds) {
        const auto q = std::find_if(questions_.begin(), questions_.end(), [&](const Question& item) {
            return item.id == id && item.active;
        });
        if (q == questions_.end()) return failure<QuizScore>(Error::QuizUnavailable);
        prompts.push_back({q->id, q->data.prompt, q->data.choices});
        keys.push_back(q->data.correctChoice);
    }
    AnswerSession attempt(std::move(prompts));
    while (!attempt.complete()) {
        if (!attempt.readNext(input, output)) return failure<QuizScore>(Error::Incomplete);
    }
    std::uint32_t correct = 0;
    for (std::size_t i = 0; i < keys.size(); ++i)
        if (attempt.answers()[i] == keys[i]) ++correct;
    const auto total = static_cast<std::uint32_t>(keys.size());
    const double percentage = 100.0 * correct / total;

    FileAttemptStore existing(directory_);
    std::vector<AttemptRecord> staged = existing.attempts();
    Id attemptId = 1;
    for (const auto& record : staged) {
        if (record.id == std::numeric_limits<Id>::max()) return failure<QuizScore>(Error::Storage);
        attemptId = std::max(attemptId, record.id + 1);
    }
    staged.push_back({attemptId, session.userId(), quiz->title, utcNow(), correct, total, true});
    std::ostringstream data;
    data << "QUIZ_ATTEMPTS_V1\n";
    for (const auto& record : staged)
        data << record.id << ' ' << record.userId << " 1 " << record.correctCount << ' '
             << record.totalQuestions << ' ' << std::quoted(record.quizTitle) << ' '
             << std::quoted(record.submittedAt) << '\n';
    if (writeCommitted(attemptsFile_, data.str()) != Error::None)
        return failure<QuizScore>(Error::Storage);
    output << "Quiz submitted.\nScore: " << correct << " of " << total << " correct ("
           << std::fixed << std::setprecision(1) << percentage << "%).\n";
    return success(QuizScore{quiz->id, quiz->title, correct, total, percentage});
}
}
