#pragma once
#include "question_catalog.h"
#include <filesystem>
#include <vector>
namespace quiz {
struct QuizDefinition {
    Id id = 0;
    std::string title;
    std::vector<Id> questionIds;
    bool active = true;
};
class QuizCatalog {
public:
    QuizCatalog(const std::filesystem::path& directory, const QuestionCatalog& questions);
    const QuizDefinition* find(Id id) const;
    std::vector<QuizDefinition> list() const;
    Result<Id> add(const Session& session, const QuestionCatalog& questions,
        std::string title, std::vector<Id> questionIds);
    Error modify(const Session& session, const QuestionCatalog& questions, Id id,
        std::string title, std::vector<Id> questionIds);
    Error remove(const Session& session, Id id);
private:
    bool validDefinition(const QuestionCatalog& questions, const std::string& title,
        const std::vector<Id>& questionIds) const;
    bool persist(const std::vector<QuizDefinition>& staged) const;
    std::filesystem::path file_;
    std::vector<QuizDefinition> quizzes_;
};
}