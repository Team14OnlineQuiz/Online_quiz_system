#pragma once
#include "answer_session.h"
#include "result_service.h"
#include <filesystem>
#include <istream>
#include <ostream>
namespace quiz {
struct QuestionDraft {
    std::string prompt;
    std::array<std::string, 4> choices;
    char correctChoice = 'A';
};
struct QuizDraft {
    std::string title;
    std::vector<Id> questionIds;
};
struct QuizSummary {
    Id id;
    std::string title;
    std::size_t questionCount;
};
struct QuizScore {
    Id quizId;
    std::string quizTitle;
    std::uint32_t correctCount;
    std::uint32_t totalQuestions;
    double percentage;
};
class QuizService {
public:
    explicit QuizService(std::filesystem::path directory);
    Result<Id> saveQuestion(const Session&, const QuestionDraft&, std::optional<Id> id = std::nullopt);
    Error deactivateQuestion(const Session&, Id id);
    Result<Id> saveQuiz(const Session&, const QuizDraft&, std::optional<Id> id = std::nullopt);
    Result<std::vector<QuizSummary>> listQuizzes(const Session&) const;
    Result<QuizScore> takeQuiz(const Session&, Id quizId, std::istream&, std::ostream&);
private:
    struct Question { Id id; bool active; QuestionDraft data; };
    struct Quiz { Id id; std::string title; std::vector<Id> questionIds; };
    std::filesystem::path directory_, questionsFile_, quizzesFile_, attemptsFile_;
    std::vector<Question> questions_;
    std::vector<Quiz> quizzes_;
    void loadQuestions();
    void loadQuizzes();
    Error saveQuestions(const std::vector<Question>&) const;
    Error saveQuizzes(const std::vector<Quiz>&) const;
    static bool validText(const std::string&, std::size_t limit);
    static Id nextId(const std::vector<Question>&);
    static Id nextQuizId(const std::vector<Quiz>&);
};
}
