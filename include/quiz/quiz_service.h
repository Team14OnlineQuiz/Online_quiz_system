#pragma once

#include "question_catalog.h"
#include "types.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace quiz {

struct QuizDraft {
    std::string title;
    std::vector<Id> questionIds;
};

struct QuizRecord {
    Id id;
    std::string title;
    std::vector<Id> questionIds;
};

class QuizService {
public:
    explicit QuizService(std::filesystem::path directory);

    Result<std::vector<QuizRecord>> listQuizzes(const Session& session) const;

    Result<Id> saveQuiz(
        const Session& session,
        const QuizDraft& draft,
        std::optional<Id> id = std::nullopt
    );

private:
    std::filesystem::path file_;
    std::filesystem::path directory_;
    std::vector<QuizRecord> quizzes_;

    void loadQuizzes();
    Error persist(const std::vector<QuizRecord>& quizzes) const;
    Id nextId() const;
    bool validDraft(const QuizDraft& draft) const;
};

}
