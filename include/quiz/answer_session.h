#pragma once
#include "console_input.h"
#include "types.h"
#include <array>
#include <vector>
namespace quiz {
struct QuestionPrompt { Id id; std::string text; std::array<std::string, 4> choices; };
class AnswerSession {
public:
    explicit AnswerSession(std::vector<QuestionPrompt> questions);
    bool accept(const std::string& input);
    bool readNext(std::istream& input, std::ostream& output);
    std::size_t position() const { return answers_.size(); }
    bool complete() const { return answers_.size() == questions_.size(); }
    const std::vector<char>& answers() const { return answers_; }
private:
    std::vector<QuestionPrompt> questions_;
    std::vector<char> answers_;
};
}
