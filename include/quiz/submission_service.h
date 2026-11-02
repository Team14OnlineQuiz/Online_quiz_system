#pragma once
#include "question_catalog.h"
#include "result_service.h"
#include <string>
namespace quiz {
struct SubmissionResult {
    Id attemptId;
    std::uint32_t correctCount;
    std::uint32_t totalQuestions;
    double percentage;
};
class SubmissionService {
public:
    SubmissionService(AttemptStore& attempts, const QuestionCatalog& questions)
        : attempts_(attempts), questions_(questions) {}
    Result<SubmissionResult> submit(const Session& session, const std::string& quizTitle,
        const std::string& submittedAtUTC, AnswerSession& answers);
private:
    AttemptStore& attempts_;
    const QuestionCatalog& questions_;
};
}