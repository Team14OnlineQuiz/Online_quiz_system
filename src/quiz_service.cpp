#include "quiz/quiz_service.h"
#include "quiz/account_store.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace quiz {

QuizService::QuizService(std::filesystem::path directory)
    : directory_(std::move(directory)) {
    std::filesystem::create_directories(directory_);
    file_ = directory_ / "quizzes.db";
    loadQuizzes();
}

void QuizService::loadQuizzes() {
    if (!std::filesystem::exists(file_)) return;

    std::ifstream input(file_);
    if (!input) throw std::runtime_error("Cannot read quizzes.db; repair the data directory");

    std::string line;
    if (!std::getline(input, line) || line != "QUIZ_DEFINITIONS_V1")
        throw std::runtime_error("Invalid quizzes.db header; restore a valid backup");

    QuestionCatalog catalog(directory_);
    std::unordered_set<Id> ids;

    while (std::getline(input, line)) {
        if (line.size() > 16384)
            throw std::runtime_error("Oversized quiz record");

        QuizRecord quiz{};
        std::size_t count = 0;
        std::istringstream record(line);

        if (!(record >> quiz.id >> std::quoted(quiz.title) >> count)
            || quiz.id == 0 || count == 0 || count > 1000)
            throw std::runtime_error("Malformed quiz record; restore a valid backup");

        for (std::size_t i = 0; i < count; ++i) {
            Id questionId = 0;
            if (!(record >> questionId) || questionId == 0)
                throw std::runtime_error("Malformed quiz question list; restore a valid backup");
            quiz.questionIds.push_back(questionId);
        }

        record >> std::ws;
        if (!record.eof() || quiz.title.empty()
            || quiz.title.find_first_not_of(' ') == std::string::npos
            || !ids.insert(quiz.id).second)
            throw std::runtime_error("Invalid quiz record; restore a valid backup");

        std::unordered_set<Id> questionIds;
        for (Id questionId : quiz.questionIds) {
            if (!questionIds.insert(questionId).second)
                throw std::runtime_error("Duplicate question in quiz record");
            if (!catalog.find(questionId))
                throw std::runtime_error(
                    "Quiz references a missing or inactive question; repair the quiz data or restore the question");
        }

        quizzes_.push_back(std::move(quiz));
    }

    if (input.bad()) throw std::runtime_error("Quiz data read failed");
}

Result<std::vector<QuizRecord>> QuizService::listQuizzes(const Session& session) const {
    if (!session.authenticated()) return failure<std::vector<QuizRecord>>(Error::Authentication);
    return success(quizzes_);
}

Id QuizService::nextId() const {
    Id next = 1;
    for (const auto& quiz : quizzes_) {
        if (quiz.id == std::numeric_limits<Id>::max()) return 0;
        next = std::max(next, quiz.id + 1);
    }
    return next;
}

bool QuizService::validDraft(const QuizDraft& draft) const {
    if (draft.title.empty() || draft.title.size() > 256
        || draft.questionIds.empty() || draft.questionIds.size() > 1000)
        return false;

    const bool hasVisibleTitleCharacter = std::any_of(
        draft.title.begin(), draft.title.end(),
        [](unsigned char ch) {
            return ch > 32;
        });

    if (!hasVisibleTitleCharacter) return false;

    for (unsigned char ch : draft.title) {
        if (ch < 32 || ch == 127) return false;
    }

    std::unordered_set<Id> ids;
    for (Id id : draft.questionIds) {
        if (id == 0 || !ids.insert(id).second) return false;
    }

    return true;
}

Error QuizService::persist(const std::vector<QuizRecord>& quizzes) const {
    auto stagedFile = file_;
    stagedFile += ".tmp";

    try {
        std::ofstream output(stagedFile, std::ios::trunc);
        if (!output) return Error::Storage;

        output << "QUIZ_DEFINITIONS_V1\n";
        for (const auto& quiz : quizzes) {
            output << quiz.id << ' ' << std::quoted(quiz.title)
                   << ' ' << quiz.questionIds.size();
            for (Id questionId : quiz.questionIds) output << ' ' << questionId;
            output << '\n';
        }

        output.flush();
        if (!output) return Error::Storage;
        output.close();
        if (!output) return Error::Storage;

        replaceFile(stagedFile, file_);
        return Error::None;
    } catch (const std::exception&) {
        return Error::Storage;
    }
}

Result<Id> QuizService::saveQuiz(
    const Session& session,
    const QuizDraft& draft,
    std::optional<Id> id) {

    if (!session.isAdmin()) return failure<Id>(Error::AccessDenied);
    if (!validDraft(draft)) return failure<Id>(Error::Validation);

    QuestionCatalog catalog(directory_);
    for (Id questionId : draft.questionIds) {
        if (!catalog.find(questionId)) return failure<Id>(Error::Validation);
    }

    auto staged = quizzes_;
    Id savedId = 0;

    if (id) {
        auto it = std::find_if(staged.begin(), staged.end(),
            [&](const QuizRecord& quiz) { return quiz.id == *id; });

        if (it == staged.end()) return failure<Id>(Error::Validation);

        it->title = draft.title;
        it->questionIds = draft.questionIds;
        savedId = *id;
    } else {
        savedId = nextId();
        if (savedId == 0) return failure<Id>(Error::Storage);

        staged.push_back({savedId, draft.title, draft.questionIds});
    }

    const Error result = persist(staged);
    if (result != Error::None) return failure<Id>(result);

    quizzes_.swap(staged);
    return success(savedId);
}

}


