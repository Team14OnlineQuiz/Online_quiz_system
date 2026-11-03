#include "quiz/question_catalog.h"
#include "quiz/account_store.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
namespace quiz {
bool validQuestion(const QuestionPrompt& prompt, char correctAnswer) {
    const auto validText = [](const std::string& value, std::size_t limit) {
        return !value.empty() && value.size() <= limit
            && std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 32 || c == 127; });
    };
    return validText(prompt.text, 4096) && correctAnswer >= 'A' && correctAnswer <= 'D'
        && std::all_of(prompt.choices.begin(), prompt.choices.end(),
            [&](const std::string& choice) { return validText(choice, 4096); });
}
QuestionCatalog::QuestionCatalog(const std::filesystem::path& directory) : file_(directory / "questions.db") {
    if (!std::filesystem::exists(file_)) return;
    std::ifstream input(file_);
    if (!input) throw std::runtime_error("Cannot read questions.db");
    std::string line;
    if (!std::getline(input, line) || line != "QUIZ_QUESTIONS_V1")
        throw std::runtime_error("Invalid questions.db header");
    while (std::getline(input, line)) {
        if (line.size() > 42000) throw std::runtime_error("Oversized question record");
        long long id = 0;
        int active = -1;
        char correct = 0;
        StoredQuestion stored{};
        std::istringstream record(line);
        if (!(record >> id >> active >> correct >> std::quoted(stored.prompt.text)))
            throw std::runtime_error("Malformed question record");
        for (auto& choice : stored.prompt.choices)
            if (!(record >> std::quoted(choice))) throw std::runtime_error("Malformed question choices");
        record >> std::ws;
        if (!record.eof() || id <= 0 || (active != 0 && active != 1)
            || !validQuestion(stored.prompt, correct) || questions_.count(static_cast<Id>(id)) != 0)
            throw std::runtime_error("Invalid question record");
        stored.prompt.id = static_cast<Id>(id);
        stored.correctAnswer = correct;
        stored.active = active == 1;
        questions_.emplace(stored.prompt.id, std::move(stored));
    }
    if (input.bad()) throw std::runtime_error("Question read failed");
}
const QuestionPrompt* QuestionCatalog::find(Id id) const {
    const auto it = questions_.find(id);
    return it == questions_.end() || !it->second.active ? nullptr : &it->second.prompt;
}
std::optional<char> QuestionCatalog::correctAnswer(const Session& session, Id id) const {
    if (!session.isAdmin()) return std::nullopt;
    const auto it = questions_.find(id);
    if (it == questions_.end() || !it->second.active) return std::nullopt;
    return it->second.correctAnswer;
}
std::optional<char> QuestionCatalog::answerForGrading(Id id) const {
    const auto it = questions_.find(id);
    if (it == questions_.end() || !it->second.active) return std::nullopt;
    return it->second.correctAnswer;
}
bool QuestionCatalog::contains(Id id) const { return questions_.find(id) != questions_.end(); }
std::size_t QuestionCatalog::size() const {
    return static_cast<std::size_t>(std::count_if(questions_.begin(), questions_.end(),
        [](const auto& entry) { return entry.second.active; }));
}
std::vector<QuestionPrompt> QuestionCatalog::list(const Session& session) const {
    std::vector<QuestionPrompt> result;
    if (!session.isAdmin()) return result;
    for (const auto& entry : questions_) if (entry.second.active) result.push_back(entry.second.prompt);
    return result;
}
bool QuestionCatalog::persist(const std::map<Id, StoredQuestion>& staged) const {
    auto temporary = file_;
    temporary += ".tmp";
    try {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) return false;
        output << "QUIZ_QUESTIONS_V1\n";
        for (const auto& entry : staged) {
            const auto& q = entry.second;
            output << entry.first << ' ' << (q.active ? 1 : 0) << ' ' << q.correctAnswer << ' '
                << std::quoted(q.prompt.text);
            for (const auto& choice : q.prompt.choices) output << ' ' << std::quoted(choice);
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
Result<Id> QuestionCatalog::add(const Session& session, QuestionPrompt prompt, char correctAnswer) {
    if (!session.isAdmin()) return failure<Id>(Error::AccessDenied);
    if (!validQuestion(prompt, correctAnswer)) return failure<Id>(Error::Validation);
    Id next = 1;
    for (const auto& entry : questions_) {
        if (entry.first == std::numeric_limits<Id>::max()) return failure<Id>(Error::Storage);
        next = std::max(next, entry.first + 1);
    }
    auto staged = questions_;
    prompt.id = next;
    staged.emplace(next, StoredQuestion{std::move(prompt), correctAnswer, true});
    if (!persist(staged)) return failure<Id>(Error::Storage);
    questions_.swap(staged);
    return success(next);
}
Error QuestionCatalog::modify(const Session& session, Id id, QuestionPrompt prompt, char correctAnswer) {
    if (!session.isAdmin()) return Error::AccessDenied;
    if (!validQuestion(prompt, correctAnswer)) return Error::Validation;
    auto found = questions_.find(id);
    if (found == questions_.end() || !found->second.active) return Error::Validation;
    auto staged = questions_;
    prompt.id = id;
    staged.at(id) = StoredQuestion{std::move(prompt), correctAnswer, true};
    if (!persist(staged)) return Error::Storage;
    questions_.swap(staged);
    return Error::None;
}
Error QuestionCatalog::remove(const Session& session, Id id) {
    if (!session.isAdmin()) return Error::AccessDenied;
    auto found = questions_.find(id);
    if (found == questions_.end() || !found->second.active) return Error::Validation;
    auto staged = questions_;
    staged.at(id).active = false;
    if (!persist(staged)) return Error::Storage;
    questions_.swap(staged);
    return Error::None;
}
}