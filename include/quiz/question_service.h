#pragma once

#include "answer_session.h"
#include "types.h"

#include <filesystem>
#include <optional>
#include <vector>

namespace quiz {

struct QuestionDraft {
    std::string prompt;
    std::array<std::string, 4> choices;
    char correctChoice = 'A';
};

struct AdminQuestion {
    Id id;
    bool active;
    QuestionDraft data;
};

class QuestionService {
public:
    explicit QuestionService(std::filesystem::path directory);

    Result<std::vector<AdminQuestion>> listQuestions(const Session& session) const;

    Result<Id> saveQuestion(
        const Session& session,
        const QuestionDraft& draft,
        std::optional<Id> id = std::nullopt
    );

    Error deactivateQuestion(const Session& session, Id id);

private:
    struct Question {
        Id id;
        bool active;
        QuestionDraft data;
    };

    std::filesystem::path file_;
    std::vector<Question> questions_;

    void loadQuestions();
    Error persist(const std::vector<Question>& questions) const;

    static bool validText(const std::string& value);
    static Id nextId(const std::vector<Question>& questions);
};

}
