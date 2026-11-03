#include "test.h"
#include "temp_directory.h"
#include "quiz/auth_service.h"
#include "quiz/question_catalog.h"
#include "quiz/result_service.h"
#include "quiz/submission_service.h"
#include <fstream>

TEST(submission_scores_answers_and_persists_one_completed_attempt) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "setup-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    const auto learner = auth.signIn("alex", "learner-password");
    REQUIRE(learner);

    const auto questionsFile = temp.path / "questions.db";
    std::ofstream questions(questionsFile);
    questions << "QUIZ_QUESTIONS_V1\n"
        << "1 1 A \"First question\" \"A1\" \"B1\" \"C1\" \"D1\"\n"
        << "2 1 C \"Second question\" \"A2\" \"B2\" \"C2\" \"D2\"\n"
        << "3 1 B \"Third question\" \"A3\" \"B3\" \"C3\" \"D3\"\n";
    questions.close();

    quiz::QuestionCatalog catalog(temp.path);
    REQUIRE(catalog.size() == 3);
    REQUIRE(catalog.correctChoice(1) == 'A');
    REQUIRE(catalog.correctChoice(2) == 'C');
    REQUIRE(catalog.correctChoice(3) == 'B');

    std::vector<quiz::QuestionPrompt> prompts;
    for (quiz::Id id = 1; id <= 3; ++id) {
        const auto* prompt = catalog.find(id);
        REQUIRE(prompt != nullptr);
        prompts.push_back(*prompt);
    }
    quiz::AnswerSession answers(std::move(prompts));
    REQUIRE(answers.accept("A"));
    REQUIRE(answers.accept("D"));
    REQUIRE(answers.accept("B"));
    REQUIRE(answers.complete());

    quiz::FileAttemptStore attempts(temp.path);
    quiz::SubmissionService submissions(attempts, catalog);
    const auto submitted = submissions.submit(*learner.value, "Sample quiz",
        "2026-10-20T10:00:00Z", answers);
    REQUIRE(submitted);
    REQUIRE(submitted.value->correctCount == 2);
    REQUIRE(submitted.value->totalQuestions == 3);
    REQUIRE(submitted.value->percentage > 66.6 && submitted.value->percentage < 66.7);
    REQUIRE(submitted.value->attemptId == 1);
    REQUIRE(answers.submitted());
    REQUIRE(attempts.attempts().size() == 1);
    REQUIRE(attempts.attempts()[0].completed);
    REQUIRE(attempts.attempts()[0].correctCount == 2);
    REQUIRE(attempts.attempts()[0].totalQuestions == 3);
    REQUIRE(attempts.attempts()[0].quizTitle == "Sample quiz");

    quiz::FileAttemptStore reloaded(temp.path);
    REQUIRE(reloaded.attempts().size() == 1);
    REQUIRE(reloaded.attempts()[0].correctCount == 2);
    REQUIRE(reloaded.attempts()[0].submittedAt == "2026-10-20T10:00:00Z");

    const auto duplicate = submissions.submit(*learner.value, "Sample quiz",
        "2026-10-20T10:01:00Z", answers);
    REQUIRE(!duplicate);
    REQUIRE(attempts.attempts().size() == 1);
    REQUIRE(!answers.accept("A"));
}

TEST(submission_rejects_guests_admins_and_incomplete_answers) {
    TempDirectory temp;
    quiz::PasswordHasher hasher;
    quiz::FileAccountStore accounts(temp.path);
    quiz::AuthService auth(accounts, hasher);
    REQUIRE(auth.provisionFirstAdmin("owner", "setup-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));
    const auto learner = auth.signIn("alex", "learner-password");
    const auto admin = auth.signIn("owner", "setup-password");
    REQUIRE(learner && admin);

    std::ofstream questions(temp.path / "questions.db");
    questions << "QUIZ_QUESTIONS_V1\n"
        << "1 1 A \"Question\" \"A\" \"B\" \"C\" \"D\"\n";
    questions.close();

    quiz::QuestionCatalog catalog(temp.path);
    quiz::FileAttemptStore attempts(temp.path);
    quiz::SubmissionService submissions(attempts, catalog);
    quiz::AnswerSession incomplete({*catalog.find(1)});

    REQUIRE(submissions.submit(quiz::Session{}, "Quiz",
        "2026-10-20T10:00:00Z", incomplete).error == quiz::Error::Authentication);
    REQUIRE(submissions.submit(*admin.value, "Quiz",
        "2026-10-20T10:00:00Z", incomplete).error == quiz::Error::AccessDenied);
    REQUIRE(submissions.submit(*learner.value, "Quiz",
        "2026-10-20T10:00:00Z", incomplete).error == quiz::Error::Validation);
    REQUIRE(attempts.attempts().empty());
}