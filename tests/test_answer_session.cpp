#include "test.h"
#include "quiz/answer_session.h"
#include <sstream>
static std::vector<quiz::QuestionPrompt> questions() {
    return {{11, "First?", {"one", "two", "three", "four"}},
        {12, "Second?", {"one", "two", "three", "four"}},
        {13, "Third?", {"one", "two", "three", "four"}}};
}
TEST(invalid_answers_never_advance_or_record_a_choice) {
    quiz::AnswerSession attempt(questions());
    for (const auto& invalid : {"X", "", "AB", "1", "a", " A"}) {
        REQUIRE(!attempt.accept(invalid));
        REQUIRE(attempt.position() == 0);
        REQUIRE(attempt.answers().empty());
    }
    REQUIRE(attempt.accept("A"));
    REQUIRE(attempt.position() == 1);
    REQUIRE(attempt.answers() == std::vector<char>{'A'});
}
TEST(bad_answer_repeats_the_same_question_before_next_question) {
    quiz::AnswerSession attempt(questions());
    std::istringstream input("X\nA\nC\nB\n");
    std::ostringstream output;
    REQUIRE(attempt.readNext(input, output));
    REQUIRE(attempt.answers() == std::vector<char>{'A'});
    const auto first = output.str().find("Question 1 of 3");
    REQUIRE(first != std::string::npos);
    REQUIRE(output.str().find("Question 1 of 3", first + 1) != std::string::npos);
    REQUIRE(output.str().find("Question 2 of 3") == std::string::npos);
    REQUIRE(attempt.readNext(input, output));
    REQUIRE(attempt.readNext(input, output));
    REQUIRE(attempt.complete());
    REQUIRE(attempt.answers() == std::vector<char>({'A', 'C', 'B'}));
    REQUIRE(!attempt.accept("D"));
    REQUIRE(!attempt.readNext(input, output));
}
TEST(oversized_answer_and_eof_preserve_attempt_state) {
    quiz::AnswerSession attempt(questions());
    std::istringstream input(std::string(100000, 'X') + "\nA\n");
    std::ostringstream output;
    REQUIRE(attempt.readNext(input, output));
    REQUIRE(attempt.answers() == std::vector<char>{'A'});
    REQUIRE(!attempt.readNext(input, output));
    REQUIRE(attempt.position() == 1);
    REQUIRE(output.str().find("Invalid answer.") != std::string::npos);
}
