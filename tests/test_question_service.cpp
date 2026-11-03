#include "test.h"
#include "temp_directory.h"

#include "quiz/auth_service.h"
#include "quiz/password_hasher.h"
#include "quiz/question_catalog.h"
#include "quiz/question_service.h"

TEST(admin_can_create_and_edit_questions_and_changes_persist) {
    TempDirectory temp;

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);

    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));

    auto signedIn = auth.signIn("owner", "admin-password");
    REQUIRE(signedIn);

    quiz::QuestionService service(temp.path);

    quiz::QuestionDraft draft{
        "What is 2 + 2?",
        {"3", "4", "5", "6"},
        'B'
    };

    const auto created = service.saveQuestion(*signedIn.value, draft);
    REQUIRE(created);
    REQUIRE(*created.value == 1);

    draft.prompt = "What is 3 + 3?";
    draft.choices = {"5", "6", "7", "8"};
    draft.correctChoice = 'B';

    const auto edited = service.saveQuestion(
        *signedIn.value,
        draft,
        *created.value
    );

    REQUIRE(edited);

    quiz::QuestionService restarted(temp.path);
    const auto questions = restarted.listQuestions(*signedIn.value);

    REQUIRE(questions);
    REQUIRE(questions.value->size() == 1);
    REQUIRE(questions.value->at(0).data.prompt == "What is 3 + 3?");
    REQUIRE(questions.value->at(0).data.correctChoice == 'B');
}

TEST(only_authenticated_admins_can_manage_questions) {
    TempDirectory temp;

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);

    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));

    auto admin = auth.signIn("owner", "admin-password");
    auto learner = auth.signIn("alex", "learner-password");

    REQUIRE(admin);
    REQUIRE(learner);

    quiz::QuestionService service(temp.path);

    quiz::QuestionDraft draft{
        "A valid question?",
        {"one", "two", "three", "four"},
        'A'
    };

    const auto denied = service.saveQuestion(*learner.value, draft);
    REQUIRE(!denied);
    REQUIRE(denied.error == quiz::Error::AccessDenied);

    REQUIRE(service.listQuestions(*learner.value).error
        == quiz::Error::AccessDenied);

    REQUIRE(service.deactivateQuestion(*learner.value, 1)
        == quiz::Error::AccessDenied);

    const auto questions = service.listQuestions(*admin.value);
    REQUIRE(questions);
    REQUIRE(questions.value->empty());
}

TEST(question_validation_rejects_empty_fields_and_invalid_answer_keys) {
    TempDirectory temp;

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);

    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));

    auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);

    quiz::QuestionService service(temp.path);

    quiz::QuestionDraft draft{
        "A valid question?",
        {"one", "two", "three", "four"},
        'A'
    };

    draft.prompt = "   ";
    REQUIRE(service.saveQuestion(*admin.value, draft).error
        == quiz::Error::Validation);

    draft.prompt = "A valid question?";
    draft.choices[2] = "   ";
    REQUIRE(service.saveQuestion(*admin.value, draft).error
        == quiz::Error::Validation);

    draft.choices[2] = "three";
    draft.correctChoice = 'E';
    REQUIRE(service.saveQuestion(*admin.value, draft).error
        == quiz::Error::Validation);

    const auto questions = service.listQuestions(*admin.value);
    REQUIRE(questions);
    REQUIRE(questions.value->empty());
}

TEST(deactivated_questions_are_persisted_and_hidden_from_learners) {
    TempDirectory temp;

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);

    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));

    auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);

    quiz::QuestionService service(temp.path);

    quiz::QuestionDraft draft{
        "A question?",
        {"one", "two", "three", "four"},
        'A'
    };

    const auto created = service.saveQuestion(*admin.value, draft);
    REQUIRE(created);

    REQUIRE(service.deactivateQuestion(*admin.value, *created.value)
        == quiz::Error::None);

    quiz::QuestionService restarted(temp.path);
    const auto questions = restarted.listQuestions(*admin.value);

    REQUIRE(questions);
    REQUIRE(questions.value->size() == 1);
    REQUIRE(!questions.value->at(0).active);

    quiz::QuestionCatalog catalog(temp.path);
    REQUIRE(catalog.size() == 0);
}

TEST(question_service_records_are_readable_by_the_existing_question_catalog) {
    TempDirectory temp;

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);

    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));

    auto admin = auth.signIn("owner", "admin-password");
    REQUIRE(admin);

    quiz::QuestionService service(temp.path);

    quiz::QuestionDraft draft{
        "Which option is correct?",
        {"A", "B", "C", "D"},
        'C'
    };

    const auto created = service.saveQuestion(*admin.value, draft);
    REQUIRE(created);

    quiz::QuestionCatalog catalog(temp.path);

    REQUIRE(catalog.size() == 1);
    REQUIRE(catalog.find(*created.value) != nullptr);
    REQUIRE(catalog.find(*created.value)->text == "Which option is correct?");
    REQUIRE(catalog.find(*created.value)->choices[2] == "C");
}

TEST(learner_cannot_change_or_deactivate_an_existing_question) {
    TempDirectory temp;

    quiz::PasswordHasher hasher;
    quiz::FileAccountStore store(temp.path);
    quiz::AuthService auth(store, hasher);

    REQUIRE(auth.provisionFirstAdmin("owner", "admin-password"));
    REQUIRE(auth.registerLearner("alex", "learner-password"));

    auto admin = auth.signIn("owner", "admin-password");
    auto learner = auth.signIn("alex", "learner-password");

    REQUIRE(admin);
    REQUIRE(learner);

    quiz::QuestionService service(temp.path);

    quiz::QuestionDraft original{
        "Original question?",
        {"one", "two", "three", "four"},
        'B'
    };

    const auto created = service.saveQuestion(*admin.value, original);
    REQUIRE(created);

    quiz::QuestionDraft replacement{
        "Changed question?",
        {"five", "six", "seven", "eight"},
        'C'
    };

    const auto edit = service.saveQuestion(
        *learner.value, replacement, *created.value
    );
    REQUIRE(!edit);
    REQUIRE(edit.error == quiz::Error::AccessDenied);

    REQUIRE(service.deactivateQuestion(
        *learner.value, *created.value
    ) == quiz::Error::AccessDenied);

    quiz::QuestionService restarted(temp.path);
    const auto questions = restarted.listQuestions(*admin.value);

    REQUIRE(questions);
    REQUIRE(questions.value->size() == 1);
    REQUIRE(questions.value->at(0).active);
    REQUIRE(questions.value->at(0).data.prompt == "Original question?");
    REQUIRE(questions.value->at(0).data.correctChoice == 'B');
}
