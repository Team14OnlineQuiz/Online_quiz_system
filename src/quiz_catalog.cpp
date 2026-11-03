#include "quiz/quiz_catalog.h"
#include "quiz/account_store.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
namespace quiz {
QuizCatalog::QuizCatalog(const std::filesystem::path& directory, const QuestionCatalog& questions)
    : file_(directory / "quizzes.db") {
    if (!std::filesystem::exists(file_)) return;
    std::ifstream input(file_);
    if (!input) throw std::runtime_error("Cannot read quizzes.db");
    std::string line;
    if (!std::getline(input, line) || (line != "QUIZ_QUIZZES_V1" && line != "QUIZ_DEFINITIONS_V1"))
        throw std::runtime_error("Invalid quizzes.db header");
    const bool legacy = line == "QUIZ_DEFINITIONS_V1";
    std::unordered_set<Id> ids;
    while (std::getline(input, line)) {
        if (line.size() > 8192) throw std::runtime_error("Oversized quiz record");
        long long id = 0;
        int active = 1;
        std::size_t count = 0;
        QuizDefinition quiz{};
        std::istringstream record(line);
        if (!(record >> id) || (!legacy && !(record >> active))
            || !(record >> std::quoted(quiz.title) >> count) || count == 0 || count > 100)
            throw std::runtime_error("Malformed quiz record");
        quiz.questionIds.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            long long questionId = 0;
            if (!(record >> questionId) || questionId <= 0 || !questions.contains(static_cast<Id>(questionId)))
                throw std::runtime_error("Invalid quiz question reference");
            quiz.questionIds.push_back(static_cast<Id>(questionId));
        }
        record >> std::ws;
        if (!record.eof() || id <= 0 || (active != 0 && active != 1)
            || quiz.title.empty() || quiz.title.size() > 120
            || std::any_of(quiz.title.begin(), quiz.title.end(), [](unsigned char c) { return c < 32 || c == 127; })
            || !ids.insert(static_cast<Id>(id)).second)
            throw std::runtime_error("Invalid quiz record");
        std::unordered_set<Id> unique(quiz.questionIds.begin(), quiz.questionIds.end());
        if (unique.size() != quiz.questionIds.size()) throw std::runtime_error("Duplicate quiz question");
        quiz.id = static_cast<Id>(id);
        quiz.active = active == 1;
        quizzes_.push_back(std::move(quiz));
    }
    if (input.bad()) throw std::runtime_error("Quiz read failed");
}
const QuizDefinition* QuizCatalog::lookup(Id id) const {
    const auto it = std::find_if(quizzes_.begin(), quizzes_.end(),
        [&](const QuizDefinition& quiz) { return quiz.id == id && quiz.active; });
    return it == quizzes_.end() ? nullptr : &*it;
}
const QuizDefinition* QuizCatalog::find(const Session& session, Id id) const {
    if (!session.isAdmin()) return nullptr;
    return lookup(id);
}
const QuizDefinition* QuizCatalog::findForLearner(const Session& session, Id id) const {
    if (!session.authenticated() || session.isAdmin()) return nullptr;
    return lookup(id);
}
std::vector<QuizDefinition> QuizCatalog::activeList() const {
    std::vector<QuizDefinition> result;
    for (const auto& quiz : quizzes_) if (quiz.active) result.push_back(quiz);
    return result;
}
std::vector<QuizDefinition> QuizCatalog::list(const Session& session) const {
    if (!session.isAdmin()) return {};
    return activeList();
}
Result<std::vector<QuizDefinition>> QuizCatalog::learnerList(const Session& session) const {
    if (!session.authenticated() || session.isAdmin())
        return failure<std::vector<QuizDefinition>>(Error::AccessDenied);
    return success(activeList());
}
bool QuizCatalog::validDefinition(const QuestionCatalog& questions, const std::string& title,
    const std::vector<Id>& questionIds) const {
    if (title.empty() || title.size() > 120 || questionIds.empty() || questionIds.size() > 100) return false;
    if (std::any_of(title.begin(), title.end(), [](unsigned char c) { return c < 32 || c == 127; })) return false;
    std::unordered_set<Id> unique;
    for (Id id : questionIds) if (!questions.find(id) || !unique.insert(id).second) return false;
    return true;
}
bool QuizCatalog::persist(const std::vector<QuizDefinition>& staged) const {
    auto temporary = file_;
    temporary += ".tmp";
    try {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) return false;
        output << "QUIZ_QUIZZES_V1\n";
        for (const auto& quiz : staged) {
            output << quiz.id << ' ' << (quiz.active ? 1 : 0) << ' ' << std::quoted(quiz.title)
                << ' ' << quiz.questionIds.size();
            for (Id id : quiz.questionIds) output << ' ' << id;
            output << '\n';
        }
        output.flush();
        if (!output) return false;
        output.close();
        if (!output) return false;
        replaceFile(temporary, file_);
        return true;
    } catch (const std::exception&) { return false; }
}
Result<Id> QuizCatalog::add(const Session& session, const QuestionCatalog& questions,
    std::string title, std::vector<Id> questionIds) {
    if (!session.isAdmin()) return failure<Id>(Error::AccessDenied);
    if (!validDefinition(questions, title, questionIds)) return failure<Id>(Error::Validation);
    Id next = 1;
    for (const auto& quiz : quizzes_) {
        if (quiz.id == std::numeric_limits<Id>::max()) return failure<Id>(Error::Storage);
        next = std::max(next, quiz.id + 1);
    }
    auto staged = quizzes_;
    staged.push_back({next, std::move(title), std::move(questionIds), true});
    if (!persist(staged)) return failure<Id>(Error::Storage);
    quizzes_.swap(staged);
    return success(next);
}
Error QuizCatalog::modify(const Session& session, const QuestionCatalog& questions, Id id,
    std::string title, std::vector<Id> questionIds) {
    if (!session.isAdmin()) return Error::AccessDenied;
    if (!validDefinition(questions, title, questionIds)) return Error::Validation;
    auto found = std::find_if(quizzes_.begin(), quizzes_.end(),
        [&](const QuizDefinition& quiz) { return quiz.id == id && quiz.active; });
    if (found == quizzes_.end()) return Error::Validation;
    auto staged = quizzes_;
    auto target = std::find_if(staged.begin(), staged.end(),
        [&](const QuizDefinition& quiz) { return quiz.id == id; });
    target->title = std::move(title);
    target->questionIds = std::move(questionIds);
    if (!persist(staged)) return Error::Storage;
    quizzes_.swap(staged);
    return Error::None;
}
Error QuizCatalog::remove(const Session& session, Id id) {
    if (!session.isAdmin()) return Error::AccessDenied;
    auto found = std::find_if(quizzes_.begin(), quizzes_.end(),
        [&](const QuizDefinition& quiz) { return quiz.id == id && quiz.active; });
    if (found == quizzes_.end()) return Error::Validation;
    auto staged = quizzes_;
    auto target = std::find_if(staged.begin(), staged.end(),
        [&](const QuizDefinition& quiz) { return quiz.id == id; });
    target->active = false;
    if (!persist(staged)) return Error::Storage;
    quizzes_.swap(staged);
    return Error::None;
}
}