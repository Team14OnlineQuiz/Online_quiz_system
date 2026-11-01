#pragma once
#include "answer_session.h"
#include "types.h"
#include <filesystem>
#include <map>
#include <optional>
namespace quiz {
struct StoredQuestion {
    QuestionPrompt prompt;
    char correctAnswer = 'A';
    bool active = true;
};
class QuestionCatalog {
public:
    explicit QuestionCatalog(const std::filesystem::path& directory);
    const QuestionPrompt* find(Id id) const;
    std::optional<char> correctAnswer(const Session& session, Id id) const;
    bool contains(Id id) const;
    std::size_t size() const;
    std::vector<QuestionPrompt> list(const Session& session) const;
    Result<Id> add(const Session& session, QuestionPrompt prompt, char correctAnswer);
    Error modify(const Session& session, Id id, QuestionPrompt prompt, char correctAnswer);
    Error remove(const Session& session, Id id);
private:
    friend class QuizService;
    std::optional<char> answerForGrading(Id id) const;
    bool persist(const std::map<Id, StoredQuestion>& staged) const;
    std::filesystem::path file_;
    std::map<Id, StoredQuestion> questions_;
};
bool validQuestion(const QuestionPrompt& prompt, char correctAnswer);
}