#include "quiz/question_catalog.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
namespace quiz {
QuestionCatalog::QuestionCatalog(const std::filesystem::path& directory) {
    const auto file = directory / "questions.db";
    if (!std::filesystem::exists(file)) return;
    std::ifstream input(file);
    if (!input) throw std::runtime_error("Cannot read questions.db");
    std::string line;
    if (!std::getline(input, line) || line != "QUIZ_QUESTIONS_V1") throw std::runtime_error("Invalid questions.db header");
    std::unordered_set<Id> ids;
    const auto validText = [](const std::string& value) {
        return !value.empty() && value.size() <= 4096
            && std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 32 || c == 127; });
    };
    while (std::getline(input, line)) {
        if (line.size() > 21000) throw std::runtime_error("Oversized question record");
        long long id = 0;
        int active = -1;
        char correct = 0;
        QuestionPrompt prompt{};
        std::istringstream record(line);
        if (!(record >> id >> active >> correct >> std::quoted(prompt.text))) throw std::runtime_error("Malformed question record");
        for (auto& choice : prompt.choices)
            if (!(record >> std::quoted(choice))) throw std::runtime_error("Malformed question choices");
        record >> std::ws;
        if (!record.eof() || id <= 0 || (active != 0 && active != 1) || correct < 'A' || correct > 'D'
            || !validText(prompt.text) || !std::all_of(prompt.choices.begin(), prompt.choices.end(), validText)
            || !ids.insert(static_cast<Id>(id)).second) throw std::runtime_error("Invalid question record");
        prompt.id = static_cast<Id>(id);
        if (active == 1) {
            answerKeys_.emplace(prompt.id, correct);
            prompts_.emplace(prompt.id, std::move(prompt));
        }
    }
    if (input.bad()) throw std::runtime_error("Question read failed");
}
const QuestionPrompt* QuestionCatalog::find(Id id) const {
    const auto it = prompts_.find(id);
    return it == prompts_.end() ? nullptr : &it->second;
}
char QuestionCatalog::correctChoice(Id id) const {
    const auto it = answerKeys_.find(id);
    return it == answerKeys_.end() ? '\0' : it->second;
}
}
