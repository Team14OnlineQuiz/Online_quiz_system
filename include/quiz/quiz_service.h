#pragma once
#include "quiz_catalog.h"
#include "result_service.h"
#include <istream>
#include <ostream>
namespace quiz {
struct QuizListing {
    Id id;
    std::string title;
    std::size_t questionCount;
};
struct QuizSnapshot {
    Id id;
    std::string title;
    std::vector<QuestionPrompt> questions;
};
class QuizService {
public:
    QuizService(const QuizCatalog& quizzes, const QuestionCatalog& questions)
        : quizzes_(quizzes), questions_(questions) {}
    Result<std::vector<QuizListing>> discover(const Session& session) const;
    Result<QuizSnapshot> select(const Session& session, Id quizId) const;
    Result<std::uint32_t> grade(const Session& session, const QuizSnapshot& quiz,
        const std::vector<char>& answers) const;
private:
    const QuizCatalog& quizzes_;
    const QuestionCatalog& questions_;
};
Error runLearnerQuiz(const Session& session, std::istream& input, std::ostream& output,
    const QuizService& quizzes, AttemptService& attempts);
std::string currentUtcTimestamp();
}