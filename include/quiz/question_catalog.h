#pragma once
#include "answer_session.h"
#include <filesystem>
#include <unordered_map>
namespace quiz {
// Read-only adapter and integration seam; administration/writing is a separate story.
class QuestionCatalog {
public:
    explicit QuestionCatalog(const std::filesystem::path& directory);
    const QuestionPrompt* find(Id id) const;
    char correctChoice(Id id) const;
    std::size_t size() const { return prompts_.size(); }
private:
    std::unordered_map<Id, QuestionPrompt> prompts_;
    std::unordered_map<Id, char> answerKeys_;
};
}
