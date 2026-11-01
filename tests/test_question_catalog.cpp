#include "test.h"
#include "temp_directory.h"
#include "quiz/question_catalog.h"
#include <fstream>
TEST(question_catalog_loads_active_prompts_without_exposing_answer_keys) {
    TempDirectory temp;
    std::ofstream(temp.path / "questions.db") << "QUIZ_QUESTIONS_V1\n"
        << "1 1 A \"Question?\" \"one\" \"two\" \"three\" \"four\"\n"
        << "2 0 D \"Inactive?\" \"one\" \"two\" \"three\" \"four\"\n";
    quiz::QuestionCatalog catalog(temp.path);
    REQUIRE(catalog.size() == 1);
    REQUIRE(catalog.find(1)->text == "Question?");
    REQUIRE(catalog.find(2) == nullptr);
    REQUIRE(catalog.find(9999) == nullptr);
}
TEST(question_catalog_rejects_duplicate_ids) {
    TempDirectory temp;
    std::ofstream(temp.path / "questions.db") << "QUIZ_QUESTIONS_V1\n"
        << "1 1 A \"First?\" \"a\" \"b\" \"c\" \"d\"\n"
        << "1 1 A \"Second?\" \"a\" \"b\" \"c\" \"d\"\n";
    bool rejected = false;
    try { quiz::QuestionCatalog catalog(temp.path); } catch (const std::runtime_error&) { rejected = true; }
    REQUIRE(rejected);
}
