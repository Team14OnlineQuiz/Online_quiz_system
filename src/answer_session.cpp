#include "quiz/answer_session.h"
#include <stdexcept>
namespace quiz {
AnswerSession::AnswerSession(std::vector<QuestionPrompt> questions) : questions_(std::move(questions)) {
    for (const auto& q : questions_) {
        if (q.text.empty()) throw std::invalid_argument("Question prompt cannot be empty");
        for (const auto& choice : q.choices)
            if (choice.empty()) throw std::invalid_argument("Question choices cannot be empty");
    }
}
bool AnswerSession::accept(const std::string& input) {
    if (complete() || input.size() != 1 || input[0] < 'A' || input[0] > 'D') return false;
    answers_.push_back(input[0]);
    return true;
}
bool AnswerSession::readNext(std::istream& input, std::ostream& output) {
    if (complete()) return false;
    while (true) {
        const auto& q = questions_[position()];
        output << "Question " << position() + 1 << " of " << questions_.size() << ": " << q.text << '\n';
        for (std::size_t i = 0; i < q.choices.size(); ++i)
            output << static_cast<char>('A' + i) << ". " << q.choices[i] << '\n';
        output << "Choose one answer (accepted values: A, B, C, or D): " << std::flush;
        const auto line = readLine(input, 4);
        if (line.state == InputState::End) return false;
        if (line.state == InputState::Value && accept(line.value)) return true;
        output << "Invalid answer. Please enter exactly one uppercase letter: A, B, C, or D.\n";
    }
}
}
