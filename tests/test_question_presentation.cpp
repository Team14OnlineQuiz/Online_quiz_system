#include "test.h"
#include "quiz/question_catalog.h"
#include "quiz/answer_session.h"
#include <fstream>
#include <sstream>
#include "temp_directory.h"

TEST(question_catalog_presents_active_questions_in_id_order_once) {
    TempDirectory temp;
    std::ofstream file(temp.path / "questions.db");
    file << "QUIZ_QUESTIONS_V1\n"
         << "3 1 B \"Third?\" \"a\" \"b\" \"c\" \"d\"\n"
         << "1 1 A \"First?\" \"a\" \"b\" \"c\" \"d\"\n"
         << "2 0 C \"Inactive?\" \"a\" \"b\" \"c\" \"d\"\n";
    file.close();

    quiz::QuestionCatalog catalog(temp.path);
    REQUIRE(catalog.size() == 2);
    REQUIRE(catalog.find(1) != nullptr);
    REQUIRE(catalog.find(2) == nullptr);
    REQUIRE(catalog.find(3) != nullptr);

    auto prompts = catalog.activeQuestions();
    REQUIRE(prompts.size() == 2);
    REQUIRE(prompts[0].id == 1);
    REQUIRE(prompts[1].id == 3);
    quiz::AnswerSession session(std::move(prompts));
    std::istringstream input("A\nB\n");
    std::ostringstream output;
    REQUIRE(session.readNext(input, output));
    REQUIRE(session.readNext(input, output));
    REQUIRE(session.complete());
    REQUIRE(session.answers() == std::vector<char>({'A', 'B'}));
    REQUIRE(!session.readNext(input, output));

    const auto text = output.str();
    REQUIRE(text.find("Question 1 of 2") != std::string::npos);
    REQUIRE(text.find("Question 2 of 2") != std::string::npos);
    REQUIRE(text.find("A. a") != std::string::npos);
    REQUIRE(text.find("D. d") != std::string::npos);
    REQUIRE(text.find("Inactive?") == std::string::npos);
}

TEST(answer_session_reprompts_without_advancing_and_records_one_answer_per_question) {
    quiz::AnswerSession session({
        {1, "First?", {"a", "b", "c", "d"}},
        {2, "Second?", {"a", "b", "c", "d"}}
    });
    std::istringstream input("X\nA\nA\n");
    std::ostringstream output;
    REQUIRE(session.readNext(input, output));
    REQUIRE(session.position() == 1);
    REQUIRE(session.answers() == std::vector<char>{'A'});
    REQUIRE(session.readNext(input, output));
    REQUIRE(session.complete());
    REQUIRE(session.answers() == std::vector<char>({'A', 'A'}));
    const auto text = output.str();
    const auto first = text.find("Question 1 of 2");
    REQUIRE(first != std::string::npos);
    REQUIRE(text.find("Question 1 of 2", first + 1) != std::string::npos);
    REQUIRE(text.find("Invalid answer.") != std::string::npos);
}
