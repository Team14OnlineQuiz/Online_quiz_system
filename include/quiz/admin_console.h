#pragma once
#include "quiz_catalog.h"
#include <istream>
#include <ostream>
namespace quiz {
void runQuestionMaintenance(const Session& session, QuestionCatalog& questions,
    std::istream& input, std::ostream& output);
void runQuizConfiguration(const Session& session, QuizCatalog& quizzes,
    const QuestionCatalog& questions, std::istream& input, std::ostream& output);
}