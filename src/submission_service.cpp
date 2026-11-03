#include "quiz/submission_service.h"
#include <limits>
namespace quiz {
Result<SubmissionResult> SubmissionService::submit(const Session& session,
    const std::string& quizTitle, const std::string& submittedAtUTC, AnswerSession& answers) {
    if (!session.authenticated()) return failure<SubmissionResult>(Error::Authentication);
    if (session.isAdmin()) return failure<SubmissionResult>(Error::AccessDenied);
    if (answers.submitted() || !answers.complete() || quizTitle.empty()
        || quizTitle.size() > 512
        return failure<SubmissionResult>(Error::Validation);

    const auto ids = answers.questionIds();
    const auto& selected = answers.answers();
    if (ids.empty() || ids.size() != selected.size()
        || ids.size() > std::numeric_limits<std::uint32_t>::max())
        return failure<SubmissionResult>(Error::Validation);

    std::uint32_t correctCount = 0;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const char key = questions_.correctChoice(ids[i]);
        if (key < 'A' || key > 'D') return failure<SubmissionResult>(Error::Validation);
        if (selected[i] == key) ++correctCount;
    }

    const auto total = static_cast<std::uint32_t>(ids.size());
    AttemptRecord attempt{0, session.userId(), quizTitle, submittedAtUTC,
        correctCount, total, true};
    const auto saved = attempts_.append(std::move(attempt));
    if (!saved) return failure<SubmissionResult>(saved.error);
    if (!answers.markSubmitted()) return failure<SubmissionResult>(Error::Validation);

    return success(SubmissionResult{*saved.value, correctCount, total,
        100.0 * correctCount / total});
}
}