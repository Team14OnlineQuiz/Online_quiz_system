#pragma once
#include "answer_session.h"
#include <filesystem>
#include <unordered_map>
#include <vector>
namespace quiz {
// Read-only adapter and integration seam; administration/writing is a separate story.
class QuestionCatalog {
public:
    explicit QuestionCatalog(const std::filesystem::path& directory);
    const QuestionPrompt* find(Id id) const;
    std::vector<QuestionPrompt> activeQuestions() const;
    std::size_t size() const { return prompts_.size(); }
private:
    std::unordered_map<Id, QuestionPrompt> prompts_;
};
}
