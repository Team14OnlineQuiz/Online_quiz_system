#include "quiz/question_service.h"
#include "quiz/account_store.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace quiz {

bool QuestionService::validText(const std::string& value) {
    if (value.empty() || value.size() > 4096) {
        return false;
    }

    const auto hasNonWhitespace = std::any_of(
        value.begin(),
        value.end(),
        [](unsigned char c) { return !std::isspace(c); }
    );

    const auto hasControlCharacter = std::any_of(
        value.begin(),
        value.end(),
        [](unsigned char c) { return c < 32 || c == 127; }
    );

    return hasNonWhitespace && !hasControlCharacter;
}

Id QuestionService::nextId(const std::vector<Question>& questions) {
    Id next = 1;

    for (const auto& question : questions) {
        if (question.id == std::numeric_limits<Id>::max()) {
            return 0;
        }

        next = std::max(next, question.id + 1);
    }

    return next;
}

QuestionService::QuestionService(std::filesystem::path directory) {
    std::filesystem::create_directories(directory);
    file_ = std::move(directory) / "questions.db";
    loadQuestions();
}

void QuestionService::loadQuestions() {
    if (!std::filesystem::exists(file_)) {
        return;
    }

    std::ifstream input(file_);
    if (!input) {
        throw std::runtime_error("Cannot read questions.db");
    }

    std::string line;

    if (!std::getline(input, line) || line != "QUIZ_QUESTIONS_V1") {
        throw std::runtime_error("Invalid questions.db header");
    }

    std::unordered_set<Id> ids;

    while (std::getline(input, line)) {
        if (line.size() > 21000) {
            throw std::runtime_error("Oversized question record");
        }

        long long rawId = 0;
        int active = -1;
        char correct = 0;
        QuestionDraft draft{};

        std::istringstream record(line);

        if (!(record >> rawId >> active >> correct >> std::quoted(draft.prompt))) {
            throw std::runtime_error("Malformed question record");
        }

        for (auto& choice : draft.choices) {
            if (!(record >> std::quoted(choice))) {
                throw std::runtime_error("Malformed question choices");
            }
        }

        record >> std::ws;

        if (!record.eof()
            || rawId <= 0
            || (active != 0 && active != 1)
            || correct < 'A'
            || correct > 'D'
            || !validText(draft.prompt)
            || !std::all_of(draft.choices.begin(), draft.choices.end(), validText)
            || !ids.insert(static_cast<Id>(rawId)).second) {
            throw std::runtime_error("Invalid question record");
        }

        draft.correctChoice = correct;

        questions_.push_back({
            static_cast<Id>(rawId),
            active == 1,
            std::move(draft)
        });
    }

    if (input.bad()) {
        throw std::runtime_error("Question read failed");
    }
}

Error QuestionService::persist(const std::vector<Question>& questions) const {
    auto stagedFile = file_;
    stagedFile += ".tmp";

    try {
        {
            std::ofstream output(stagedFile, std::ios::trunc);

            if (!output) {
                return Error::Storage;
            }

#ifndef _WIN32
            std::filesystem::permissions(
                stagedFile,
                std::filesystem::perms::owner_read
                    | std::filesystem::perms::owner_write
            );
#endif

            output << "QUIZ_QUESTIONS_V1\n";

            for (const auto& question : questions) {
                output << question.id << ' '
                    << (question.active ? 1 : 0) << ' '
                    << question.data.correctChoice << ' '
                    << std::quoted(question.data.prompt);

                for (const auto& choice : question.data.choices) {
                    output << ' ' << std::quoted(choice);
                }

                output << '\n';
            }

            output.flush();

            if (!output) {
                return Error::Storage;
            }
        }

        replaceFile(stagedFile, file_);
        return Error::None;
    } catch (const std::exception&) {
        return Error::Storage;
    }
}

Result<std::vector<AdminQuestion>> QuestionService::listQuestions(
    const Session& session
) const {
    if (!session.isAdmin()) {
        return failure<std::vector<AdminQuestion>>(Error::AccessDenied);
    }

    std::vector<AdminQuestion> result;
    result.reserve(questions_.size());

    for (const auto& question : questions_) {
        result.push_back({question.id, question.active, question.data});
    }

    return success(std::move(result));
}

Result<Id> QuestionService::saveQuestion(
    const Session& session,
    const QuestionDraft& draft,
    std::optional<Id> id
) {
    if (!session.isAdmin()) {
        return failure<Id>(Error::AccessDenied);
    }

    if (!validText(draft.prompt)
        || !std::all_of(draft.choices.begin(), draft.choices.end(), validText)
        || draft.correctChoice < 'A'
        || draft.correctChoice > 'D') {
        return failure<Id>(Error::Validation);
    }

    auto staged = questions_;
    Id questionId = 0;

    if (id.has_value()) {
        questionId = *id;

        const auto it = std::find_if(
            staged.begin(),
            staged.end(),
            [questionId](const Question& question) {
                return question.id == questionId;
            }
        );

        if (it == staged.end()) {
            return failure<Id>(Error::Validation);
        }

        // Editing a deactivated question does not reactivate it.
        it->data = draft;
    } else {
        questionId = nextId(staged);

        if (questionId == 0) {
            return failure<Id>(Error::Storage);
        }

        staged.push_back({questionId, true, draft});
    }

    const auto saved = persist(staged);

    if (saved != Error::None) {
        return failure<Id>(saved);
    }

    questions_.swap(staged);
    return success(questionId);
}

Error QuestionService::deactivateQuestion(const Session& session, Id id) {
    if (!session.isAdmin()) {
        return Error::AccessDenied;
    }

    auto staged = questions_;

    const auto it = std::find_if(
        staged.begin(),
        staged.end(),
        [id](const Question& question) {
            return question.id == id;
        }
    );

    if (it == staged.end()) {
        return Error::Validation;
    }

    it->active = false;

    const auto saved = persist(staged);

    if (saved != Error::None) {
        return saved;
    }

    questions_.swap(staged);
    return Error::None;
}

}
